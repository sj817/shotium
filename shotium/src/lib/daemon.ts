import {EventEmitter} from 'node:events';
import fs from 'node:fs';
import net from 'node:net';

import type {DaemonOptions, DaemonStatus} from '../types.js';

import {resolveStartOptions} from './config.js';
import type {ResolvedStartOptions} from './config.js';
import {endpointFor} from './endpoint.js';
import {Pool} from './pool.js';
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

// How much longer than the page's own deadline the daemon waits before it
// decides a worker is not going to answer at all. Same margin, same reasoning
// as index.ts: the worker fails a slow page by itself and replies.
const SUPERVISOR_MARGIN_MS = 10000;
const DEFAULT_TIMEOUT_MS = 30000;
const DEFAULT_IDLE_TIMEOUT_MS = 300000;

// One message off the socket. `op` defaults to screenshot because that is what
// almost every message is.
interface DaemonMessage {
  id?: number|null;
  op?: 'screenshot'|'status'|'ping'|'shutdown';
  request?: WireRequest;
  timeout?: number;
  retry?: number;
}

interface DaemonReply {
  id: number|null;
  ok?: boolean;
  error?: string;
  bytes?: number;
  path?: string;
  stopping?: boolean;
}

// A worker pool that outlives the process that asked for it.
//
// The pool in index.ts is already resident, but only for as long as the Node
// process holding it: a command-line invocation, a CI step, a serverless
// handler and a `node -e` all pay for starting workers and then throw them
// away. This is the same pool behind a socket, so the second caller -- in a
// different process, minutes later -- pays a connect() and nothing else.
//
// The wire format is the worker's own, one level up: a request frame of JSON,
// answered by a header frame and a payload frame. What it adds is `id`, so one
// connection can have several requests in flight; the worker protocol cannot,
// because a worker renders one document at a time, and multiplexing is exactly
// what the pool in the middle is for.
//
//   ->  [len][{"id":7,"op":"screenshot","request":{...}}]
//   <-  [len][{"id":7,"ok":true,"bytes":97756}]  [len][<PNG>]
//
// Events: ready, request, response, idle-exit, error, plus the pool's own.
class Daemon extends EventEmitter {
  private readonly options: ResolvedStartOptions;
  private readonly endpointPath: string;
  private readonly idleTimeoutMs: number;
  private readonly prewarmOnStart: boolean;
  private pool: Pool|null = null;
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

  // Brings the pool up and starts listening. The pipe existing *is* the
  // readiness signal -- a client's connect() either succeeds or the daemon is
  // not up -- so nothing is bound until the pool has been asked to start.
  async listen(): Promise<this> {
    const pool = new Pool(this.options);
    this.pool = pool;
    for (const event of ['exit', 'crash', 'timeout', 'worker-restart',
                         'worker-error', 'stderr']) {
      pool.on(event, (payload) => this.emit(event, payload));
    }
    pool.start();

    this.server = net.createServer((socket) => this.accept(socket));
    this.server.on('error', (error) => this.emit('error', error));
    await this.bind();
    this.armIdleTimer();
    this.emit('ready',
              {endpoint: this.endpointPath, workers: this.options.workers});
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
  // Windows host is therefore as trusted as the machine's users are -- render
  // in-process, or with a binary that has no file access, if that is not
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

  // Renders one throwaway document per worker so that the first real request
  // does not pay for whatever each process initialises lazily. The pool hands
  // one request to each free worker, and there are exactly as many requests as
  // workers, so every process is touched.
  //
  // `data:` rather than a file, because a daemon started without
  // --allow-file-access would otherwise be prewarmed by a request it refuses.
  async prewarm(): Promise<void> {
    const blank = 'data:text/html,<!doctype html><title>shotium</title>';
    await Promise.all(Array.from({length: this.options.workers}, () => {
      return this.pool!
          .submit({file: blank, width: 16, height: 16},
                  {timeout: DEFAULT_TIMEOUT_MS + SUPERVISOR_MARGIN_MS, retry: 1})
          .catch(() => null);
    }));
    this.warmed = true;
    this.emit('warm', {workers: this.options.workers});
  }

  status(): DaemonStatus {
    return {
      ok: true,
      pid: process.pid,
      endpoint: this.endpointPath,
      binary: this.options.binary,
      workers: this.options.workers,
      cacheDir: this.options.cacheDir,
      args: this.options.args,
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
    if (op !== 'screenshot') {
      this.reply(socket, {id, ok: false, error: `shotium: unknown op "${op}"`});
      return;
    }

    const request = message.request || ({} as WireRequest);
    const timeout = (typeof message.timeout === 'number' ? message.timeout :
                                                           DEFAULT_TIMEOUT_MS) +
        SUPERVISOR_MARGIN_MS;
    const retry = typeof message.retry === 'number' ? message.retry : 0;

    this.inFlight += 1;
    this.armIdleTimer();
    this.emit('request', {id, file: request.file});
    this.pool!.submit(request, {timeout, retry})
        .then((result) => {
          this.served += 1;
          this.reply(
              socket,
              {
                id,
                ok: true,
                bytes: result.image ? result.image.length : 0,
                path: result.header ? result.header.path : undefined,
              },
              result.image);
        })
        .catch((error: Error) => {
          this.reply(
              socket, {id, ok: false, error: String(error.message || error)});
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
    await this.pool!.stop();
    this.emit('close', {});
  }
}

export {Daemon, DEFAULT_IDLE_TIMEOUT_MS};
