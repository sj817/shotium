// One checksum file for the complete set of public Release archives.
import {cac} from 'cac';
import {resolve} from '../lib/repo.ts';
import {collectNativeArchives, verifyReleaseChecksums, writeReleaseChecksums} from '../lib/release-artifacts.ts';

const cli = cac('pnpm package:checksums');
cli.command('', 'write or verify SHA256SUMS for 17 archives; paths resolve from the repository root')
    .option('--dir <dir>', 'directory containing only the Release attachments (required)')
    .option('--collect <dir>', 'collect the six platform artifact directories before generating the list')
    .option('--check', 'verify the file set and hashes without writing')
    .action(async (options: {dir?: string; check?: boolean; collect?: string}) => {
      try {
        if (!options.dir) throw new Error('--dir is required');
        const directory = resolve(options.dir);
        if (options.collect) {
          if (options.check) throw new Error('--collect cannot be combined with --check');
          await collectNativeArchives(resolve(options.collect), directory);
        }
        if (!options.check) await writeReleaseChecksums(directory);
        await verifyReleaseChecksums(directory);
        console.log('Verified 17 archives and SHA256SUMS: ' + directory);
      } catch (error) {
        console.error(error);
        process.exitCode = 1;
      }
    });
cli.help();
cli.parse();
