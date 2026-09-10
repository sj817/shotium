// Stage only the standalone CLI and its runtime resources.
import {cac} from 'cac';
import {resolve} from '../lib/repo.ts';
import {stageNativeDelivery, verifyNativeDelivery, type NativeOS} from '../lib/release-artifacts.ts';

const cli = cac('pnpm package:cli');
cli.command('', 'stage a CLI Release directory; paths are relative to the repository root')
    .option('--build <dir>', 'engine outputs', {default: 'out/Shot'})
    .option('--dest <dir>', 'empty destination directory (required)')
    .option('--check', 'verify an extracted delivery instead of staging files')
    .option('--os <name>', 'win, linux or mac (required)')
    .action(async (options: {build: string; dest?: string; os?: NativeOS; check?: boolean}) => {
      try {
        if (!options.dest || !options.os) throw new Error('--dest and --os are required');
        const dest = resolve(options.dest);
        if (options.check) {
          await verifyNativeDelivery('cli', options.os, resolve(options.dest));
          console.log('Verified cli delivery: ' + resolve(options.dest));
          return;
        }
        const files = await stageNativeDelivery({kind: 'cli', os: options.os, build: resolve(options.build), dest});
        console.log('CLI delivery: ' + dest + '\n' + files.join('\n'));
      } catch (error) {
        console.error(error);
        process.exitCode = 1;
      }
    });
cli.help();
cli.parse();
