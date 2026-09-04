import {EventEmitter} from 'node:events';
import fs from 'node:fs';
import net from 'node:net';
import os from 'node:os';
import path from 'node:path';

import type {
  CaptureStats,
  DaemonOptions,
  DaemonStatus,
} from '../types.js';

import {resolveStartOptions} from './config.js';
import type {ResolvedStartOptions} from './config.js';
import {endpointFor} from './endpoint.js';
import {Engine} from './engine.js';
import {FrameReader, encodeFrame} from './protocol.js';
import type {WireRequest} from './request.js';

// Our own version, for status(). Read rather than imported: an import
// attribute would do it too, but only on a node new enough that this package
// would not run on the rest. The URL is relative to the built module, which
// sits one directory below the manifest.
const VERSION = (() => {
  try {
    const manifest =
        fs.readFileSync(new URL('../package.json', import.meta.url), 'utf8');
    return (JSON.parse(manifest) as {version?: string}).version ?? '0.0.0';
  } catch {
    return '0.0.0';
  }
})();

const DEFAULT_IDLE_TIMEOUT_MS = 300000;

// One message off the socket. `op` defaults to screenshot because that is what
// almost every message is.
interface DaemonMessage {
  id?: number|null;
  op?: 'screenshot'|'tiles'|'status'|'ping'|'shutdown';
  request?: WireRequest;
  timeout?: number;
  retry?: number;
}

// One tile's place and size on a tiles reply. The image itself follows the
// header as a frame of its own, one per entry, in this order.
interface TileHeader {
  x: number;
  y: number;
  width: number;
  height: number;
  bytes: number;
  path?: string;
}

interface DaemonReply {
  id: number|null;
  ok?: boolean;
  error?: string;
  bytes?: number;
  path?: string;
  stopping?: boolean;
  // A tiles reply: the header lists the tiles and as many payload frames
  // follow as there are entries, instead of the one frame a screenshot gets.
  tiles?: TileHeader[];
  // What the capture cost, on the success header and on the failure one. The
  // client turns it back into the same CaptureStats the in-process engine
  // returns, so a program moving between the two changes an import and
  // nothing else.
  stats?: CaptureStats;
}

// An engine that outlives the process that asked for it.
//
// The engine in index.ts is already resident, but only for as long as the Node
// process holding it: a command-line invocation, a CI step, a serverless
// handler and a `node -e` all pay for starting Blink and then throw it away.
// This is the same engine behind a socket, so the second caller -- in a
// different process, minutes later -- pays a connect() and nothing else.
//
// A request frame of JSON, answered by a header frame and a payload frame:
//
//   ->  [len][{"id":7,"op":"screenshot","request":{...}}]
//   <-  [len][{"id":7,"ok":true,"bytes":97756}]  [len][<PNG>]
//
// `id` is on the wire so that a client may have several requests outstanding
// on one connection. That is a convenience for the client, not concurrency:
// there is one renderer here, because Blink is a process-wide singleton, so
// the requests queue and come back in the order the engine finished them.
// Wanting two at once means wanting two daemons, addressed by `name`.
//
// Nothing supervises a capture. The pool this replaced could time a worker out
// and kill it; an in-process engine has no such seam -- there is no way to
// abandon a render without abandoning the process. A page's own deadline
// (`pageGotoParams.timeout`) is what bounds it, and the engine answers slow
// pages by itself. `timeout` and `retry` on the wire are accepted and ignored,
// so that an older client still talks to this.
//
// Events: ready, warm, request, response, idle-exit, error, close.
class Daemon extends EventEmitter {
  private readonly options: ResolvedStartOptions;
  private readonly endpointPath: string;
  private readonly idleTimeoutMs: number;
  private readonly prewarmOnStart: boolean;
  private readonly engine = new Engine();
  private server: net.Server|null = null;
  private sockets = new Set<net.Socket>();
  private inFlight = 0;
  private served = 0;
  private warmed = false;
  private startedAt = Date.now();
  private idleTimer: NodeJS.Timeout|null = null;
  private closing = false;

  constructor(options: DaemonOptions = {}) {
    super();
    this.options = resolveStartOptions(options);
    this.endpointPath = endpointFor({
      ...this.options,
      name: options.name,
      endpoint: options.endpoint,
    });
    this.idleTimeoutMs = options.idleTimeoutMs === undefined ?
        DEFAULT_IDLE_TIMEOUT_MS :
        options.idleTimeoutMs;
    this.prewarmOnStart = options.prewarm !== false;
  }

  get endpoint(): string {
    return this.endpointPath;
  }

  get warm(): boolean {
    return this.warmed;
  }

