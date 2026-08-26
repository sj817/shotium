import {spawn} from 'node:child_process';
import type {ChildProcess, StdioOptions} from 'node:child_process';
import {EventEmitter} from 'node:events';

import {FrameReader, encodeRequest} from './protocol.js';
import type {WireRequest} from './request.js';

export interface WorkerOptions {
  id: number;
  binary: string;
  args?: string[];
}

// The header frame the worker answers with, followed by the image frame.
export interface ResponseHeader {
  ok: boolean;
  error?: string;
  bytes?: number;
  path?: string;
}

export interface WorkerResult {
  header: ResponseHeader;
  image: Buffer|null;
}

interface Pending {
  resolve: (result: WorkerResult) => void;
  reject: (error: Error) => void;
}

// One shotium.exe --serve process.
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
  readonly id: number;
  // How many requests this process has answered, either way. The pool reads
  // it to tell a worker that was working and then died from one that never
  // came up at all.
  served = 0;

  private readonly binary: string;
  private readonly args: string[];
  private process: ChildProcess|null = null;
  private pending: Pending|null = null;
  private stopping = false;
  private reader = new FrameReader();
  private header: ResponseHeader|null = null;
  private stderr = '';

  constructor(options: WorkerOptions) {
    super();
    this.id = options.id;
    this.binary = options.binary;
    this.args = options.args || [];
    this.start();
  }

  get busy(): boolean {
    return this.pending !== null;
  }

  get alive(): boolean {
    return this.process !== null && this.process.exitCode === null &&
        !this.stopping;
  }

  private start(): void {
    const stdio: StdioOptions = ['pipe', 'pipe', 'pipe'];
    const child = spawn(this.binary, ['--serve', ...this.args], {
      stdio,
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
    this.process = child;

    child.stdout?.on('data', (chunk: Buffer) => this.onStdout(chunk));
    child.stderr?.on('data', (chunk: Buffer) => this.onStderr(chunk));
    child.on('error', (error) => this.onGone(null, null, error));
    child.on('exit', (code, signal) => this.onGone(code, signal, null));

    // The process is up as soon as spawn resolves the executable; there is no
    // handshake in the protocol, and adding one would only move the failure --
    // a binary that cannot start fails the first request just as visibly.
    this.emit('ready');
  }

  private onStdout(chunk: Buffer): void {
    this.reader.push(chunk);
    for (;;) {
      const frame = this.reader.next();
      if (frame === null) {
        return;
      }
      if (this.header === null) {
        // Frame one of two: the JSON header.
        try {
          this.header = JSON.parse(frame.toString('utf8')) as ResponseHeader;
        } catch {
          this.fail(new Error(
              `shotium: worker ${this.id} sent a header that is not JSON`));
          return;
        }
        continue;
      }
      // Frame two: the image, empty when the header reported a failure or when
      // the worker was asked to write the file itself.
      const header = this.header;
      this.header = null;
      this.settle(header, frame);
    }
  }

  private onStderr(chunk: Buffer): void {
    this.stderr += chunk.toString('utf8');
    const lines = this.stderr.split(/\r?\n/);
    this.stderr = lines.pop() ?? '';
    for (const line of lines) {
      if (line.length > 0) {
        this.emit('stderr', {worker: this.id, line});
      }
    }
  }

  private onGone(
      code: number|null, signal: NodeJS.Signals|null,
      error: Error|null): void {
    const wasOwed = this.pending !== null;
    this.process = null;
    if (wasOwed) {
      // A worker that dies mid-request is indistinguishable from one that never
      // answered, which is the point: the retry path does not have to tell a
      // crash from a hang.
      this.fail(
          error ||
          new Error(`shotium: worker ${this.id} exited (code ${code}, signal ${
              signal}) with a request in flight`));
      this.emit('crash', {worker: this.id, code, signal});
    } else if (error) {
      this.emit('error', error);
    }
    this.emit('exit', {worker: this.id, code, signal});
  }

  private settle(header: ResponseHeader, payload: Buffer): void {
    const pending = this.pending;
    if (!pending) {
      return;
    }
    this.pending = null;
    this.served += 1;
    if (header.ok) {
      pending.resolve({header, image: header.path ? null : payload});
    } else {
      pending.reject(new Error(header.error || 'shotium: request failed'));
    }
  }

  private fail(error: Error): void {
    const pending = this.pending;
    if (!pending) {
      return;
    }
    this.pending = null;
    pending.reject(error);
  }

  // Sends one request. Rejects if the worker dies before answering; there is no
  // timeout here, because the deadline belongs to whoever owns the retry
  // policy.
  send(request: WireRequest): Promise<WorkerResult> {
    if (this.pending) {
      return Promise.reject(
          new Error(`shotium: worker ${this.id} is already busy`));
    }
    if (!this.alive) {
      return Promise.reject(new Error(`shotium: worker ${this.id} is not up`));
    }
    return new Promise<WorkerResult>((resolve, reject) => {
      this.pending = {resolve, reject};
      this.process?.stdin?.write(encodeRequest(request), (error) => {
        if (error) {
          this.fail(error);
        }
      });
    });
  }

  // Closing stdin is the shutdown message: the worker sees the frame stream end
  // on a frame boundary and exits 0. kill() is for when it has stopped
  // listening.
  stop(): void {
    this.stopping = true;
    this.process?.stdin?.end();
  }

  kill(): void {
    this.stopping = true;
    this.process?.kill();
  }
}

export {Worker};
