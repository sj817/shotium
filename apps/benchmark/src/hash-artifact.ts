import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {parseArgs, recoverNpmRunValues} from './args.ts';

function filesBelow(root: string): string[] {
  if (!fs.existsSync(root)) throw new Error(`evidence directory does not exist: ${root}`);
  const files: string[] = [];
  const pending = [root];
  while (pending.length) {
    const directory = pending.pop()!;
    for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
      const candidate = path.join(directory, entry.name);
      if (entry.isDirectory()) pending.push(candidate);
      else if (entry.isFile()) files.push(candidate);
    }
  }
  return files.sort((left, right) => left.localeCompare(right));
}

export function hashDirectory(root: string) {
  const hash = crypto.createHash('sha256');
  const files = filesBelow(root).map((file) => {
    const relative = path.relative(root, file).replaceAll('\\', '/');
    const bytes = fs.readFileSync(file);
    const sha256 = crypto.createHash('sha256').update(bytes).digest('hex');
    hash.update(relative).update('\0').update(sha256).update('\n');
    return {path: relative, bytes: bytes.length, sha256};
  });
  if (!files.length) throw new Error(`evidence directory is empty: ${root}`);
  return {sha256: hash.digest('hex'), files};
}

export function writeArtifactRecord(input: string, output: string, name: string, runUrl: string | null) {
  const record = {
    name,
    run_url: runUrl,
    generated_utc: new Date().toISOString(),
    ...hashDirectory(input),
  };
  fs.mkdirSync(path.dirname(output), {recursive: true});
  fs.writeFileSync(output, `${JSON.stringify(record, null, 2)}\n`);
  return record;
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const options = parseArgs(process.argv.slice(2));
  recoverNpmRunValues(options, ['input', 'output', 'name', 'runUrl']);
  if (!options.input || !options.output || !options.name) {
    throw new Error('usage: hash-artifact --input <dir> --output <file> --name <artifact> [--run-url <url>]');
  }
  const record = writeArtifactRecord(
      path.resolve(options.input), path.resolve(options.output), String(options.name),
      options.runUrl ? String(options.runUrl) : null);
  process.stdout.write(`${record.sha256}\n`);
}
