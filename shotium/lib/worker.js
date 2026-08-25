'use strict';

const {EventEmitter} = require('events');
const {spawn} = require('child_process');

const {FrameReader, encodeRequest} = require('./protocol');

// One shot.exe --serve process.
//
// Exactly one request is in flight at a time, and that is not a simplification:
// blink is a process-wide singleton bound to the worker's main thread, so a
// second request could not be rendered concurrently even if the protocol
// allowed it. Concurrency is the pool's job, and it gets it by running more
// processes.
//
// Events:
//   ready                       the process has started
//   exit   {code, signal}       it is gone, for any reason
//   crash  {code, signal}       it is gone while it owed an answer
//   stderr {line}               a diagnostic line, useful when a render is wrong
class Worker extends EventEmitter {
  constructor(options) {
    super();
    this.id = options.id;
    // How many requests this process has answered, either way. The pool reads
    // it to tell a worker that was working and then died from one that never
    // came up at all.
    this.served = 0;
    this._binary = options.binary;
    this._args = options.args || [];
    this._pending = null;
    this._stopping = false;
    this._reader = new FrameReader();
    this._header = null;
    this._stderr = '';
    this._start();
  }

  get busy() {
    return this._pending !== null;
  }

  get alive() {
    return this._process !== null && this._process.exitCode === null &&
        !this._stopping;
  }

  _start() {
    this._process = spawn(this._binary, ['--serve', ...this._args], {
      stdio: ['pipe', 'pipe', 'pipe'],
      windowsHide: true,
      // Detached, which on Windows means DETACHED_PROCESS: no console, and so
      // no conhost.exe beside every worker. Four of those cost 40 MB of
      // working set for a console nothing writes to -- the worker's output is
      // three pipes.
      //
      // It does not outlive its supervisor despite the name: the worker exits
      // when its stdin closes, and stdin closes when this process dies.
      detached: true,
    });

    this._process.stdout.on('data', (chunk) => this._onStdout(chunk));
    this._process.stderr.on('data', (chunk) => this._onStderr(chunk));
    this._process.on('error', (error) => this._onGone(null, null, error));
    this._process.on('exit', (code, signal) => this._onGone(code, signal, null));

    // The process is up as soon as spawn resolves the executable; there is no
    // handshake in the protocol, and adding one would only move the failure --
    // a binary that cannot start fails the first request just as visibly.
    this.emit('ready');
  }

  _onStdout(chunk) {
    this._reader.push(chunk);
    for (;;) {
      const frame = this._reader.next();
      if (frame === null) {
        return;
      }
      if (this._header === null) {
        // Frame one of two: the JSON header.
        try {
          this._header = JSON.parse(frame.toString('utf8'));
        } catch (error) {
          this._fail(new Error(
              `shotium: worker ${this.id} sent a header that is not JSON`));
          return;
        }
        continue;
      }
      // Frame two: the image, empty when the header reported a failure or when
      // the worker was asked to write the file itself.
      const header = this._header;
      this._header = null;
      this._settle(header, frame);
    }
  }

  _onStderr(chunk) {
    this._stderr += chunk.toString('utf8');
    const lines = this._stderr.split(/\r?\n/);
    this._stderr = lines.pop();
    for (const line of lines) {
      if (line.length > 0) {
        this.emit('stderr', {worker: this.id, line});
      }
    }
  }

  _onGone(code, signal, error) {
    const wasOwed = this._pending !== null;
    this._process = null;
    if (wasOwed) {
      // A worker that dies mid-request is indistinguishable from one that never
      // answered, which is the point: the retry path does not have to tell a
      // crash from a hang.
      this._fail(
          error ||
          new Error(`shotium: worker ${this.id} exited (code ${code}, signal ${
              signal}) with a request in flight`));
      this.emit('crash', {worker: this.id, code, signal});
    } else if (error) {
      this.emit('error', error);
    }
    this.emit('exit', {worker: this.id, code, signal});
  }

  _settle(header, payload) {
    const pending = this._pending;
    if (!pending) {
      return;
    }
    this._pending = null;
    this.served += 1;
    if (header.ok) {
      pending.resolve({header, image: header.path ? null : payload});
    } else {
      pending.reject(new Error(header.error || 'shotium: request failed'));
    }
  }

  _fail(error) {
    const pending = this._pending;
    if (!pending) {
      return;
    }
    this._pending = null;
    pending.reject(error);
  }

  // Sends one request. Rejects if the worker dies before answering; there is no
  // timeout here, because the deadline belongs to whoever owns the retry
  // policy.
  send(request) {
    if (this._pending) {
      return Promise.reject(
          new Error(`shotium: worker ${this.id} is already busy`));
    }
    if (!this.alive) {
      return Promise.reject(new Error(`shotium: worker ${this.id} is not up`));
    }
    return new Promise((resolve, reject) => {
      this._pending = {resolve, reject};
      this._process.stdin.write(encodeRequest(request), (error) => {
        if (error) {
          this._fail(error);
        }
      });
    });
  }

  // Closing stdin is the shutdown message: the worker sees the frame stream end
  // on a frame boundary and exits 0. kill() is for when it has stopped
  // listening.
  stop() {
    this._stopping = true;
    if (this._process) {
      this._process.stdin.end();
    }
  }

  kill() {
    this._stopping = true;
    if (this._process) {
      this._process.kill();
    }
  }
}

module.exports = {Worker};
