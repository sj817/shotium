// Merge only retained paths; never merge Chromium's whole history into this slice.
import {createHash} from 'node:crypto';
import {mkdir, readFile, writeFile, lstat, realpath, mkdtemp, rm} from 'node:fs/promises';
import path from 'node:path';
import os from 'node:os';
import {cac} from 'cac';
import {execa} from 'execa';
import pMap from 'p-map';
import {root} from './lib/repo.ts';

type Entry = {oid: string; mode: string};
type Row = {path: string; status: string; old?: string; ours?: string; theirs?: string; sha256?: string};
type Plan = {version: 1; base: string; head: string; target: string; scopes: string[]; retainedOnly?: boolean; rows: Row[]};
const digest = (data: Buffer) => createHash('sha256').update(data).digest('hex');
const git = (repo: string, args: string[]) => execa('git', args, {cwd: repo, maxBuffer: 128 * 1024 * 1024});

function safeName(name: string): string {
  if (!name || name.includes('\\') || name.includes(':') || name.startsWith('/') ||
      name.split('/').some(p => !p || p === '.' || p === '..' || p.toLowerCase() === '.git')) {
    throw new Error(`Unsafe relative path: ${name}`);
  }
  return name;
}

async function tree(repo: string, ref: string, scopes: string[]): Promise<Map<string, Entry>> {
  const {stdout} = await git(repo, ['ls-tree', '-rz', ref, '--', ...scopes]);
  return new Map(stdout.split('\0').filter(Boolean).map(line => {
    const match = /^(\d+) \w+ ([0-9a-f]+)\t([\s\S]+)$/.exec(line)!;
    return [safeName(match[3]), {mode: match[1], oid: match[2]}];
  }));
}

// Prefetch missing blobs in bounded packs instead of one network request per file.
async function blobs(repo: string, ids: string[], remote: string): Promise<Map<string, Buffer>> {
  const unique = [...new Set(ids)];
  if (!unique.length) return new Map();
  const check = await execa('git', ['cat-file', '--batch-check=%(objectname) %(objecttype)'], {
    cwd: repo, input: unique.join('\n') + '\n', env: {GIT_NO_LAZY_FETCH: '1'},
    maxBuffer: 128 * 1024 * 1024,
  });
  const missing = check.stdout.split('\n').filter(l => l.endsWith(' missing')).map(l => l.split(' ')[0]);
  for (let i = 0; i < missing.length; i += 96) {
    console.log(`fetch missing blobs ${i + 1}–${Math.min(i + 96, missing.length)}/${missing.length}`);
    await git(repo, ['-c', 'fetch.negotiationAlgorithm=noop', 'fetch', '--no-tags',
      '--no-write-fetch-head', '--recurse-submodules=no', remote, ...missing.slice(i, i + 96)]);
  }
  const result = await execa('git', ['cat-file', '--batch'], {cwd: repo,
    input: unique.join('\n') + '\n', encoding: 'buffer', maxBuffer: 256 * 1024 * 1024});
  const output = Buffer.from(result.stdout);
  const found = new Map<string, Buffer>();
  let cursor = 0;
  for (const id of unique) {
    const end = output.indexOf(10, cursor);
    const header = output.subarray(cursor, end).toString();
    const match = /^([0-9a-f]+) blob (\d+)$/.exec(header);
    if (!match || match[1] !== id) throw new Error(`Unexpected blob response: ${header}`);
    const length = Number(match[2]);
    found.set(id, output.subarray(end + 1, end + 1 + length));
    cursor = end + 2 + length;
  }
  return found;
}

