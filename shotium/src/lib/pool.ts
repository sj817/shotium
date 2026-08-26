import {EventEmitter} from 'node:events';
import fs from 'node:fs';
import path from 'node:path';

import type {ResolvedStartOptions} from './config.js';
import {defaultCacheDir} from './config.js';
import type {WireRequest} from './request.js';
import type {WorkerResult} from './worker.js';
import {Worker} from './worker.js';

// A worker that exits sooner than this never really started, so its slot is
// refilled on a doubling delay rather than immediately.
const FAST_FAILURE_MS = 1000;
const RESPAWN_DELAY_MS = 100;
const MAX_RESPAWN_DELAY_MS = 5000;

export interface SubmitOptions {
  /** The supervisor's deadline, in milliseconds. */
  timeout: number;
  /** How many times to re-send after a crash or a timeout. */
  retry: number;
}

interface Job {
  request: WireRequest;
  timeout: number;
  attemptsLeft: number;
  resolve: (result: WorkerResult) => void;
  reject: (error: Error) => void;
}

// A fixed set of shotium.exe --serve processes, and a queue in front of them.
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
  private readonly binary: string;
  private readonly size: number;
  private readonly args: string[];
  private readonly cacheDir: string|null;
  private slots: Worker[] = [];
  private failures: number[] = [];
  private queue: Job[] = [];
  private stopping = false;
  private nextId = 0;

  constructor(options: ResolvedStartOptions) {
    super();
    this.binary = options.binary;
    this.size = options.workers;
    this.args = options.args || [];
    this.cacheDir = options.cacheDir || null;
  }

  start(): void {
    if (this.slots.length > 0) {
      return;
    }
    for (let slot = 0; slot < this.size; ++slot) {
      this.slots[slot] = this.spawn(slot);
    }
    this.emit('ready', {workers: this.size});
  }

  private spawn(slot: number): Worker {
    const id = this.nextId++;
    const args = [...this.args];
    if (this.cacheDir) {
      // One directory per slot, not per process: the Simple backend takes an
      // exclusive lock on its directory, so sharing one would leave every
      // worker but the first running uncached. Keying on the slot rather than
      // the worker id means a restarted worker inherits the warm cache its
      // predecessor built.
      const dir = path.join(this.cacheDir, `worker-${slot}`);
      fs.mkdirSync(dir, {recursive: true});
      args.push(`--cache-dir=${dir}`);
    }

    const startedAt = Date.now();
    const worker = new Worker({id, binary: this.binary, args});
    worker.on('stderr', (event) => this.emit('stderr', event));
    worker.on('crash', (event) => this.emit('crash', event));
    // A worker that could not be started at all -- a binary that is not there,
    // a path that is not executable -- reports it here. Without a listener
    // EventEmitter throws the error instead, which for a resident daemon means
    // a typo in a path takes the whole pool down.
    worker.on('error', (error) => this.emit('worker-error', {worker: id, error}));
    worker.on('exit', (event) => {
      this.emit('exit', event);
      if (this.stopping || this.slots[slot] !== worker) {
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
        this.failures[slot] = 0;
      }
      const failures = this.failures[slot] || 0;
      const delay = started ?
          0 :
          Math.min(MAX_RESPAWN_DELAY_MS, RESPAWN_DELAY_MS * 2 ** failures);
      this.failures[slot] = failures + 1;
      const refill = () => {
        if (this.stopping || this.slots[slot] !== worker) {
          return;
        }
        const replacement = this.spawn(slot);
        this.slots[slot] = replacement;
        this.emit('worker-restart',
                  {worker: replacement.id, reason: 'exit', delay});
        this.pump();
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
  submit(request: WireRequest, {timeout, retry}: SubmitOptions):
      Promise<WorkerResult> {
    return new Promise<WorkerResult>((resolve, reject) => {
      this.queue.push({
        request,
        timeout,
        attemptsLeft: Math.max(0, retry) + 1,
        resolve,
        reject,
      });
      this.pump();
    });
  }

  private pump(): void {
    while (this.queue.length > 0) {
      const slot = this.slots.findIndex((w) => w && w.alive && !w.busy);
      if (slot < 0) {
        return;
      }
      this.dispatch(this.slots[slot]!, this.queue.shift()!);
    }
  }

  private dispatch(worker: Worker, job: Job): void {
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
      this.retryOrFail(
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
          this.pump();
        })
        .catch((error: Error) => {
          if (settled) {
            return;
          }
          settled = true;
          clearTimeout(timer);
          this.retryOrFail(job, error);
        });
  }

  private retryOrFail(job: Job, error: Error): void {
    // A request rejected on its own merits -- a bad selector, an unreadable
    // file -- would fail the same way every time, but the worker also rejects
    // with the same shape when it dies. Retrying both is the safe direction:
    // the cost of a pointless retry is one more render, and the cost of not
    // retrying a crash is a failure the caller cannot do anything about.
    if (job.attemptsLeft > 0 && !this.stopping) {
      this.queue.unshift(job);
      this.pump();
      return;
    }
    job.reject(error);
    this.pump();
  }

  async stop(): Promise<void> {
    this.stopping = true;
    for (const job of this.queue.splice(0)) {
      job.reject(new Error('shotium: the runtime was stopped'));
    }
    await Promise.all(this.slots.map((worker) => new Promise<void>((resolve) => {
      if (!worker || !worker.alive) {
        resolve();
        return;
      }
      worker.once('exit', () => resolve());
      worker.stop();
    })));
    this.slots = [];
  }
}

export {Pool, defaultCacheDir};
