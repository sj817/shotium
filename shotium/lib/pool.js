'use strict';

const {EventEmitter} = require('events');
const fs = require('fs');
const os = require('os');
const path = require('path');

const {Worker} = require('./worker');

// A fixed set of shot.exe --serve processes, and a queue in front of them.
//
// The pool exists because blink is a process-wide singleton: one worker renders
// one document at a time, so N concurrent screenshots means N processes. It is
// also what makes a crash survivable -- a worker that dies takes its own
// request down and nothing else, and the slot is refilled.
//
// Events:
//   ready          {workers}                 the pool is up
//   exit           {worker, code, signal}    a worker is gone
//   crash          {worker, code, signal}    a worker died owing an answer
//   timeout        {worker, timeout}         a request outlived its deadline
//   worker-restart {worker, reason}          a slot was refilled
//   stderr         {worker, line}            a diagnostic line from a worker
class Pool extends EventEmitter {
  constructor(options) {
    super();
    this._binary = options.binary;
    this._size = options.workers;
    this._args = options.args || [];
    this._cacheDir = options.cacheDir || null;
    this._slots = [];
    this._queue = [];
    this._stopping = false;
    this._nextId = 0;
  }

  start() {
    if (this._slots.length > 0) {
      return;
    }
    for (let slot = 0; slot < this._size; ++slot) {
      this._slots[slot] = this._spawn(slot);
    }
    this.emit('ready', {workers: this._size});
  }

  _spawn(slot) {
    const id = this._nextId++;
    const args = [...this._args];
    if (this._cacheDir) {
      // One directory per slot, not per process: the Simple backend takes an
      // exclusive lock on its directory, so sharing one would leave every
      // worker but the first running uncached. Keying on the slot rather than
      // the worker id means a restarted worker inherits the warm cache its
      // predecessor built.
      const dir = path.join(this._cacheDir, `worker-${slot}`);
      fs.mkdirSync(dir, {recursive: true});
      args.push(`--cache-dir=${dir}`);
    }

    const worker = new Worker({id, binary: this._binary, args});
    worker.on('stderr', (event) => this.emit('stderr', event));
    worker.on('crash', (event) => this.emit('crash', event));
    worker.on('exit', (event) => {
      this.emit('exit', event);
      if (this._stopping || this._slots[slot] !== worker) {
        return;
      }
      this._slots[slot] = this._spawn(slot);
      this.emit('worker-restart', {worker: this._slots[slot].id, reason: 'exit'});
      this._pump();
    });
    return worker;
  }

  // Queues one request. `timeout` is the supervisor's deadline, which is longer
  // than the worker's own: the worker fails a slow page by itself and answers,
  // and this only fires when it has stopped answering at all.
  submit(request, {timeout, retry}) {
    return new Promise((resolve, reject) => {
      this._queue.push({
        request,
        timeout,
        attemptsLeft: Math.max(0, retry) + 1,
        resolve,
        reject,
      });
      this._pump();
    });
  }

  _pump() {
    while (this._queue.length > 0) {
      const slot = this._slots.findIndex((w) => w && w.alive && !w.busy);
      if (slot < 0) {
        return;
      }
      this._dispatch(this._slots[slot], this._queue.shift());
    }
  }

  _dispatch(worker, job) {
    job.attemptsLeft -= 1;

    let settled = false;
    const timer = setTimeout(() => {
      if (settled) {
        return;
      }
      settled = true;
      this.emit('timeout', {worker: worker.id, timeout: job.timeout});
      // The worker is not answering, so the only way to get the slot back is to
      // take the process down. The exit handler refills the slot.
      worker.kill();
      this._retryOrFail(
          job,
          new Error(`shotium: no answer within ${job.timeout}ms`));
    }, job.timeout);

    worker.send(job.request)
        .then((result) => {
          if (settled) {
            return;
          }
          settled = true;
          clearTimeout(timer);
          job.resolve(result);
          this._pump();
        })
        .catch((error) => {
          if (settled) {
            return;
          }
          settled = true;
          clearTimeout(timer);
          this._retryOrFail(job, error);
        });
  }

  _retryOrFail(job, error) {
    // A request rejected on its own merits -- a bad selector, an unreadable
    // file -- would fail the same way every time, but the worker also rejects
    // with the same shape when it dies. Retrying both is the safe direction:
    // the cost of a pointless retry is one more render, and the cost of not
    // retrying a crash is a failure the caller cannot do anything about.
    if (job.attemptsLeft > 0 && !this._stopping) {
      this._queue.unshift(job);
      this._pump();
      return;
    }
    job.reject(error);
    this._pump();
  }

  async stop() {
    this._stopping = true;
    for (const job of this._queue.splice(0)) {
      job.reject(new Error('shotium: the runtime was stopped'));
    }
    await Promise.all(this._slots.map((worker) => new Promise((resolve) => {
      if (!worker || !worker.alive) {
        resolve();
        return;
      }
      worker.once('exit', resolve);
      worker.stop();
    })));
    this._slots = [];
  }
}

function defaultCacheDir() {
  return path.join(os.tmpdir(), 'shotium-cache');
}

module.exports = {Pool, defaultCacheDir};