export async function createPlan(repo: string, baseRef: string, targetRef: string,
  scopes: string[], destination: string, remote = 'upstream', retainedOnly = false): Promise<Plan> {
  if (!scopes.length) throw new Error('At least one explicit scope is required');
  scopes.forEach(safeName);
  const [base, head, target] = await Promise.all([baseRef, 'HEAD', targetRef].map(async ref =>
    (await git(repo, ['rev-parse', '--verify', '--end-of-options', `${ref}^{commit}`])).stdout));
  const [old, ours, theirs] = await Promise.all([base, head, target].map(ref => tree(repo, ref, scopes)));
  const rows: Row[] = [];
  const retainedParents = new Set([...ours.keys()].map(n => path.posix.dirname(n)));
  // Full-tree updates need not materialize hundreds of thousands of deleted paths.
  // New sibling files remain visible; entirely new directories need dependency review.
  const incoming = retainedOnly ? [...theirs.keys()].filter(n => !old.has(n) && retainedParents.has(path.posix.dirname(n))) : [...theirs.keys()];
  for (const name of new Set([...ours.keys(), ...incoming])) {
    const a = old.get(name), b = ours.get(name), c = theirs.get(name);
    let status: string;
    if (!b) status = a ? 'keep-deleted' : 'new-file-review';
    else if (!a) status = c && c.oid !== b.oid ? 'add-add-review' : 'local-only';
    else if (!c) status = 'upstream-delete-review';
    else if ([a, b, c].some(e => e.mode !== '100644' && e.mode !== '100755') ||
      a.mode !== b.mode || a.mode !== c.mode) status = 'mode-review';
    else if (b.oid === c.oid) status = 'already-current';
    else if (a.oid === c.oid) status = 'unchanged-upstream';
    else status = a.oid === b.oid ? 'take-upstream' : 'merge';
    rows.push({path: name, status, old: a?.oid, ours: b?.oid, theirs: c?.oid});
  }
  await mkdir(path.dirname(destination), {recursive: true});
  await mkdir(destination); // Never reuse or overwrite a previous plan.
  const pending = rows.filter(r => r.status === 'take-upstream' || r.status === 'merge');
  console.log(`${rows.length} paths; ${pending.length} candidate updates`);
  const data = await blobs(repo, pending.flatMap(r => r.status === 'merge'
    ? [r.old!, r.ours!, r.theirs!] : [r.theirs!]), remote);
  await pMap(pending, async row => {
    let merged = data.get(row.theirs!)!;
    if (row.status === 'merge') {
      const inputs = [data.get(row.ours!)!, data.get(row.old!)!, merged];
      if (inputs.some(b => b.includes(0))) { row.status = 'binary-review'; return; }
      const temp = await mkdtemp(path.join(os.tmpdir(), 'shotium-merge-'));
      try {
        const names = ['ours', 'base', 'upstream'].map(n => path.join(temp, n));
        await Promise.all(names.map((n, i) => writeFile(n, inputs[i])));
        const result = await execa('git', ['merge-file', '-p', '--diff3', '-L', 'shotium',
          '-L', 'old-upstream', '-L', 'new-upstream', ...names], {encoding: 'buffer', reject: false,
          maxBuffer: 64 * 1024 * 1024});
        if (result.exitCode === undefined || result.exitCode > 127) throw new Error(result.stderr.toString());
        row.status = result.exitCode === 0 ? 'merged' : 'conflict';
        merged = Buffer.from(result.stdout);
      } finally {
        if (!path.resolve(temp).startsWith(path.resolve(os.tmpdir()) + path.sep)) throw new Error('Unsafe temporary directory');
        await rm(temp, {recursive: true, force: true});
      }
    }
    const file = path.join(destination, row.status === 'conflict' ? 'conflicts' : 'files', row.path);
    await mkdir(path.dirname(file), {recursive: true});
    await writeFile(file, merged);
    row.sha256 = digest(merged);
  }, {concurrency: 4});
  const plan: Plan = {version: 1, base, head, target, scopes, retainedOnly, rows};
  await writeFile(path.join(destination, 'manifest.json'), JSON.stringify(plan, null, 2) + '\n');
  const ready = rows.filter(r => ['take-upstream', 'merged'].includes(r.status)).map(r => r.path);
  await writeFile(path.join(destination, 'ready.json'), JSON.stringify(ready, null, 2) + '\n');
  await writeFile(path.join(destination, 'conflicts.json'), JSON.stringify(rows.filter(r => r.status === 'conflict'), null, 2) + '\n');
  await writeFile(path.join(destination, 'review.json'), JSON.stringify(rows.filter(r => r.status.endsWith('-review')), null, 2) + '\n');
  const counts = rows.reduce<Record<string, number>>((out, r) => {
    out[r.status] = (out[r.status] ?? 0) + 1; return out;
  }, {});
  await writeFile(path.join(destination, 'summary.md'), `# Upstream merge preview\n\n` +
    `Base: ${base}\n\nLocal: ${head}\n\nTarget: ${target}\n\n` +
    Object.entries(counts).map(([name, count]) => `- ${name}: ${count}`).join('\n') +
    '\n\nClean text merges are not build or rendering verification. New files and upstream deletions require review.\n');
  console.log(JSON.stringify(counts));
  return plan;
}