  // Brings the engine up and starts listening. The pipe existing *is* the
  // readiness signal -- a client's connect() either succeeds or the daemon is
  // not up -- so nothing is bound until the engine has started.
  //
  // Starting it here rather than on the first request is deliberate: a machine
  // with no engine for its platform should fail while the caller is still
  // watching, not answer a connect() and then reject every request on it.
  async listen(): Promise<this> {
    this.engine.start(this.options);

    this.server = net.createServer((socket) => this.accept(socket));
    this.server.on('error', (error) => this.emit('error', error));
    await this.bind();
    this.armIdleTimer();
    this.emit('ready', {endpoint: this.endpointPath});
    if (this.prewarmOnStart) {
      await this.prewarm();
    }
    return this;
  }

  private bind(): Promise<void> {
    return new Promise<void>((resolve, reject) => {
      const server = this.server!;
      const onError = (error: NodeJS.ErrnoException) => {
        // A unix socket file outlives the process that made it, so EADDRINUSE
        // means either a live daemon or a leftover path. Connecting is the only
        // way to tell them apart: refused means nobody is home, and the file
        // can go.
        if (error.code === 'EADDRINUSE' && process.platform !== 'win32') {
          const probe = net.connect(this.endpointPath);
          probe.on('connect', () => {
            probe.destroy();
            reject(error);
          });
          probe.on('error', () => {
            try {
              fs.unlinkSync(this.endpointPath);
            } catch {
              reject(error);
              return;
            }
            server.listen(this.endpointPath, () => {
              this.restrict();
              resolve();
            });
          });
          return;
        }
        reject(error);
      };
      server.once('error', onError);
      server.listen(this.endpointPath, () => {
        server.removeListener('error', onError);
        this.restrict();
        resolve();
      });
    });
  }

  // Who may talk to this daemon.
  //
  // It matters because a request may set `allowFileAccess`, so a stranger who
  // can connect can have a document read this machine's filesystem and get the
  // result back as a picture. On POSIX the socket is a file and 0600 says only
  // its owner may connect.
  //
  // On Windows it is a named pipe, and node exposes no way to give one an ACL:
  // the default lets any account on the machine open it. A daemon on a shared
  // Windows host is therefore as trusted as the machine's users are -- use the
  // engine in your own process, where nothing is listening, if that is not
  // acceptable.
  private restrict(): void {
    if (process.platform === 'win32') {
      return;
    }
    try {
      fs.chmodSync(this.endpointPath, 0o600);
    } catch (error) {
      this.emit('error', error);
    }
  }

  // Renders one throwaway document so that the first real request does not pay
  // for whatever the engine initialises lazily. One is enough: there is one
  // renderer, and it is the same one every request lands on.
  //
  // A temporary file, not a `data:` URL. This used to send
  // `data:text/html,...`, which the renderer rejects -- shot_capture.cc takes
  // file, http and https and nothing else -- so every prewarm failed into the
  // catch below and the step had never once done anything. The failure was
  // invisible because a prewarm that does not work looks exactly like one that
  // does, only slower on the first request.
  //
  // The document names no subresources, so it renders identically whether or
  // not this daemon allows file access -- which is what the `data:` URL was
  // reaching for. A top-level file: URL always loads; `allowFileAccess` gates
  // what the document may then pull in.
  async prewarm(): Promise<void> {
    const blank = path.join(
        os.tmpdir(), `shotium-prewarm-${process.pid}.html`);
    try {
      fs.writeFileSync(
          blank, '<!doctype html><title>shotium</title><p>shotium');
      await this.engine.capture({file: blank, width: 16, height: 16});
      this.warmed = true;
    } catch (error) {
      // Not fatal: a daemon that could not prewarm still serves. But it is not
      // warm, and status() should not claim it is.
      this.emit('error', error);
    } finally {
      fs.rmSync(blank, {force: true});
    }
    this.emit('warm', {warm: this.warmed});
  }

  status(): DaemonStatus {
    return {
      ok: true,
      pid: process.pid,
      endpoint: this.endpointPath,
      cacheDir: this.options.cacheDir,
      userAgent: this.options.userAgent,
      resourceDir: this.options.resourceDir,
      warm: this.warmed,
      uptimeMs: Date.now() - this.startedAt,
      connections: this.sockets.size,
      inFlight: this.inFlight,
      served: this.served,
      idleTimeoutMs: this.idleTimeoutMs,
      version: VERSION,
    };
  }

  private accept(socket: net.Socket): void {
    socket.on('error', () => socket.destroy());
    this.sockets.add(socket);
    this.armIdleTimer();

    const reader = new FrameReader();
    socket.on('data', (chunk: Buffer) => {
      reader.push(chunk);
      for (;;) {
        const frame = reader.next();
        if (frame === null) {
          return;
        }
        this.dispatch(socket, frame);
      }
    });
    socket.on('close', () => {
      this.sockets.delete(socket);
      this.armIdleTimer();
    });
  }

