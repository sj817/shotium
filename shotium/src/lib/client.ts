import {spawn} from 'node:child_process';
import {EventEmitter} from 'node:events';
import fs from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

import type {
  CaptureStats,
  DaemonOptions,
  DaemonStatus,
  ScreenshotOptions,
  ScreenshotResult,
  ScreenshotTile,
  ScreenshotTilesOptions,
  ScreenshotTilesResult,
} from '../types.js';

import {resolveStartOptions} from './config.js';
import {emptyStats} from './engine.js';
import {endpointFor} from './endpoint.js';
import {FrameReader, encodeFrame} from './protocol.js';
import {timeoutFor, toRequest, toTilesRequest} from './request.js';

// ESM has no __dirname. This is the same thing, from the module's own URL.
const HERE = path.dirname(fileURLToPath(import.meta.url));

// The detached daemon's entry point, which is a build output beside this one.
// It is spawned as `node <path>`, so it has to be a file on disk with a name
// that does not move -- see tsdown.config.ts, where it is an entry of its own
// for exactly that reason.
const DAEMON_MAIN = path.join(HERE, 'daemon_main.js');
// How long to wait for a daemon this process just started to bind its
// endpoint. Binding happens after the workers are spawned but before they are
// warm, so this covers process startup and nothing else.
const START_TIMEOUT_MS = 20000;
const CONNECT_RETRY_MS = 20;

// One tile's place on a tiles reply; the image is the matching payload frame.
interface TileReply {
  x: number;
  y: number;
  width: number;
  height: number;
  bytes: number;
  path?: string;
}

interface ClientReply {
  id: number;
  ok?: boolean;
  error?: string;
  path?: string;
  // The daemon reports the same CaptureStats the in-process engine does, in
  // its response header. It rides on the failure header too, which is why the
  // rejection below carries it.
  stats?: CaptureStats;
  // A tiles reply lists its tiles here, and is followed by one payload frame
  // per entry rather than the single frame a screenshot reply has.
  tiles?: TileReply[];
}

interface ClientResult {
  header: ClientReply;
  image: Buffer|null;
  // The payload frames of a tiles reply, in header order.
  images: Buffer[];
}

interface Pending {
  resolve: (result: ClientResult) => void;
  reject: (error: Error) => void;
}

interface ResolvedDaemonOptions {
  cacheDir: string|null;
  userAgent?: string;
  resourceDir?: string;
  name: string|undefined;
  endpoint: string;
  idleTimeoutMs: number|undefined;
  prewarm: boolean|undefined;
  logFile: string|null;
}

// The client half of the resident daemon.
//
// One connection can carry several requests at once: every message carries an
// `id` and the answers are matched back by it, so a caller can fire ten
// screenshots down one socket without waiting between them. They still come
// back one at a time -- there is one renderer on the other side -- so this
// saves the round trips, not the renders.
class DaemonClient extends EventEmitter {
  private readonly socket: net.Socket;
  private readonly endpointPath: string;
  private readonly pending = new Map<number, Pending>();
  private nextId = 1;
  private header: ClientReply|null = null;
  // The payload frames collected for `header` so far. A screenshot reply has
  // one; a tiles reply has one per tile the header lists.
  private payloads: Buffer[] = [];
  private reader = new FrameReader();

  constructor(socket: net.Socket, endpoint: string) {
    super();
    this.socket = socket;
    this.endpointPath = endpoint;

    socket.on('data', (chunk: Buffer) => this.onData(chunk));
    socket.on('error', (error: Error) => this.failAll(error));
    socket.on('close', () => {
      this.failAll(new Error('shotium: the daemon closed the connection'));
      this.emit('close', {});
    });
  }

  get endpoint(): string {
    return this.endpointPath;
  }

  get closed(): boolean {
    return this.socket.destroyed;
  }

  private onData(chunk: Buffer): void {
    this.reader.push(chunk);
    for (;;) {
      const frame = this.reader.next();
      if (frame === null) {
        return;
      }
      if (this.header === null) {
        try {
          this.header = JSON.parse(frame.toString('utf8')) as ClientReply;
        } catch {
          this.failAll(
              new Error('shotium: the daemon sent a header that is not JSON'));
          return;
        }
        this.payloads = [];
        continue;
      }
      this.payloads.push(frame);
      const expected =
          this.header.ok && this.header.tiles ? this.header.tiles.length : 1;
      if (this.payloads.length < expected) {
        continue;
      }
      const header = this.header;
      const payloads = this.payloads;
      this.header = null;
      this.payloads = [];
      this.settle(header, payloads);
    }
  }

