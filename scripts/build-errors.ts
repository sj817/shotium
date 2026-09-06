// Summarise a ninja log into error classes rather than error lines.
//
// Reading the raw log costs more than the information in it. What actually
// drives the next edit is: which diagnostic, how many times, and in which
// files. build-engine prints the class view after every build through
// lib/ninja-log.ts; this is the same view on demand, plus the per-edge one.
//
//   pnpm build:errors out/Shot/build.log                 # classes, commonest first
//   pnpm build:errors out/Shot/build.log --limit 10 --files
//   pnpm build:errors out/Shot/build.log --targets       # one entry per failed edge
//   pnpm build:errors out/Shot/build.log --targets --full
//   pnpm build:errors out/Shot/build.log --top 30        # kinds, quoted parts collapsed
//
// A relative log path is resolved against the repository root.

import {readFileSync} from 'node:fs';

import {cac} from 'cac';

import {formatClasses, formatFailures, formatKinds} from './lib/ninja-log.ts';
import {resolve} from './lib/repo.ts';

const cli = cac('build-errors');
cli.command('<log>', 'reduce a ninja log to what went wrong')
    .option('--limit <n>', 'classes to print', {default: 40})
    .option('--files', 'list the files under each class')
    .option('--targets', 'one entry per failed edge, then a count per target')
    .option('--full', 'with --targets: every diagnostic line, not just the first')
    .option('--top <n>', 'group failures by kind and print the commonest')
    .action((logArg: string, options: {limit: number; files?: boolean; targets?: boolean; full?: boolean; top?: number}) => {
      const log = readFileSync(resolve(logArg), 'utf8');
      if (options.top) {
        const out = formatKinds(log, Number(options.top));
        console.log(out || `no failures in ${logArg}`);
      } else if (options.targets) {
        const out = formatFailures(log, {full: options.full === true});
        console.log(out || `no failures in ${logArg}`);
      } else {
        console.log(formatClasses(log, {limit: Number(options.limit), files: options.files === true}));
      }
    });
cli.help();
cli.parse();
