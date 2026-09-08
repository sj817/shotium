import {createHash} from 'node:crypto';
import {closeSync, existsSync, mkdirSync, openSync, readFileSync, readSync, statSync, writeFileSync} from 'node:fs';
import path from 'node:path';

interface Entry {hash: string; time: number}

// This state travels with the objects it describes, inside the build cache.
export class InputMtimes {
  private previous: Record<string, Entry> = {};
  private current: Record<string, Entry> = {};
  private buffer = Buffer.allocUnsafe(1024 * 1024);
  private changedTime: number;
  private hasState: boolean;
  private stateFile: string;
  private fallbackTime: number;

  constructor(stateFile: string, fallbackTime: number, headTime: number) {
    this.stateFile = stateFile;
    this.fallbackTime = fallbackTime;
    this.hasState = existsSync(stateFile);
    if (this.hasState) {
      const state = JSON.parse(readFileSync(stateFile, 'utf8')) as {version: number; files: Record<string, Entry>};
      if (state.version !== 1) throw new Error('Unsupported input timestamp state');
      this.previous = state.files;
    }
    const log = path.join(path.dirname(stateFile), '.ninja_log');
    // A changed input must be newer than cached outputs, even when switching
    // to an older commit. All shards restoring this cache derive the same time.
    this.changedTime = Math.max(headTime, existsSync(log) ? statSync(log).mtimeMs / 1000 + 1 : headTime);
  }

  time(file: string, key: string): number {
    const digest = createHash('sha256');
    const fd = openSync(file, 'r');
    try {
      for (;;) {
        const length = readSync(fd, this.buffer, 0, this.buffer.length, null);
        if (!length) break;
        digest.update(this.buffer.subarray(0, length));
      }
    } finally { closeSync(fd); }
    const hash = digest.digest('hex');
    const previous = this.previous[key];
    const time = previous?.hash === hash ? previous.time
      : previous ? Math.max(this.changedTime, previous.time + 1)
      : this.hasState ? this.changedTime : this.fallbackTime;
    this.current[key] = {hash, time};
    return time;
  }

  save(): void {
    mkdirSync(path.dirname(this.stateFile), {recursive: true});
    writeFileSync(this.stateFile, JSON.stringify({version: 1, files: this.current}));
  }
}
