// The public Release contract: isolated native deliveries and one checksum list.
import {createHash} from 'node:crypto';
import {constants, createReadStream} from 'node:fs';
import {chmod, copyFile, mkdir, readFile, readdir, stat, writeFile} from 'node:fs/promises';
import path from 'node:path';
import {root} from './repo.ts';

export const releasePlatforms = ['windows-amd64', 'windows-arm64', 'linux-amd64', 'linux-arm64', 'macos-amd64', 'macos-arm64'] as const;
export const releaseLanguages = ['go', 'python', 'rust', 'csharp', 'java'] as const;
export const checksumName = 'SHA256SUMS';
export const releaseArchives = [
  ...releasePlatforms.flatMap(platform => ['shotium-cli-' + platform + '.7z', 'shotium-c-abi-' + platform + '.7z']),
  ...releaseLanguages.map(language => 'shotium-example-' + language + '.7z'),
].sort();

export type NativeKind = 'cli' | 'c-abi';
export type NativeOS = 'win' | 'linux' | 'mac';
const resources = ['shotium_data.pak', 'shotium_strings.pak'];

export function nativeFiles(kind: NativeKind, os: NativeOS): string[] {
  if (!['win', 'linux', 'mac'].includes(os)) throw new Error('--os must be win, linux or mac');
  if (kind === 'cli') return [os === 'win' ? 'shotium.exe' : 'shotium', ...resources, 'LICENSE'];
  if (kind !== 'c-abi') throw new Error('unknown native delivery kind');
  return [
    ...(os === 'win' ? ['shotium.dll', 'shotium.dll.lib'] : [os === 'mac' ? 'libshotium.dylib' : 'libshotium.so']),
    ...resources, 'shot_api.h', 'C_ABI.md', 'C_ABI.zh.md', 'LICENSE',
  ];
}

export async function stageNativeDelivery(options: {
  kind: NativeKind; os: NativeOS; build: string; dest: string; sourceRoot?: string;
}): Promise<string[]> {
  const source = options.sourceRoot ?? root;
  const docs: Record<string, string> = {
    LICENSE: 'LICENSE', 'shot_api.h': 'shot/shot_api.h',
    'C_ABI.md': 'apps/c-abi/README.md', 'C_ABI.zh.md': 'apps/c-abi/README.zh.md',
  };
  const files = nativeFiles(options.kind, options.os);
  const inputs = files.map(name => ({name, source: docs[name] ? path.join(source, docs[name]) : path.join(options.build, name)}));
  for (const input of inputs) {
    const info = await stat(input.source).catch(() => undefined);
    if (!info?.isFile()) throw new Error('missing ' + options.kind + ' delivery input: ' + input.source);
  }
  // Never merge with an old combined delivery or erase someone else's staging.
  const existing = await readdir(options.dest).catch((error: NodeJS.ErrnoException) => {
    if (error.code === 'ENOENT') return [];
    throw error;
  });
  if (existing.length) throw new Error('delivery destination must be empty: ' + options.dest);
  await mkdir(options.dest, {recursive: true});
  for (const input of inputs) {
    const output = path.join(options.dest, input.name);
    if (input.name.startsWith('C_ABI.')) {
      await writeFile(output, archiveMarkdown(await readFile(input.source, 'utf8'), true));
    } else {
      await copyFile(input.source, output);
    }
    const executable = ['shotium', 'shotium.exe', 'shotium.dll', 'libshotium.so', 'libshotium.dylib'].includes(input.name);
    await chmod(output, executable ? 0o755 : 0o644);
  }
  return files;
}


/** Keep shared guide/header/license links usable outside the repository. */
export function archiveMarkdown(markdown: string, isGuide: boolean): string {
  let result = markdown
      .replaceAll('](../../LICENSE)', '](LICENSE)')
      .replaceAll('](../../shot/shot_api.h)', '](shot_api.h)')
      .replaceAll('](../c-abi/README.md)', '](C_ABI.md)')
      .replaceAll('](../c-abi/README.zh.md)', '](C_ABI.zh.md)');
  if (isGuide) {
    result = result.replaceAll('](./README.md)', '](C_ABI.md)')
        .replaceAll('](./README.zh.md)', '](C_ABI.zh.md)');
  }
  return result;
}

