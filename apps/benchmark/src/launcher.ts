import path from 'node:path';
import {createRequire} from 'node:module';
import {pathToFileURL} from 'node:url';
import {execaNode} from 'execa';
import {booleanArg, parseArgs, recoverNpmRunValues} from './args.ts';
import {APP_ROOT} from './constants.ts';
import {ensureShotium} from './install-target.ts';

const require = createRequire(import.meta.url);
const tsxCli = require.resolve('tsx/cli');
const coreCli = path.join(APP_ROOT, 'src', 'cli.ts');

export async function prepareCoreArgs(argv: string[]): Promise<string[]> {
  const options = parseArgs(argv, {shotiumVersion: 'latest', skipInstall: false});
  recoverNpmRunValues(options, ['shotiumVersion', 'skipInstall']);
  const skipInstall = booleanArg(options.skipInstall, false);
  const version = skipInstall ? String(options.shotiumVersion) :
    await ensureShotium(options.shotiumVersion);
  return [...argv, `--shotium-version=${version}`, '--skip-install=true'];
}

export async function run(argv = process.argv.slice(2)): Promise<number> {
  const coreArgs = await prepareCoreArgs(argv);
  const result = await execaNode(tsxCli, [coreCli, ...coreArgs], {
    cwd: APP_ROOT,
    stdio: 'inherit',
    reject: false,
  });
  return result.exitCode ?? 1;
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  run().then((exitCode) => {
    process.exitCode = exitCode;
  }).catch((error) => {
    process.stderr.write(`${error?.stack || error}\n`);
    process.exitCode = 1;
  });
}
