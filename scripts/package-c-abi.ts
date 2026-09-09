// Stage the public C ABI delivery alongside the CLI, from one build directory.
// The header and guide must travel with the binary rather than require a checkout.
import {copyFileSync, existsSync, mkdirSync} from 'node:fs';
import path from 'node:path';
import {cac} from 'cac';
import {resolve} from './lib/repo.ts';

const cli = cac('package-c-abi');
cli.command('', 'stage a C ABI Release directory; paths are relative to the repository root')
    .option('--build <dir>', 'engine outputs', {default: 'out/Shot'})
    .option('--dest <dir>', 'destination directory (required)')
    .option('--os <name>', 'win, linux or mac (required)')
    .action((options: {build: string; dest?: string; os?: string}) => {
      try {
        if (!options.dest || !['win', 'linux', 'mac'].includes(options.os ?? '')) throw new Error('--dest and --os win|linux|mac are required');
        const names = options.os === 'win' ? ['shotium.exe', 'shotium.dll', 'shotium.dll.lib'] :
          ['shotium', options.os === 'mac' ? 'libshotium.dylib' : 'libshotium.so'];
        const files = [...names, 'shotium_data.pak', 'shotium_strings.pak'].map(name => [resolve(options.build, name), name]);
        files.push([resolve('shot/shot_api.h'), 'shot_api.h'], [resolve('apps/c-abi/README.md'), 'C_ABI.md'], [resolve('LICENSE'), 'LICENSE']);
        for (const [source] of files) if (!existsSync(source!)) throw new Error(`missing C ABI delivery input: ${source}`);
        const destination = resolve(options.dest);
        mkdirSync(destination, {recursive: true});
        for (const [source, name] of files) copyFileSync(source!, path.join(destination, name!));
        console.log(`C ABI delivery: ${destination}\n${files.map(([, name]) => name).join('\n')}`);
      } catch (error) {
        console.error(error);
        process.exitCode = 1;
      }
    });
cli.help();
cli.parse();
