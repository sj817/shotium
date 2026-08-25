'use strict';

const {EventEmitter} = require('events');
const fs = require('fs');
const path = require('path');

const {Worker} = require('./worker');
const {defaultCacheDir} = require('./config');

// A worker that exits sooner than this never really started, so its slot is
// refilled on a doubling delay rather than immediately.
const FAST_FAILURE_MS = 1000;
const RESPAWN_DELAY_MS = 100;
const MAX_RESPAWN_DELAY_MS = 5000;

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
//   worker-restart {worker, reason, delay}   a slot was refilled
//   worker-error   {worker, error}           a worker could not be started
//   stderr         {worker, line}            a diagnostic line from a worker
class Pool extends EventEmitter {
  constructor(options) {
    super();
    this._binary = options.binary;
    this._size = options.workers;
    this._args = options.args || [];
    this._cacheDir = options.cacheDir || null;
    this._slots = [];
    this._failures = [];
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

    const startedAt = Date.now();
    const worker = new Worker({id, binary: this._binary, args});
    worker.on('stderr', (event) => this.emit('stderr', event));
    worker.on('crash', (event) => this.emit('crash', event));
    // A worker that could not be started at all -- a binary that is not there,
    // a path that is not executable -- reports it here. Without a listener
    // EventEmitter throws the error instead, which for a resident daemon means
    // a typo in a path takes the whole pool down.
    worker.on('error', (error) => this.emit('worker-error', {worker: id, error}));
    worker.on('exit', (event) => {
      this.emit('exit', event);
      if (this._stopping || this._slots[slot] !== worker) {
        return;
      }
      // A worker that died on the way up is not a crash to recover from, it is
      // a configuration that does not work, and refilling the slot as fast as
      // the loop allows would spin a core until someone noticed. Back off, but
      // never give up: the binary may yet appear, and a pool that stopped
      // trying would have to be restarted by hand.
      //
      // "On the way up" is answered nothing and did not last a second, in that
      // order. Age alone would misread the ordinary case this design exists
      // for -- a worker killed mid-request seconds after the pool started --
      // as a startup failure, and delay the slot that the retry needs.
      const started = worker.served > 0 ||
          (Date.now() - startedAt) >= FAST_FAILURE_MS;
      if (started) {
        this._failures[slot] = 0;
      }
      const failures = this._failures[slot] || 0;
      const delay = started ?
          0 :
          Math.min(MAX_RESPAWN_DELAY_MS, RESPAWN_DELAY_MS * 2 ** failures);
      this._failures[slot] = failures + 1;
      const refill = () => {
        if (this._stopping || this._slots[slot] !== worker) {
          return;
        }
        this._slots[slot] = this._spawn(slot);
        this.emit('worker-restart',
                  {worker: this._slots[slot].id, reason: 'exit', delay});
        this._pump();
      };
      if (delay === 0) {
        refill();
        return;
      }
      const timer = setTimeout(refill, delay);
      // An unref'd timer does not hold the process open: a pool whose workers
      // all failed should not be the reason a program refuses to exit.
      timer.unref();
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

module.exports = {Pool, defaultCacheDir};
