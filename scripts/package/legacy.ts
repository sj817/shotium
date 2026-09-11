// Writes @shotkit/shotium, the package's previous name, as a compatibility
// shim: a manifest that depends on @pixel.js/shotium at exactly this version,
// and an entry that re-exports it.
//
//   pnpm package:legacy --dest dist/legacy/shotium
//
// The package moved to the @pixel.js scope in 0.7.3. An install pinned to the
// old name keeps working and keeps receiving releases because this package is
// published beside every release, with the same version number; what it
// installs is the new package and its platform engine. The six old platform
// packages, @shotkit/shotium-<os>-<arch>, are not published any more: nothing
// depends on them once the main package under that name depends on
// @pixel.js/shotium instead.
//
// Generated at publish time rather than committed. Everything in it is a copy
// of something in apps/typescript/package.json -- the version, the exports
// map, the repository field provenance checks against -- and a second
// checked-in copy of any of those is a copy that drifts. The exports map is
// mirrored by name: the guard below fails on a subpath this file does not
// know how to re-export, so adding one to the real package is a loud failure
// here rather than a shim that quietly lacks it.
//
// This script does not run npm. It writes a directory; publish.yml runs
// `npm publish` on it, after the registry has been seen to serve the package
// it depends on.

import {mkdirSync, readFileSync, rmSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';

import {resolve} from '../lib/repo.ts';

export const PRIMARY = '@pixel.js/shotium';
export const LEGACY = '@shotkit/shotium';

export interface PrimaryManifest {
  name: string;
  version: string;
  keywords?: string[];
  homepage?: string;
  bugs?: string;
  repository?: unknown;
  license?: string;
  engines?: unknown;
  exports: Record<string, unknown>;
}

// The two entries this shim knows how to mirror. "." is the API; the
// manifest is exported so a caller who reads the version of the package
// they installed gets the shim's, which is the same number.
const MIRRORED_EXPORTS = ['.', './package.json'];

export function shimManifest(primary: PrimaryManifest): Record<string, unknown> {
  if (primary.name !== PRIMARY) {
    throw new Error(`the legacy shim mirrors ${PRIMARY}, not ${primary.name}`);
  }
  const exported = Object.keys(primary.exports).sort();
  if (exported.join('\n') !== [...MIRRORED_EXPORTS].sort().join('\n')) {
    throw new Error(
        `${PRIMARY} exports ${exported.join(', ')}; this shim mirrors only ${MIRRORED_EXPORTS.join(', ')}. ` +
        'Teach scripts/package/legacy.ts the new entry before publishing.');
  }
  const manifest: Record<string, unknown> = {
    name: LEGACY,
    version: primary.version,
    description: `Compatibility alias of ${PRIMARY}, the package's current name. Install ${PRIMARY} instead.`,
  };
  if (primary.keywords) manifest.keywords = primary.keywords;
  if (primary.homepage) manifest.homepage = primary.homepage;
  if (primary.bugs) manifest.bugs = primary.bugs;
  // Provenance compares this against the repository the workflow runs in and
  // refuses a mismatch; it is the primary's field, verbatim, for that reason.
  if (primary.repository) manifest.repository = primary.repository;
  Object.assign(manifest, {
    type: 'module',
    main: './index.js',
    types: './index.d.ts',
    exports: {
      '.': {types: './index.d.ts', default: './index.js'},
      './package.json': './package.json',
    },
    files: ['index.js', 'index.d.ts', 'README.md'],
    // Exact, not a range: the two names are one package and move together.
    dependencies: {[PRIMARY]: primary.version},
  });
  if (primary.engines) manifest.engines = primary.engines;
  if (primary.license) manifest.license = primary.license;
  return manifest;
}

// One file serves as both index.js and index.d.ts: `export *` forwards the
// named exports and, in a declaration file, the types; `default` is the one
// thing `export *` never forwards, and the real package has one.
export function shimEntry(): string {
  return `// ${LEGACY} is the previous name of ${PRIMARY}. Everything here is that package.\n` +
      `export * from '${PRIMARY}';\n` +
      `export {default} from '${PRIMARY}';\n`;
}

export function shimReadme(version: string): string {
  const npm = `https://www.npmjs.com/package/${PRIMARY}`;
  return `# ${LEGACY}\n\n` +
      `> **Compatibility alias.** \`${LEGACY}\` is the previous name of [\`${PRIMARY}\`](${npm}). ` +
      `This package depends on \`${PRIMARY}@${version}\` and re-exports it, so an existing install ` +
      `pinned to the old name keeps receiving every release. New projects should install \`${PRIMARY}\` directly.\n` +
      `>\n` +
      `> **兼容别名包。** \`${LEGACY}\` 是 [\`${PRIMARY}\`](${npm}) 的旧包名。本包依赖 \`${PRIMARY}@${version}\` ` +
      `并原样重新导出，已有项目保留旧依赖即可继续收到每一个新版本；新项目请直接安装 \`${PRIMARY}\`。\n\n` +
      `## Migrate / 迁移\n\n` +
      '```bash\n' +
      `npm uninstall ${LEGACY}\n` +
      `npm install ${PRIMARY}\n` +
      '```\n\n' +
      '```diff\n' +
      `- import { screenshot } from '${LEGACY}';\n` +
      `+ import { screenshot } from '${PRIMARY}';\n` +
      '```\n\n' +
      `The six \`${LEGACY}-<os>-<arch>\` platform packages are no longer published; the engine now arrives as ` +
      `\`${PRIMARY}-<os>-<arch>\`, which npm resolves on the next install.\n\n` +
      `旧的六个 \`${LEGACY}-<os>-<arch>\` 平台包不再发布，引擎改由 \`${PRIMARY}-<os>-<arch>\` 提供，下次安装时由 npm 自动完成替换。\n`;
}

export function writeShim(dest: string, primary: PrimaryManifest): string[] {
  const manifest = shimManifest(primary);
  rmSync(dest, {recursive: true, force: true});
  mkdirSync(dest, {recursive: true});
  const files: Record<string, string> = {
    'package.json': JSON.stringify(manifest, null, 2) + '\n',
    'index.js': shimEntry(),
    'index.d.ts': shimEntry(),
    'README.md': shimReadme(primary.version),
  };
  for (const [name, content] of Object.entries(files)) {
    writeFileSync(path.join(dest, name), content);
  }
  return Object.keys(files);
}

function main(args: {dest: string; manifest: string}): void {
  const primary = JSON.parse(readFileSync(resolve(args.manifest), 'utf8')) as PrimaryManifest;
  const dest = resolve(args.dest);
  const files = writeShim(dest, primary);
  process.stdout.write(
      `${LEGACY}@${primary.version} -> ${PRIMARY}@${primary.version}\n  ${dest}\n${files.map((f) => `  ${f}`).join('\n')}\n`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === path.resolve(import.meta.filename)) {
  const cli = cac('pnpm package:legacy');
  cli.command('', `write the ${LEGACY} compatibility package directory`)
      .option('--dest <dir>', 'where the package directory goes')
      .option('--manifest <file>', 'the manifest to mirror', {default: 'apps/typescript/package.json'})
      .action((options: {dest?: string; manifest: string}) => {
        try {
          if (!options.dest) throw new Error('--dest is required');
          main({dest: options.dest, manifest: options.manifest});
        } catch (error) {
          console.error(error instanceof Error ? error.message : String(error));
          process.exitCode = 1;
        }
      });
  cli.help();
  cli.parse();
}
