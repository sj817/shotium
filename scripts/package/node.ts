// Stage the Node addon and its runtime resources, without a version.
//
// The engine job archives this directory as shotium-node-<platform>.7z and
// uploads it under the engine's fingerprint. Whoever ships it later --
// publish.yml, a pull-request preview, check-ffi -- turns it into the
// versioned @pixel.js/shotium-<os>-<arch> package with
// `pnpm package:platform --from-archive`, so a version bump never needs
// the engine rebuilt.
import {cac} from 'cac';
import {resolve} from '../lib/repo.ts';
import {stageNativeDelivery, verifyNativeDelivery, type NativeOS} from '../lib/release-artifacts.ts';

const cli = cac('pnpm package:node');
cli.command('', 'stage a Node addon delivery directory; paths are relative to the repository root')
    .option('--build <dir>', 'engine outputs', {default: 'out/Shot'})
    .option('--dest <dir>', 'empty destination directory (required)')
    .option('--check', 'verify an extracted delivery instead of staging files')
    .option('--os <name>', 'win, linux or mac (required)')
    .action(async (options: {build: string; dest?: string; os?: NativeOS; check?: boolean}) => {
      try {
        if (!options.dest || !options.os) throw new Error('--dest and --os are required');
        const dest = resolve(options.dest);
        if (options.check) {
          await verifyNativeDelivery('node', options.os, dest);
          console.log('Verified node delivery: ' + dest);
          return;
        }
        const files = await stageNativeDelivery({kind: 'node', os: options.os, build: resolve(options.build), dest});
        console.log('Node delivery: ' + dest + '\n' + files.join('\n'));
      } catch (error) {
        console.error(error);
        process.exitCode = 1;
      }
    });
cli.help();
cli.parse();
