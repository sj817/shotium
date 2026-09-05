// Delete depfiles in a build directory that name files which no longer exist.
//
// ninja reads an action's depfile (`depfile = x.d`) from disk and treats every
// path in it as an implicit input. When the tree loses a file that such a
// depfile recorded -- a .grd that grit's resource-id allocator opened, a .mojom
// a generator imported -- a warm build directory fails with
//
//   ninja: error: '../../x', needed by 'gen/y', missing and no known rule to make it
//
// before a single step runs, and the only fix is to remove the stale depfile so
// the action reruns and writes a fresh one. A cold directory never has the
// problem, which is why it surfaces only in CI's cached builds and on the
// development host after a large deletion. Compiler dependencies are not
// affected: they live in ninja's deps log (`deps = msvc|gcc`), and ninja marks
// a missing header as dirty instead of refusing.
//
//   pnpm depfiles:prune [out/Shot] [--dry-run]

import {existsSync} from 'node:fs';
import {readFile, unlink} from 'node:fs/promises';
import path from 'node:path';

import {cac} from 'cac';
import pc from 'picocolors';
import {glob} from 'tinyglobby';

// pnpm runs this with cwd = scripts; paths are resolved against the repository.
const root = path.resolve(import.meta.dirname, '..');
const resolve = (p: string) => path.resolve(root, p);

import {parseDepfile} from './lib/depfile.ts';

const dependencies = (text: string) => parseDepfile(text).deps;

export async function pruneStaleDepfiles(buildDir: string, dryRun = false): Promise<{scanned: number; removed: string[]}> {
  const files = await glob('**/*.d', {cwd: buildDir, absolute: true, ignore: ['**/node_modules/**']});
  const removed: string[] = [];
  for (const file of files) {
    let text: string;
    try {
      text = await readFile(file, 'utf8');
    } catch {
      continue;
    }
    const missing = dependencies(text).filter((dep) => !existsSync(path.resolve(buildDir, dep)));
    if (missing.length === 0) continue;
    removed.push(path.relative(buildDir, file));
    console.log(`${dryRun ? 'would remove' : 'removed'} ${path.relative(root, file)}: ${missing.length} missing (${missing[0]}${missing.length > 1 ? ', ...' : ''})`);
    if (!dryRun) await unlink(file);
  }
  return {scanned: files.length, removed};
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const argv = process.argv.slice(2);
  const dryRun = argv.includes('--dry-run');
  const cli = cac('prune-stale-depfiles');
  cli.command('[out]', 'delete depfiles that name files missing from the tree')
      .option('--dry-run', 'list, do not delete')
      .action(async (out: string | undefined) => {
        const buildDir = resolve(out ?? 'out/Shot');
        if (!existsSync(path.join(buildDir, 'build.ninja'))) {
          console.error(`${buildDir} has no build.ninja`);
          process.exitCode = 1;
          return;
        }
        const {scanned, removed} = await pruneStaleDepfiles(buildDir, dryRun);
        console.log(`${scanned} depfiles scanned, ${removed.length} ${dryRun ? 'stale' : 'removed'}${removed.length ? '' : pc.green(' -- nothing stale')}`);
      });
  cli.help();
  cli.parse([...process.argv.slice(0, 2), ...argv.filter((a) => a !== '--dry-run')]);
}