  private settle(header: ClientReply, payloads: Buffer[]): void {
    const pending = this.pending.get(header.id);
    if (!pending) {
      return;
    }
    this.pending.delete(header.id);
    if (header.ok) {
      pending.resolve({
        header,
        image: header.path ? null : payloads[0],
        images: payloads,
      });
    } else {
      const error = new Error(header.error || 'shotium: request failed');
      // Attached rather than dropped: a capture that failed part of the way
      // through has already measured what it did, and that is usually the
      // explanation. The in-process engine does the same.
      if (header.stats) {
        (error as Error & {stats?: CaptureStats}).stats = header.stats;
      }
      pending.reject(error);
    }
  }

  private failAll(error: Error): void {
    for (const [, pending] of this.pending) {
      pending.reject(error);
    }
    this.pending.clear();
  }

  // Sends one message and resolves with {header, image}.
  send(message: Record<string, unknown>): Promise<ClientResult> {
    return new Promise<ClientResult>((resolve, reject) => {
      if (this.socket.destroyed) {
        reject(new Error('shotium: not connected to a daemon'));
        return;
      }
      const id = this.nextId++;
      this.pending.set(id, {resolve, reject});
      this.socket.write(
          encodeFrame(Buffer.from(JSON.stringify({...message, id}), 'utf8')));
    });
  }

  /**
   * One screenshot, and what taking it cost.
   *
   * The same shape the in-process engine returns, so that moving a program
   * between the two is an import change and nothing else.
   */
  async screenshot(options: ScreenshotOptions): Promise<ScreenshotResult> {
    const request = toRequest(options);
    const result = await this.send({
      op: 'screenshot',
      request,
      timeout: timeoutFor(options),
    });
    return {image: result.image, stats: result.header.stats ?? emptyStats()};
  }

  /**
   * The region in tiles, through the daemon. The same shape the in-process
   * engine returns; see ScreenshotTilesOptions.
   */
  async screenshotTiles(options: ScreenshotTilesOptions):
      Promise<ScreenshotTilesResult> {
    const request = toTilesRequest(options);
    const result = await this.send({
      op: 'tiles',
      request,
      timeout: timeoutFor(options),
    });
    const listed = result.header.tiles ?? [];
    const tiles: ScreenshotTile[] = listed.map((tile, i) => ({
      image: request.path ? null : result.images[i],
      x: tile.x,
      y: tile.y,
      width: tile.width,
      height: tile.height,
      ...(tile.path !== undefined ? {path: tile.path} : {}),
    }));
    return {tiles, stats: result.header.stats ?? emptyStats()};
  }

  async status(): Promise<DaemonStatus> {
    const {header} = await this.send({op: 'status'});
    return header as unknown as DaemonStatus;
  }

  async shutdown(): Promise<{ok: boolean}> {
    const {header} = await this.send({op: 'shutdown'});
    return {ok: header.ok === true};
  }

  close(): void {
    this.socket.end();
    this.socket.destroy();
  }
}

// Opens a connection to a daemon that is already listening, and fails if there
// is not one. Nothing is spawned here: a caller that wants a daemon started
// says so, because starting one is a side effect on the machine and not the
// sort of thing a status query should do.
function connectOnly(endpoint: string): Promise<DaemonClient> {
  return new Promise<DaemonClient>((resolve, reject) => {
    const socket = net.connect(endpoint);
    const onError = (error: Error) => {
      socket.destroy();
      reject(error);
    };
    socket.once('error', onError);
    socket.once('connect', () => {
      socket.removeListener('error', onError);
      resolve(new DaemonClient(socket, endpoint));
    });
  });
}

function resolveDaemonOptions(options: DaemonOptions = {}):
    ResolvedDaemonOptions {
  const resolved = resolveStartOptions(options);
  return {
    ...resolved,
    name: options.name,
    endpoint: endpointFor({
      ...resolved,
      name: options.name,
      endpoint: options.endpoint,
    }),
    idleTimeoutMs: options.idleTimeoutMs,
    prewarm: options.prewarm,
    logFile: options.logFile || process.env.SHOTIUM_DAEMON_LOG || null,
  };
}

