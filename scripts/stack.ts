// Run shotium.exe under cdb and print the crashing stack.
//
// The build sets symbol_level = 0, which does not mean "no symbols": lld still
// writes a PDB with public symbols, so every frame comes back with a function
// name and an offset. That is enough to say which function faulted and who
// called it, which is all a startup crash usually needs -- and it costs
// nothing, where a symbol_level = 1 build costs a full recompile of the tree.
//
//   pnpm stack
//   pnpm stack --frames 60 --page shot/testdata/demos/foo.html
//
// Windows only: cdb is the Windows SDK's debugger. On Linux read the stack with
// gdb (see CLAUDE.md, "The build layer"). Relative paths are resolved against
// the repository root.

import {existsSync} from 'node:fs';

import {cac} from 'cac';
import {execa} from 'execa';

import {resolve, root} from './lib/repo.ts';

const CDB = 'C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\cdb.exe';

async function main(opts: {page: string; width: number; height: number; output: string; frames: number}): Promise<number> {
  if (!existsSync(CDB)) {
    console.log(`no cdb at ${CDB} -- install the Windows SDK debugging tools`);
    return 1;
  }
  // -o follows child processes, -g/-G skip the initial and final breakpoints
  // so the only stop is the fault itself. .symfix points at the public symbol
  // server for the system DLLs; shotium.exe's own PDB is found next to the
  // binary.
  const script = `.symfix;.reload;g;kb ${opts.frames};q`;
  const result = await execa(CDB, [
    '-o', '-g', '-G', '-c', script, resolve('out/Shot/shotium.exe'),
    '--file', resolve(opts.page), '--width', String(opts.width), '--height', String(opts.height), '--output', resolve(opts.output),
  ], {cwd: root, reject: false, all: true});
  const interesting = /shot!|ntdll!|KERNELBASE!|Access violation|second chance|FATAL|Check failed/;
  const lines = (result.all ?? '').split(/\r?\n/).filter((l) => interesting.test(l)).slice(0, opts.frames + 10);
  console.log(lines.join('\n'));
  return 0;
}

const cli = cac('stack');
cli.command('', 'run shotium.exe under cdb and print the crashing stack')
    .option('--page <file>', 'the document to render', {default: 'shot/testdata/render_corpus.html'})
    .option('--width <px>', 'viewport width', {default: 1248})
    .option('--height <px>', 'viewport height', {default: 1320})
    .option('--output <file>', 'where the render goes', {default: 'shot/testdata/out/shot.png'})
    .option('--frames <n>', 'stack frames to print', {default: 40})
    .action(async (options: {page: string; width: number; height: number; output: string; frames: number}) => {
      process.exitCode = await main({...options, width: Number(options.width), height: Number(options.height), frames: Number(options.frames)});
    });
cli.help();
cli.parse();
