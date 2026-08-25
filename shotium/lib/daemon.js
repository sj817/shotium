'use strict';

const {EventEmitter} = require('events');
const fs = require('fs');
const net = require('net');

const {Pool} = require('./pool');
const {resolveStartOptions} = require('./config');
const {endpointFor} = require('./endpoint');
const {FrameReader, encodeFrame} = require('./protocol');

// How much longer than the page's own deadline the daemon waits before it
// decides a worker is not going to answer at all. Same margin, same reasoning
// as index.js: the worker fails a slow page by itself and replies.
const SUPERVISOR_MARGIN_MS = 10000;
const DEFAULT_TIMEOUT_MS = 30000;
const DEFAULT_IDLE_TIMEOUT_MS = 300000;

// A worker pool that outlives the process that asked for it.
//
// The pool in index.js is already resident, but only for as long as the Node
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
  constructor(options = {}) {
    super();
    this._options = resolveStartOptions(options);
    this._endpoint = endpointFor({
      ...this._options,
      name: options.name,
      endpoint: options.endpoint,
    });
    this._idleTimeoutMs = options.idleTimeoutMs === undefined ?
        DEFAULT_IDLE_TIMEOUT_MS :
        options.idleTimeoutMs;
    this._prewarm = options.prewarm !== false;
    this._pool = null;
    this._server = null;
    this._sockets = new Set();
    this._inFlight = 0;
    this._served = 0;
    this._warm = false;
    this._startedAt = Date.now();
    this._idleTimer = null;
    this._closing = false;
  }

  get endpoint() {
    return this._endpoint;
  }

  get warm() {
    return this._warm;
  }

  // Brings the pool up and starts listening. The pipe existing *is* the
  // readiness signal -- a client's connect() either succeeds or the daemon is
  // not up -- so nothing is bound until the pool has been asked to start.
  async listen() {
    this._pool = new Pool(this._options);
    for (const event of ['exit', 'crash', 'timeout', 'worker-restart',
                         'worker-error', 'stderr']) {
      this._pool.on(event, (payload) => this.emit(event, payload));
    }
    this._pool.start();

    this._server = net.createServer((socket) => this._accept(socket));
    this._server.on('error', (error) => this.emit('error', error));
    await this._bind();
    this._armIdleTimer();
    this.emit('ready', {endpoint: this._endpoint, workers: this._options.workers});
    if (this._prewarm) {
      await this.prewarm();
    }
    return this;
  }

  _bind() {
    return new Promise((resolve, reject) => {
      const onError = (error) => {
        // A unix socket file outlives the process that made it, so EADDRINUSE
        // means either a live daemon or a leftover path. Connecting is the only
        // way to tell them apart: refused means nobody is home, and the file
        // can go.
        if (error.code === 'EADDRINUSE' && process.platform !== 'win32') {
          const probe = net.connect(this._endpoint);
          probe.on('connect', () => {
            probe.destroy();
            reject(error);
          });
          probe.on('error', () => {
            try {
              fs.unlinkSync(this._endpoint);
            } catch (unlinkError) {
              reject(error);
              return;
            }
            this._server.listen(this._endpoint, () => {
              this._restrict();
              resolve();
            });
          });
          return;
        }
        reject(error);
      };
      this._server.once('error', onError);
      this._server.listen(this._endpoint, () => {
        this._server.removeListener('error', onError);
        this._restrict();
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
  // Windows host is therefore as trusted as the machine's users are -- run it
  // with `--no-daemon`, or with a binary that has no file access, if that is
  // not acceptable.
  _restrict() {
    if (process.platform === 'win32') {
      return;
    }
    try {
      fs.chmodSync(this._endpoint, 0o600);
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
  async prewarm() {
    const blank = 'data:text/html,<!doctype html><title>shotium</title>';
    await Promise.all(Array.from({length: this._options.workers}, () => {
      return this._pool
          .submit({file: blank, width: 16, height: 16},
                  {timeout: DEFAULT_TIMEOUT_MS + SUPERVISOR_MARGIN_MS, retry: 1})
          .catch(() => null);
    }));
    this._warm = true;
    this.emit('warm', {workers: this._options.workers});
  }

  status() {
    return {
      ok: true,
      pid: process.pid,
      endpoint: this._endpoint,
      binary: this._options.binary,
      workers: this._options.workers,
      cacheDir: this._options.cacheDir,
      args: this._options.args,
      warm: this._warm,
      uptimeMs: Date.now() - this._startedAt,
      connections: this._sockets.size,
      inFlight: this._inFlight,
      served: this._served,
      idleTimeoutMs: this._idleTimeoutMs,
      version: require('../package.json').version,
    };
  }

  _accept(socket) {
    socket.on('error', () => socket.destroy());
    this._sockets.add(socket);
    this._armIdleTimer();

    const reader = new FrameReader();
    socket.on('data', (chunk) => {
      reader.push(chunk);
      for (;;) {
        const frame = reader.next();
        if (frame === null) {
          return;
        }
        this._dispatch(socket, frame);
      }
    });
    socket.on('close', () => {
      this._sockets.delete(socket);
      this._armIdleTimer();
    });
  }

  _dispatch(socket, frame) {
    let message;
    try {
      message = JSON.parse(frame.toString('utf8'));
    } catch (error) {
      this._reply(socket, {id: null, ok: false, error: 'shotium: request is not JSON'});
      return;
    }

    const id = message.id === undefined ? null : message.id;
    const op = message.op || 'screenshot';
    if (op === 'status') {
      this._reply(socket, {...this.status(), id});
      return;
    }
    if (op === 'ping') {
      this._reply(socket, {id, ok: true});
      return;
    }
    if (op === 'shutdown') {
      this._reply(socket, {id, ok: true, stopping: true});
      // After the reply is on the wire, not before: a client that asked for a
      // shutdown is entitled to hear that it happened.
      socket.end(() => this.close());
      return;
    }
    if (op !== 'screenshot') {
      this._reply(socket, {id, ok: false, error: `shotium: unknown op "${op}"`});
      return;
    }

    const request = message.request || {};
    const timeout = (typeof message.timeout === 'number' ? message.timeout :
                                                           DEFAULT_TIMEOUT_MS) +
        SUPERVISOR_MARGIN_MS;
    const retry = typeof message.retry === 'number' ? message.retry : 0;

    this._inFlight += 1;
    this._armIdleTimer();
    this.emit('request', {id, file: request.file});
    this._pool.submit(request, {timeout, retry})
        .then((result) => {
          this._served += 1;
          this._reply(
              socket,
              {
                id,
                ok: true,
                bytes: result.image ? result.image.length : 0,
                path: result.header ? result.header.path : undefined,
              },
              result.image);
        })
        .catch((error) => {
          this._reply(socket, {id, ok: false, error: String(error.message || error)});
        })
        .finally(() => {
          this._inFlight -= 1;
          this.emit('response', {id});
          this._armIdleTimer();
        });
  }

  _reply(socket, header, payload) {
    if (socket.destroyed) {
      return;
    }
    socket.write(encodeFrame(Buffer.from(JSON.stringify(header), 'utf8')));
    socket.write(encodeFrame(payload || Buffer.alloc(0)));
  }

  // Idle is "nobody connected and nothing rendering". A client that holds its
  // socket open -- a long-lived service using connect() -- keeps the daemon
  // alive without having to poll it.
  _armIdleTimer() {
    if (this._idleTimer) {
      clearTimeout(this._idleTimer);
      this._idleTimer = null;
    }
    if (!this._idleTimeoutMs || this._closing) {
      return;
    }
    if (this._sockets.size > 0 || this._inFlight > 0) {
      return;
    }
    this._idleTimer = setTimeout(() => {
      this.emit('idle-exit', {idleTimeoutMs: this._idleTimeoutMs});
      this.close();
    }, this._idleTimeoutMs);
    this._idleTimer.unref();
  }

  async close() {
    if (this._closing) {
      return;
    }
    this._closing = true;
    if (this._idleTimer) {
      clearTimeout(this._idleTimer);
      this._idleTimer = null;
    }
    for (const socket of this._sockets) {
      socket.destroy();
    }
    this._sockets.clear();
    await new Promise((resolve) => this._server.close(resolve));
    await this._pool.stop();
    this.emit('close', {});
  }
}

module.exports = {Daemon, DEFAULT_IDLE_TIMEOUT_MS};