  private dispatch(socket: net.Socket, frame: Buffer): void {
    let message: DaemonMessage;
    try {
      message = JSON.parse(frame.toString('utf8')) as DaemonMessage;
    } catch {
      this.reply(
          socket, {id: null, ok: false, error: 'shotium: request is not JSON'});
      return;
    }

    const id = message.id === undefined ? null : message.id;
    const op = message.op || 'screenshot';
    if (op === 'status') {
      this.reply(socket, {...this.status(), id});
      return;
    }
    if (op === 'ping') {
      this.reply(socket, {id, ok: true});
      return;
    }
    if (op === 'shutdown') {
      this.reply(socket, {id, ok: true, stopping: true});
      // After the reply is on the wire, not before: a client that asked for a
      // shutdown is entitled to hear that it happened.
      socket.end(() => void this.close());
      return;
    }
    if (op !== 'screenshot' && op !== 'tiles') {
      this.reply(socket, {id, ok: false, error: `shotium: unknown op "${op}"`});
      return;
    }

    const request = message.request || ({} as WireRequest);

    this.inFlight += 1;
    this.armIdleTimer();
    this.emit('request', {id, file: request.file});
    const capture = op === 'tiles' ?
        this.engine.captureTiles(request).then(({tiles, stats}) => {
          this.served += 1;
          const payloads = tiles.map((tile) => tile.image || Buffer.alloc(0));
          this.replyMany(
              socket, {
                id,
                ok: true,
                tiles: tiles.map((tile, i) => ({
                  x: tile.x,
                  y: tile.y,
                  width: tile.width,
                  height: tile.height,
                  bytes: payloads[i].length,
                  ...(tile.path !== undefined ? {path: tile.path} : {}),
                })),
                stats,
              },
              payloads);
        }) :
        this.engine.capture(request).then(({image, stats}) => {
          this.served += 1;
          this.reply(
              socket,
              {
                id,
                ok: true,
                bytes: image ? image.length : 0,
                path: request.path,
                stats,
              },
              image);
        });
    capture
        .catch((error: Error&{stats?: CaptureStats}) => {
          // The counters go back with the failure, matching the in-process
          // engine: a capture that timed out after fetching forty subresources
          // has already said why, and the message alone has not.
          this.reply(socket, {
            id,
            ok: false,
            error: String(error.message || error),
            stats: error.stats,
          });
        })
        .finally(() => {
          this.inFlight -= 1;
          this.emit('response', {id});
          this.armIdleTimer();
        });
  }

  private reply(
      socket: net.Socket, header: DaemonReply|(DaemonStatus&{id: number|null}),
      payload?: Buffer|null): void {
    if (socket.destroyed) {
      return;
    }
    socket.write(encodeFrame(Buffer.from(JSON.stringify(header), 'utf8')));
    socket.write(encodeFrame(payload || Buffer.alloc(0)));
  }

  // A tiles reply: the header, then one frame per tile it lists.
  private replyMany(
      socket: net.Socket, header: DaemonReply, payloads: Buffer[]): void {
    if (socket.destroyed) {
      return;
    }
    socket.write(encodeFrame(Buffer.from(JSON.stringify(header), 'utf8')));
    for (const payload of payloads) {
      socket.write(encodeFrame(payload));
    }
  }

  // Idle is "nobody connected and nothing rendering". A client that holds its
  // socket open -- a long-lived service using connect() -- keeps the daemon
  // alive without having to poll it.
  private armIdleTimer(): void {
    if (this.idleTimer) {
      clearTimeout(this.idleTimer);
      this.idleTimer = null;
    }
    if (!this.idleTimeoutMs || this.closing) {
      return;
    }
    if (this.sockets.size > 0 || this.inFlight > 0) {
      return;
    }
    this.idleTimer = setTimeout(() => {
      this.emit('idle-exit', {idleTimeoutMs: this.idleTimeoutMs});
      void this.close();
    }, this.idleTimeoutMs);
    this.idleTimer.unref();
  }

  async close(): Promise<void> {
    if (this.closing) {
      return;
    }
    this.closing = true;
    if (this.idleTimer) {
      clearTimeout(this.idleTimer);
      this.idleTimer = null;
    }
    for (const socket of this.sockets) {
      socket.destroy();
    }
    this.sockets.clear();
    await new Promise<void>((resolve) => this.server!.close(() => resolve()));
    // dispose() rather than stop(), and this is the one caller that should.
    // The daemon owns its process and is leaving it, so the real teardown is
    // available and worth taking: joining the engine thread unwinds the
    // network stack, which is what lets the disk cache write its index. A
    // daemon that merely stood the engine down would leave the index dirty and
    // make the next daemon rebuild it by scanning the directory.
    await this.engine.dispose();
    this.emit('close', {});
  }
}

export {Daemon, DEFAULT_IDLE_TIMEOUT_MS};