// Reject symlinks in every path component, including parent directories.
async function regularFile(repo: string, name: string): Promise<string> {
  let cursor = await realpath(repo);
  const parts = safeName(name).split('/');
  for (let i = 0; i < parts.length; i++) {
    cursor = path.join(cursor, parts[i]);
    const stat = await lstat(cursor);
    if (stat.isSymbolicLink() || (i === parts.length - 1 ? !stat.isFile() : !stat.isDirectory())) {
      throw new Error(`Not a regular source path: ${name}`);
    }
  }
  return cursor;
}

export async function applyPlan(repo: string, directory: string, selection: string): Promise<number> {
  const plan: Plan = JSON.parse(await readFile(path.join(directory, 'manifest.json'), 'utf8'));
  const names: string[] = JSON.parse(await readFile(selection, 'utf8'));
  if (plan.version !== 1 || !Array.isArray(names) || names.some(n => typeof n !== 'string')) {
    throw new Error('Invalid plan/selection');
  }
  if ((await git(repo, ['rev-parse', 'HEAD'])).stdout !== plan.head) throw new Error('HEAD changed; regenerate plan');
  const entries = await tree(repo, 'HEAD', plan.scopes);
  const prepared: {name: string; file: string; before: Buffer; after: Buffer}[] = [];
  for (const name of new Set(names)) {
    const row = plan.rows.find(r => r.path === name);
    if (!row || !['take-upstream', 'merged'].includes(row.status) || entries.get(name)?.oid !== row.ours) {
      throw new Error(`Not an eligible retained update: ${name}`);
    }
    const file = await regularFile(repo, name);
    if ((await git(repo, ['--no-optional-locks', 'status', '--porcelain', '--', name])).stdout) throw new Error(`Dirty source: ${name}`);
    const before = await readFile(file);
    const after = await readFile(await regularFile(path.join(directory, 'files'), name));
    if (digest(after) !== row.sha256) throw new Error(`Candidate changed: ${name}`);
    prepared.push({name, file, before, after});
  }
  // Complete all preflight checks and backups before changing any source.
  const backup = path.join(directory, `backup-${Date.now()}`);
  for (const p of prepared) {
    const file = path.join(backup, p.name);
    await mkdir(path.dirname(file), {recursive: true});
    await writeFile(file, p.before);
  }
  try {
    for (const p of prepared) {
      if (!((await readFile(p.file)).equals(p.before))) throw new Error(`Concurrent edit: ${p.name}`);
      await writeFile(p.file, p.after);
    }
  } catch (error) {
    // Preserve the journal for recovery; do not overwrite concurrent edits during rollback.
    throw new Error(`Apply interrupted; original bytes are in ${backup}`, {cause: error});
  }
  await writeFile(path.join(backup, 'receipt.json'), JSON.stringify({head: plan.head, target: plan.target, paths: names}, null, 2));
  return prepared.length;
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('upstream-sync');
  cli.command('plan').option('--base <ref>', 'recorded upstream baseline')
    .option('--target <ref>', 'new upstream commit').option('--scope <paths>', 'comma-separated directory/file scopes')
    .option('--out <path>', 'new output directory relative to repository root')
    .option('--remote <name>', 'partial-clone blob source', {default: 'upstream'})
    .option('--retained-only', 'omit deleted trees; list new siblings in retained directories')
    .action(async (o: {base: string; target: string; scope: string; out: string; remote: string; retainedOnly?: boolean}) => {
      if (!o.base || !o.target || !o.scope || !o.out) throw new Error('--base, --target, --scope and --out are required');
      await createPlan(root, o.base, o.target, o.scope.split(','), path.resolve(root, o.out), o.remote, o.retainedOnly);
    });
  cli.command('apply').option('--plan <path>', 'plan directory').option('--selection <path>', 'JSON list of approved retained paths')
    .action(async (o: {plan: string; selection: string}) => {
      if (!o.plan || !o.selection) throw new Error('--plan and --selection are required');
      console.log(`Applied ${await applyPlan(root, path.resolve(root, o.plan), path.resolve(root, o.selection))} retained files; not compiled`);
    });
  cli.help();
  cli.parse();
}
