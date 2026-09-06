// Run a git command, retrying while another process holds .git/index.lock.
//
// An external `git status --porcelain` poller (GitHub Desktop, an editor)
// takes the index lock every few seconds, so any staging command in this
// repository fails at random. `git fsmonitor--daemon` also shows up in the
// process list but does NOT hold the lock -- killing it is never the fix and
// costs a full rescan.
//
// A lock is only ever removed when both are true: no live git process other
// than fsmonitor--daemon, and the lock's mtime is not advancing. Size is not a
// criterion -- a 44 MB index.lock left by a killed `git add` is abandoned, and
// a 0-byte one held by a live process is not.
//
//   pnpm git-retry add -- path/one path/two
//   pnpm git-retry commit -m "msg"

import {existsSync, rmSync, statSync} from 'node:fs';
import path from 'node:path';

import {execa} from 'execa';
import pRetry, {AbortError} from 'p-retry';

import {root, sleep} from './lib/repo.ts';

// Command lines of the live git processes. Node has no API for another
// process's command line, so the platform's own query is used: CIM on
// Windows, ps elsewhere.
async function liveGitCommandLines(): Promise<string[]> {
  if (process.platform === 'win32') {
    const {stdout} = await execa('powershell', [
      '-NoProfile', '-Command',
      "Get-CimInstance Win32_Process -Filter \"Name='git.exe'\" | ForEach-Object { $_.CommandLine }",
    ], {reject: false});
    return stdout.split(/\r?\n/).map((l) => l.trim()).filter(Boolean);
  }
  const {stdout} = await execa('ps', ['-eo', 'args'], {reject: false});
  return stdout.split('\n').filter((l) => /(^|\/)git(\s|$)/.test(l));
}

async function lockAbandoned(lock: string): Promise<boolean> {
  if (!existsSync(lock)) return false;
  const live = (await liveGitCommandLines()).filter((l) => !l.includes('fsmonitor--daemon'));
  if (live.length) return false;
  const before = statSync(lock).mtimeMs;
  await sleep(700);
  if (!existsSync(lock)) return false;
  return statSync(lock).mtimeMs === before;
}

async function main(args: string[]): Promise<number> {
  const {stdout: gitDir} = await execa('git', ['rev-parse', '--git-dir'], {cwd: root});
  const lock = path.resolve(root, gitDir.trim(), 'index.lock');
  try {
    return await pRetry(async () => {
      const result = await execa('git', args, {cwd: root, reject: false, all: true});
      if (result.exitCode === 0) {
        if (result.all) console.log(result.all);
        return 0;
      }
      if (!/index\.lock/.test(result.all ?? '')) {
        if (result.all) console.log(result.all);
        throw new AbortError(String(result.exitCode));
      }
      if ((await lockAbandoned(lock)) && (await lockAbandoned(lock))) {
        console.log('index.lock looks abandoned (no live git, mtime frozen); removing');
        rmSync(lock, {force: true});
        // A removed lock can leave the index mid-write; make git re-read it.
        await execa('git', ['reset', '-q'], {cwd: root, reject: false});
      }
      throw new Error(result.all ?? 'index.lock');
    }, {retries: 59, factor: 1, minTimeout: 400, maxTimeout: 400});
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    if (/^\d+$/.test(message)) return Number(message);
    console.error(`gave up after 60 attempts: ${message}`);
    return 1;
  }
}

const args = process.argv.slice(2);
if (args.length === 0) {
  console.log('usage: pnpm git-retry <git arguments>');
  process.exitCode = 2;
} else {
  process.exitCode = await main(args);
}
