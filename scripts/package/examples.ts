// Ship each language demo as one self-contained ZIP and verify it after
// extraction. The ZIP is the language directory as git tracks it -- README in
// both languages, card.html, sources and lock files -- plus the C ABI header,
// guide and licence, so a reader never needs the repository. Taking the file
// list from git rather than a hand-written whitelist keeps build outputs,
// dependencies and engines out without a list to forget to update.
import {createHash} from 'node:crypto';
import {copyFileSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync} from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {cac} from 'cac';
import {execa} from 'execa';
import {globSync} from 'tinyglobby';
import {resolve} from '../lib/repo.ts';

const LANGUAGES = ['go', 'python', 'rust', 'csharp', 'java'] as const;
const SHARED: Array<[string, string]> = [
  ['LICENSE', 'LICENSE'],
  ['shot/shot_api.h', 'shot_api.h'],
  ['apps/c-abi/README.md', 'C_ABI.md'],
  ['apps/c-abi/README.zh.md', 'C_ABI.zh.md'],
];

const hash = (bytes: Buffer) => createHash('sha256').update(bytes).digest('hex');

async function git(args: string[]): Promise<string> {
  const {stdout} = await execa('git', ['--no-optional-locks', ...args], {cwd: resolve()});
  return stdout;
}

async function packageLanguage(destination: string, sevenzip: string, language: string): Promise<void> {
  const {version} = JSON.parse(readFileSync(resolve('apps/typescript/package.json'), 'utf8')) as {version: string};
  if (!/^\d+\.\d+\.\d+(?:-[\w.-]+)?$/.test(version)) throw new Error('Invalid package version');
  const directory = `apps/${language}`;
  const tracked = (await git(['ls-files', '--', directory])).split(/\r?\n/).filter(Boolean);
  if (!tracked.includes(`${directory}/card.html`)) throw new Error(`${directory}/card.html is not tracked; run pnpm demo:sync`);
  const files: Array<[string, string]> = [...tracked.map((file): [string, string] => [file, file.slice(directory.length + 1)]), ...SHARED];
  const name = `shotium-${language}-example-v${version}`;
  const temporary = mkdtempSync(path.join(os.tmpdir(), 'shotium-examples-'));
  try {
    const stage = path.join(temporary, name);
    const expected = new Map<string, string>();
    const put = (relative: string, bytes: Buffer) => {
      const target = path.join(stage, relative);
      mkdirSync(path.dirname(target), {recursive: true});
      writeFileSync(target, bytes);
      expected.set(relative, hash(bytes));
    };
    for (const [source, relative] of files) put(relative, readFileSync(resolve(source)));
    const commit = (await git(['rev-parse', 'HEAD'])).trim();
    const changes = await git(['status', '--porcelain', '--', ...files.map(([source]) => source), 'scripts/package/examples.ts', 'apps/typescript/package.json']);
    put('manifest.json', Buffer.from(JSON.stringify({version, language, commit, dirty: changes.length > 0, files: Object.fromEntries(expected)}, null, 2) + '\n'));
    const archive = path.join(temporary, `${name}.zip`);
    await execa(sevenzip, ['a', '-tzip', '-mx=9', archive, name], {cwd: temporary});
    const extracted = path.join(temporary, 'verified');
    await execa(sevenzip, ['x', archive, `-o${extracted}`, '-y']);
    const root = path.join(extracted, name);
    const actual = globSync('**/*', {cwd: root, onlyFiles: true, dot: true}).map(file => file.replaceAll('\\', '/'));
    if (actual.length !== expected.size) throw new Error('ZIP file list differs from manifest');
    for (const file of actual) {
      if (hash(readFileSync(path.join(root, file))) !== expected.get(file)) throw new Error(`ZIP content mismatch: ${file}`);
    }
    const output = resolve(destination);
    mkdirSync(output, {recursive: true});
    const target = path.join(output, `${name}.zip`);
    copyFileSync(archive, target);
    writeFileSync(`${target}.sha256`, `${hash(readFileSync(target))}  ${name}.zip\n`);
    console.log(`Verified ${expected.size} files: ${target}`);
  } finally {
    rmSync(temporary, {recursive: true, force: true});
  }
}

const cli = cac('pnpm package:examples');
cli.command('', 'package and verify one source ZIP per language; paths resolve from the repository root')
    .option('--dest <dir>', 'output directory', {default: 'out/examples'})
    .option('--sevenzip <command>', '7-Zip executable', {default: '7z'})
    .action(async (options: {dest: string; sevenzip: string}) => {
      try { for (const language of LANGUAGES) await packageLanguage(options.dest, options.sevenzip, language); }
      catch (error) { console.error(error); process.exitCode = 1; }
    });
cli.help();
cli.parse();