function spawnDaemon(options: ResolvedDaemonOptions): void {
  const config = {
    cacheDir: options.cacheDir,
    userAgent: options.userAgent,
    resourceDir: options.resourceDir,
    endpoint: options.endpoint,
    idleTimeoutMs: options.idleTimeoutMs,
    prewarm: options.prewarm,
  };
  const encoded =
      Buffer.from(JSON.stringify(config), 'utf8').toString('base64');

  // Detached, with the standard streams let go of: the daemon has to outlive
  // the process that started it, and a child still holding this process's pipes
  // would keep it from exiting -- the exact failure that makes a "background"
  // daemon hang a shell.
  let stdio: 'ignore'|['ignore', number, number] = 'ignore';
  let logFd: number|null = null;
  if (options.logFile) {
    logFd = fs.openSync(options.logFile, 'a');
    stdio = ['ignore', logFd, logFd];
  }
  const child = spawn(process.execPath, [DAEMON_MAIN, encoded], {
    detached: true,
    stdio,
    windowsHide: true,
  });
  child.unref();
  if (logFd !== null) {
    fs.closeSync(logFd);
  }
}

const sleep = (ms: number) =>
    new Promise<void>((resolve) => setTimeout(resolve, ms));

export interface EnsuredClient {
  client: DaemonClient;
  spawned: boolean;
  endpoint: string;
}

// Connects, starting a daemon if none answers.
//
// The endpoint existing is the readiness signal, so this is a connect loop
// rather than a handshake: a daemon that has bound can be talked to, and one
// that has not is indistinguishable from one that was never started. Several
// processes racing here is fine -- the losers' daemons exit on EADDRINUSE and
// everyone ends up on the winner.
async function ensureClient(options: DaemonOptions = {}):
    Promise<EnsuredClient> {
  const resolved = resolveDaemonOptions(options);
  try {
    const client = await connectOnly(resolved.endpoint);
    return {client, spawned: false, endpoint: resolved.endpoint};
  } catch {
    if (options.spawn === false) {
      throw new Error(`shotium: no daemon at ${resolved.endpoint}`);
    }
  }

  spawnDaemon(resolved);
  const deadline = Date.now() +
      (options.startTimeoutMs === undefined ? START_TIMEOUT_MS :
                                              options.startTimeoutMs);
  for (;;) {
    try {
      const client = await connectOnly(resolved.endpoint);
      return {client, spawned: true, endpoint: resolved.endpoint};
    } catch {
      if (Date.now() >= deadline) {
        throw new Error(
            `shotium: the daemon did not come up at ${resolved.endpoint}`);
      }
      await sleep(CONNECT_RETRY_MS);
    }
  }
}

// The five things a caller does with a daemon. Each opens a connection, does
// one thing and closes it, which is the shape a short-lived process wants; a
// service that will send more than one request calls connect() and keeps the
// client.
async function connect(options: DaemonOptions = {}): Promise<DaemonClient> {
  const {client} = await ensureClient(options);
  return client;
}

async function start(options: DaemonOptions = {}):
    Promise<DaemonStatus&{spawned: boolean}> {
  const {client, spawned, endpoint} = await ensureClient(options);
  try {
    const status = await client.status();
    return {...status, endpoint, spawned};
  } finally {
    client.close();
  }
}

async function status(options: DaemonOptions = {}):
    Promise<Partial<DaemonStatus>&{running: boolean, endpoint: string}> {
  const resolved = resolveDaemonOptions(options);
  let client: DaemonClient;
  try {
    client = await connectOnly(resolved.endpoint);
  } catch {
    return {running: false, endpoint: resolved.endpoint};
  }
  try {
    return {...(await client.status()), running: true};
  } finally {
    client.close();
  }
}

async function stop(options: DaemonOptions = {}):
    Promise<{stopped: boolean, endpoint: string}> {
  const resolved = resolveDaemonOptions(options);
  let client: DaemonClient;
  try {
    client = await connectOnly(resolved.endpoint);
  } catch {
    return {stopped: false, endpoint: resolved.endpoint};
  }
  try {
    await client.shutdown();
    return {stopped: true, endpoint: resolved.endpoint};
  } finally {
    client.close();
  }
}

// One screenshot through the daemon, connection and all. `daemon` carries the
// pool's configuration -- binary, workers, cache root -- and is stripped out
// here rather than sent, because it says which daemon to talk to and not what
// to photograph.
async function screenshot(options: ScreenshotOptions&{daemon?: DaemonOptions}):
    Promise<ScreenshotResult> {
  const {daemon, ...rest} = options;
  const client = await connect(daemon || {});
  try {
    return await client.screenshot(rest);
  } finally {
    client.close();
  }
}

async function screenshotTiles(
    options: ScreenshotTilesOptions&{daemon?: DaemonOptions}):
    Promise<ScreenshotTilesResult> {
  const {daemon, ...rest} = options;
  const client = await connect(daemon || {});
  try {
    return await client.screenshotTiles(rest);
  } finally {
    client.close();
  }
}

export {screenshotTiles};

export {
  DaemonClient,
  connect,
  ensureClient,
  resolveDaemonOptions,
  screenshot,
  start,
  status,
  stop,
};
