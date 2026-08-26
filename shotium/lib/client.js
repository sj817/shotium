import {EventEmitter} from 'node:events';
import {spawn} from 'node:child_process';
import fs from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

import {resolveStartOptions} from './config.js';
import {endpointFor} from './endpoint.js';
import {FrameReader, encodeFrame} from './protocol.js';
import {timeoutFor, toRequest} from './request.js';

// ESM has no __dirname. This is the same thing, from the module's own URL.
const HERE = path.dirname(fileURLToPath(import.meta.url));

const DAEMON_MAIN = path.join(HERE, 'daemon_main.js');
// How long to wait for a daemon this process just started to bind its
// endpoint. Binding happens after the workers are spawned but before they are
// warm, so this covers process startup and nothing else.
const START_TIMEOUT_MS = 20000;
const CONNECT_RETRY_MS = 20;

// The client half of the resident daemon.
//
// One connection can carry several requests at once, which is the difference
// between this and the worker protocol underneath: every message carries an
// `id` and the answers are matched back by it, so a caller can fire ten
// screenshots down one socket and let the pool on the other side spread them
// across workers.
class DaemonClient extends EventEmitter {
  constructor(socket, endpoint) {
    super();
    this._socket = socket;
    this._endpoint = endpoint;
    this._pending = new Map();
    this._nextId = 1;
    this._header = null;
    this._reader = new FrameReader();

    socket.on('data', (chunk) => this._onData(chunk));
    socket.on('error', (error) => this._failAll(error));
    socket.on('close', () => {
      this._failAll(new Error('shotium: the daemon closed the connection'));
      this.emit('close', {});
    });
  }

  get endpoint() {
    return this._endpoint;
  }

  get closed() {
    return this._socket.destroyed;
  }

  _onData(chunk) {
    this._reader.push(chunk);
    for (;;) {
      const frame = this._reader.next();
      if (frame === null) {
        return;
      }
      if (this._header === null) {
        try {
          this._header = JSON.parse(frame.toString('utf8'));
        } catch (error) {
          this._failAll(new Error('shotium: the daemon sent a header that is not JSON'));
          return;
        }
        continue;
      }
      const header = this._header;
      this._header = null;
      this._settle(header, frame);
    }
  }

  _settle(header, payload) {
    const pending = this._pending.get(header.id);
    if (!pending) {
      return;
    }
    this._pending.delete(header.id);
    if (header.ok) {
      pending.resolve({header, image: header.path ? null : payload});
    } else {
      pending.reject(new Error(header.error || 'shotium: request failed'));
    }
  }

  _failAll(error) {
    for (const [, pending] of this._pending) {
      pending.reject(error);
    }
    this._pending.clear();
  }

  // Sends one message and resolves with {header, image}.
  send(message) {
    return new Promise((resolve, reject) => {
      if (this._socket.destroyed) {
        reject(new Error('shotium: not connected to a daemon'));
        return;
      }
      const id = this._nextId++;
      this._pending.set(id, {resolve, reject});
      this._socket.write(
          encodeFrame(Buffer.from(JSON.stringify({...message, id}), 'utf8')));
    });
  }

  async screenshot(options) {
    const request = toRequest(options);
    const retry = typeof options.retry === 'number' ? options.retry : 0;
    const result = await this.send({
      op: 'screenshot',
      request,
      timeout: timeoutFor(options),
      retry,
    });
    return result.image;
  }

  async status() {
    const {header} = await this.send({op: 'status'});
    return header;
  }

  async shutdown() {
    const {header} = await this.send({op: 'shutdown'});
    return header;
  }

  close() {
    this._socket.end();
    this._socket.destroy();
  }
}

// Opens a connection to a daemon that is already listening, and fails if there
// is not one. Nothing is spawned here: a caller that wants a daemon started
// says so, because starting one is a side effect on the machine and not the
// sort of thing a status query should do.
function connectOnly(endpoint) {
  return new Promise((resolve, reject) => {
    const socket = net.connect(endpoint);
    const onError = (error) => {
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

function resolveDaemonOptions(options = {}) {
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

function spawnDaemon(options) {
  const config = {
    binary: options.binary,
    workers: options.workers,
    cacheDir: options.cacheDir,
    args: options.args,
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
  let stdio = 'ignore';
  let logFd = null;
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
  return child;
}

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

// Connects, starting a daemon if none answers.
//
// The endpoint existing is the readiness signal, so this is a connect loop
// rather than a handshake: a daemon that has bound can be talked to, and one
// that has not is indistinguishable from one that was never started. Several
// processes racing here is fine -- the losers' daemons exit on EADDRINUSE and
// everyone ends up on the winner.
async function ensureClient(options = {}) {
  const resolved = resolveDaemonOptions(options);
  try {
    const client = await connectOnly(resolved.endpoint);
    return {client, spawned: false, endpoint: resolved.endpoint};
  } catch (error) {
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
    } catch (error) {
      if (Date.now() >= deadline) {
        throw new Error(
            `shotium: the daemon did not come up at ${resolved.endpoint}`);
      }
      await sleep(CONNECT_RETRY_MS);
    }
  }
}

// The five things a caller does with a daemon. Each opens a connection, does
// one thing and closes it, which is the shape a command line wants; a service
// that will send more than one request calls connect() and keeps the client.
async function connect(options = {}) {
  const {client} = await ensureClient(options);
  return client;
}

async function start(options = {}) {
  const {client, spawned, endpoint} = await ensureClient(options);
  try {
    const status = await client.status();
    return {...status, endpoint, spawned};
  } finally {
    client.close();
  }
}

async function status(options = {}) {
  const resolved = resolveDaemonOptions(options);
  let client;
  try {
    client = await connectOnly(resolved.endpoint);
  } catch (error) {
    return {running: false, endpoint: resolved.endpoint};
  }
  try {
    return {...(await client.status()), running: true};
  } finally {
    client.close();
  }
}

async function stop(options = {}) {
  const resolved = resolveDaemonOptions(options);
  let client;
  try {
    client = await connectOnly(resolved.endpoint);
  } catch (error) {
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
async function screenshot(options = {}) {
  const {daemon, ...rest} = options;
  const client = await connect(daemon || {});
  try {
    return await client.screenshot(rest);
  } finally {
    client.close();
  }
}

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
