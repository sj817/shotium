// A client for `shotium --serve`, for the check suites.
//
// The framing is the package's own (shotium/src/lib/protocol.ts): a 4-byte
// little-endian length and that many bytes, in both directions. Importing it
// rather than restating it is the point -- the Python suites each carried
// their own copy of the four-byte header, and three copies of a wire format
// are three places it can be wrong.
//
// A response is a JSON header frame followed by one payload frame; a tiles
// response is a header followed by one frame per tile it lists, and a failed
// one by a single empty frame, so the stream stays in step either way.

import {execa} from 'execa';

type Subprocess = ReturnType<typeof execa>;

import {encodeRequest, FrameReader} from '../../shotium/src/lib/protocol.ts';

export interface Header {
  ok?: boolean;
  error?: string;
  bytes?: number;
  path?: string;
  tiles?: Array<{y: number; height: number; bytes: number; path?: string}>;
  [key: string]: unknown;
}

export class ServeWorker {
  private readonly proc: Subprocess;
  private readonly reader = new FrameReader();
  private readonly frames: Buffer[] = [];
  private readonly waiting: Array<{resolve: (f: Buffer) => void; reject: (e: Error) => void}> = [];
  private ended: Error | null = null;

  constructor(exe: string, args: string[] = [], env?: NodeJS.ProcessEnv) {
    this.proc = execa(exe, ['--serve', ...args], {
      stdin: 'pipe',
      stdout: 'pipe',
      stderr: 'inherit',
      env,
      extendEnv: env === undefined,
      buffer: false,
      reject: false,
    });
    this.proc.stdout!.on('data', (chunk: Buffer) => {
      this.reader.push(chunk);
      for (let frame = this.reader.next(); frame !== null; frame = this.reader.next()) {
        const waiter = this.waiting.shift();
        if (waiter) waiter.resolve(Buffer.from(frame));
        else this.frames.push(Buffer.from(frame));
      }
    });
    this.proc.stdout!.on('end', () => {
      this.ended = new Error('worker closed the stream mid-frame');
      for (const w of this.waiting.splice(0)) w.reject(this.ended);
    });
  }

  send(request: unknown): void {
    this.proc.stdin!.write(encodeRequest(request));
  }

  frame(): Promise<Buffer> {
    const ready = this.frames.shift();
    if (ready) return Promise.resolve(ready);
    if (this.ended) return Promise.reject(this.ended);
    return new Promise((resolve, reject) => this.waiting.push({resolve, reject}));
  }

  async recv(): Promise<[Header, Buffer]> {
    const header = JSON.parse((await this.frame()).toString('utf8')) as Header;
    return [header, await this.frame()];
  }

  async recvTiles(): Promise<[Header, Buffer[]]> {
    const header = JSON.parse((await this.frame()).toString('utf8')) as Header;
    const count = header.ok ? (header.tiles ?? []).length : 1;
    const tiles: Buffer[] = [];
    for (let i = 0; i < count; i++) tiles.push(await this.frame());
    return [header, tiles];
  }

  async ask(request: unknown): Promise<[Header, Buffer]> {
    this.send(request);
    return this.recv();
  }

  // Closes stdin and waits for the exit code; -1 if it did not exit in time.
  async close(timeoutMs = 60_000): Promise<number> {
    this.proc.stdin!.end();
    const timer = new Promise<number>((resolve) => setTimeout(() => {
      this.proc.kill();
      resolve(-1);
    }, timeoutMs).unref());
    const exited = this.proc.then((r: {exitCode?: number}) => r.exitCode ?? -1);
    return Promise.race([exited, timer]);
  }
}