/** Validate separate CI artifacts before merging, so duplicate names cannot overwrite bytes. */
export async function collectNativeArchives(source: string, destination: string): Promise<void> {
  const platforms = await readdir(source, {withFileTypes: true});
  if (platforms.some(entry => !entry.isDirectory()) ||
      platforms.map(entry => entry.name).sort().join('\n') !== [...releasePlatforms].sort().join('\n')) {
    throw new Error('native artifact collection must contain exactly the six platform directories');
  }
  const inputs: Array<{source: string; target: string}> = [];
  for (const platform of releasePlatforms) {
    const directory = path.join(source, platform);
    const entries = await readdir(directory, {withFileTypes: true});
    const names = ['shotium-c-abi-' + platform + '.7z', 'shotium-cli-' + platform + '.7z'];
    if (entries.some(entry => !entry.isFile()) ||
        entries.map(entry => entry.name).sort().join('\n') !== names.join('\n')) {
      throw new Error('native artifact must contain only its CLI and C ABI archives: ' + platform);
    }
    for (const name of names) {
      const target = path.join(destination, name);
      if (await stat(target).catch((error: NodeJS.ErrnoException) => {
        if (error.code === 'ENOENT') return undefined;
        throw error;
      })) throw new Error('duplicate release archive: ' + name);
      inputs.push({source: path.join(directory, name), target});
    }
  }
  await mkdir(destination, {recursive: true});
  for (const input of inputs) await copyFile(input.source, input.target, constants.COPYFILE_EXCL);
}


export async function verifyNativeDelivery(kind: NativeKind, os: NativeOS, directory: string): Promise<void> {
  const entries = await readdir(directory, {withFileTypes: true});
  if (entries.some(entry => !entry.isFile()) ||
      entries.map(entry => entry.name).sort().join('\n') !== nativeFiles(kind, os).sort().join('\n')) {
    throw new Error('unexpected ' + kind + ' delivery contents: ' + directory);
  }
}

export async function sha256File(file: string): Promise<string> {
  const hash = createHash('sha256');
  for await (const chunk of createReadStream(file)) hash.update(chunk);
  return hash.digest('hex');
}

async function checkReleaseFiles(directory: string): Promise<void> {
  const entries = await readdir(directory, {withFileTypes: true});
  for (const entry of entries) {
    if (!entry.isFile() || (!releaseArchives.includes(entry.name) && entry.name !== checksumName)) {
      throw new Error('unexpected release entry: ' + entry.name);
    }
  }
  const names = new Set(entries.map(entry => entry.name));
  const missing = releaseArchives.filter(name => !names.has(name));
  if (missing.length) throw new Error('missing release archives: ' + missing.join(', '));
}

export async function writeReleaseChecksums(directory: string): Promise<void> {
  await checkReleaseFiles(directory);
  const lines: string[] = [];
  for (const name of releaseArchives) lines.push(await sha256File(path.join(directory, name)) + '  ' + name);
  await writeFile(path.join(directory, checksumName), lines.join('\n') + '\n', 'utf8');
}

export async function verifyReleaseChecksums(directory: string): Promise<void> {
  await checkReleaseFiles(directory);
  const contents = await readFile(path.join(directory, checksumName), 'utf8');
  if (!contents.endsWith('\n')) throw new Error('SHA256SUMS must end with a newline');
  const entries = contents.slice(0, -1).split('\n').map(line => {
    const match = /^([0-9a-f]{64})  (shotium-[a-z0-9-]+\.7z)$/.exec(line);
    if (!match) throw new Error('invalid SHA256SUMS line: ' + line);
    return {hash: match[1]!, name: match[2]!};
  });
  if (new Set(entries.map(entry => entry.name)).size !== entries.length) throw new Error('duplicate SHA256SUMS entry');
  if (entries.map(entry => entry.name).join('\n') !== releaseArchives.join('\n')) {
    throw new Error('SHA256SUMS must list exactly the 17 release archives in filename order');
  }
  for (const entry of entries) {
    if (await sha256File(path.join(directory, entry.name)) !== entry.hash) throw new Error('checksum mismatch: ' + entry.name);
  }
}
