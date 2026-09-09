// Merge only retained paths; never merge Chromium's whole history into this slice.
import {createHash} from 'node:crypto';
import {mkdir, readFile, writeFile, lstat, realpath, mkdtemp, rm, open} from 'node:fs/promises';
import path from 'node:path';
import os from 'node:os';
import {cac} from 'cac';
import {execa} from 'execa';
import pMap from 'p-map';
import {root} from './lib/repo.ts';

type Entry = {oid: string; mode: string};
type Row = {path: string; status: string; old?: string; ours?: string; theirs?: string; sha256?: string; resolution?: string};
type Plan = {version: 1; base: string; head: string; target: string; scopes: string[]; retainedOnly?: boolean; upstreamPrefix?: string; rows: Row[]};
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
    input: unique.join('\n') + '\n', encoding: 'buffer', stripFinalNewline: false, maxBuffer: 256 * 1024 * 1024});
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
  scopes: string[], destination: string, remote = 'upstream', retainedOnly = false, upstreamPrefix?: string): Promise<Plan> {
  if (!scopes.length) throw new Error('At least one explicit scope is required');
  scopes.forEach(safeName);
  if (upstreamPrefix) {
    safeName(upstreamPrefix);
    if (scopes.some(s => s !== upstreamPrefix && !s.startsWith(upstreamPrefix + '/'))) {
      throw new Error('Vendored scopes must stay inside upstream prefix');
    }
  }
  const [base, head, target] = await Promise.all([baseRef, 'HEAD', targetRef].map(async ref =>
    (await git(repo, ['rev-parse', '--verify', '--end-of-options', `${ref}^{commit}`])).stdout));
  const upstreamTree = async (ref: string) => {
    if (!upstreamPrefix) return tree(repo, ref, scopes);
    const entries = await tree(repo, ref, []);
    return new Map([...entries].map(([name, entry]) => [upstreamPrefix + '/' + name, entry] as const)
      .filter(([name]) => scopes.some(s => name === s || name.startsWith(s + '/'))));
  };
  const [old, ours, theirs] = await Promise.all([upstreamTree(base), tree(repo, head, scopes), upstreamTree(target)]);
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
          '-L', 'old-upstream', '-L', 'new-upstream', ...names], {encoding: 'buffer', stripFinalNewline: false, reject: false,
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
  const plan: Plan = {version: 1, base, head, target, scopes, retainedOnly, upstreamPrefix, rows};
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
  const dirty = new Set<string>();
  const status = (await git(repo, ['--no-optional-locks', 'status', '--porcelain=v1', '-z', '--untracked-files=no'])).stdout.split('\0');
  for (let i = 0; i < status.length; i++) {
    const entry = status[i];
    if (!entry) continue;
    dirty.add(entry.slice(3));
    if (/[RC]/.test(entry.slice(0, 2))) dirty.add(status[++i]);
  }
  const prepared: {name: string; file: string; before: Buffer; after: Buffer}[] = [];
  for (const name of new Set(names)) {
    const row = plan.rows.find(r => r.path === name);
    if (!row || !['take-upstream', 'merged'].includes(row.status) || entries.get(name)?.oid !== row.ours) {
      throw new Error(`Not an eligible retained update: ${name}`);
    }
    const file = await regularFile(repo, name);
    if (dirty.has(name)) throw new Error(`Dirty source: ${name}`);
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

export async function resolvePlan(directory: string, decisionsFile: string): Promise<number> {
  const manifest = path.join(directory, 'manifest.json');
  const plan: Plan = JSON.parse(await readFile(manifest, 'utf8'));
  const decisions: {path: string; source: string; reason: string}[] = JSON.parse(await readFile(decisionsFile, 'utf8'));
  const prepared = await Promise.all(decisions.map(async decision => {
    const row = plan.rows.find(r => r.path === safeName(decision.path));
    if (row?.status !== 'conflict' || !decision.reason?.trim()) throw new Error(`Not an unresolved conflict with a reason: ${decision.path}`);
    const content = await readFile(decision.source);
    if (/^(?:<<<<<<<(?: |$)|\|\|\|\|\|\|\|(?: |$)|=======$|>>>>>>>(?: |$))/m.test(content.toString())) throw new Error(`Conflict markers remain: ${row.path}`);
    return {row, content, reason: decision.reason};
  }));
  for (const {row, content, reason} of prepared) {
    const file = path.join(directory, 'files', row.path);
    await mkdir(path.dirname(file), {recursive: true});
    await writeFile(file, content);
    row.status = 'merged'; row.resolution = reason; row.sha256 = digest(content);
  }
  await writeFile(manifest, JSON.stringify(plan, null, 2) + '\n');
  await writeFile(path.join(directory, 'conflicts.json'), JSON.stringify(plan.rows.filter(r => r.status === 'conflict'), null, 2) + '\n');
  return prepared.length;
}

// Follow-up corrections may only replace bytes previously applied by this plan.
export async function revisePlan(repo: string, directory: string, decisionsFile: string): Promise<number> {
  const manifest = path.join(directory, 'manifest.json');
  const plan: Plan = JSON.parse(await readFile(manifest, 'utf8'));
  if ((await git(repo, ['rev-parse', 'HEAD'])).stdout !== plan.head) throw new Error('HEAD changed');
  const decisions: {path: string; source: string; reason: string}[] = JSON.parse(await readFile(decisionsFile, 'utf8'));
  if (new Set(decisions.map(d => d.path)).size !== decisions.length) throw new Error('Duplicate decisions');
  const prepared = await Promise.all(decisions.map(async decision => {
    const row = plan.rows.find(r => r.path === safeName(decision.path));
    if (!row || !['merged', 'take-upstream', 'adopted', 'restored'].includes(row.status) || !decision.reason?.trim()) throw new Error(`Not an applied candidate: ${decision.path}`);
    const file = await regularFile(repo, row.path);
    const before = await readFile(file);
    if (digest(before) !== row.sha256) throw new Error(`Source differs from plan: ${row.path}`);
    const after = await readFile(decision.source);
    if (/^(?:<<<<<<<(?: |$)|\|\|\|\|\|\|\|(?: |$)|=======$|>>>>>>>(?: |$))/m.test(after.toString())) throw new Error(`Conflict markers remain: ${row.path}`);
    return {row, file, before, after, reason: decision.reason};
  }));
  const backup = path.join(directory, `revision-${Date.now()}`);
  await mkdir(backup);
  for (const p of prepared) {
    const saved = path.join(backup, p.row.path);
    await mkdir(path.dirname(saved), {recursive: true});
    await writeFile(saved, p.before);
  }
  for (const p of prepared) {
    if (!(await readFile(p.file)).equals(p.before)) throw new Error(`Concurrent edit: ${p.row.path}; backup: ${backup}`);
    await writeFile(p.file, p.after);
    await writeFile(path.join(directory, 'files', p.row.path), p.after);
    p.row.sha256 = digest(p.after);
    p.row.resolution = [p.row.resolution, p.reason].filter(Boolean).join('; ');
  }
  await writeFile(manifest, JSON.stringify(plan, null, 2) + '\n');
  await writeFile(path.join(backup, 'receipt.json'), JSON.stringify({head: plan.head, target: plan.target,
    revisions: prepared.map(p => ({path: p.row.path, before: digest(p.before), after: digest(p.after), reason: p.reason}))}, null, 2));
  return prepared.length;
}

export async function adoptPlan(repo: string, directory: string, decisionsFile: string, remote = 'upstream', restoreDeleted = false): Promise<number> {
  const manifest = path.join(directory, 'manifest.json');
  const plan: Plan = JSON.parse(await readFile(manifest, 'utf8'));
  if ((await git(repo, ['rev-parse', 'HEAD'])).stdout !== plan.head) throw new Error('HEAD changed');
  const decisions: {path: string; reason: string}[] = JSON.parse(await readFile(decisionsFile, 'utf8'));
  if (new Set(decisions.map(d => d.path)).size !== decisions.length) throw new Error('Duplicate decisions');
  const names = decisions.map(d => safeName(d.path));
  const prefix = plan.upstreamPrefix;
  if (prefix && names.some(n => !n.startsWith(prefix + '/'))) throw new Error('New source outside vendor prefix');
  const rawEntries = await tree(repo, plan.target, prefix ? names.map(n => n.slice(prefix.length + 1)) : names);
  const entries = prefix ? new Map([...rawEntries].map(([n, e]) => [prefix + '/' + n, e])) : rawEntries;
  const prepared = await Promise.all(decisions.map(async decision => {
    const row = plan.rows.find(r => r.path === decision.path);
    const entry = entries.get(decision.path);
    if (row?.status !== (restoreDeleted ? 'keep-deleted' : 'new-file-review') || !decision.reason?.trim() ||
        entry?.mode !== '100644' || entry.oid !== row.theirs) throw new Error(`Not an eligible new source: ${decision.path}`);
    // Explicitly scoped new trees are allowed; every existing ancestor must be a directory.
    let parent = await realpath(repo);
    for (const part of row.path.split('/').slice(0, -1)) {
      parent = path.join(parent, part);
      try {
        const stat = await lstat(parent);
        if (stat.isSymbolicLink() || !stat.isDirectory()) throw new Error(`Linked parent: ${row.path}`);
      } catch (error) {
        if ((error as NodeJS.ErrnoException).code !== 'ENOENT') throw error;
      }
    }
    const file = path.join(parent, path.basename(row.path));
    try { await lstat(file); } catch (error) {
      if ((error as NodeJS.ErrnoException).code === 'ENOENT') return {row, file, reason: decision.reason};
      throw error;
    }
    throw new Error(`New source already exists: ${row.path}`);
  }));
  const data = await blobs(repo, prepared.map(p => p.row.theirs!), remote);
  const journal = path.join(directory, `adoption-${Date.now()}`);
  await mkdir(journal);
  await writeFile(path.join(journal, 'intent.json'), JSON.stringify({head: plan.head, target: plan.target, decisions}, null, 2));
  for (const p of prepared) {
    const content = data.get(p.row.theirs!)!;
    await mkdir(path.dirname(p.file), {recursive: true});
    await writeFile(p.file, content, {flag: 'wx'});
    const candidate = path.join(directory, 'files', p.row.path);
    await mkdir(path.dirname(candidate), {recursive: true});
    await writeFile(candidate, content);
    p.row.status = restoreDeleted ? 'restored' : 'adopted'; p.row.sha256 = digest(content); p.row.resolution = p.reason;
  }
  await writeFile(manifest, JSON.stringify(plan, null, 2) + '\n');
  await writeFile(path.join(journal, 'receipt.json'), JSON.stringify({head: plan.head, target: plan.target, decisions}, null, 2));
  return prepared.length;
}

export async function retirePlan(repo: string, directory: string, decisionsFile: string): Promise<number> {
  const manifest = path.join(directory, 'manifest.json');
  const plan: Plan = JSON.parse(await readFile(manifest, 'utf8'));
  if ((await git(repo, ['rev-parse', 'HEAD'])).stdout !== plan.head) throw new Error('HEAD changed');
  const decisions: {path: string; reason: string}[] = JSON.parse(await readFile(decisionsFile, 'utf8'));
  if (new Set(decisions.map(d => d.path)).size !== decisions.length) throw new Error('Duplicate decisions');
  const dirty = new Set((await git(repo, ['--no-optional-locks', 'diff', 'HEAD', '--name-only', '-z'])).stdout.split('\0'));
  const prepared = await Promise.all(decisions.map(async decision => {
    const row = plan.rows.find(r => r.path === safeName(decision.path));
    if (row?.status !== 'upstream-delete-review' || !decision.reason?.trim() || dirty.has(row.path)) throw new Error(`Not a clean upstream deletion: ${decision.path}`);
    const file = await regularFile(repo, row.path);
    return {row, file, before: await readFile(file), reason: decision.reason};
  }));
  const backup = path.join(directory, `retirement-${Date.now()}`);
  await mkdir(backup);
  for (const p of prepared) {
    const saved = path.join(backup, p.row.path);
    await mkdir(path.dirname(saved), {recursive: true});
    await writeFile(saved, p.before);
  }
  await writeFile(path.join(backup, 'intent.json'), JSON.stringify({head: plan.head, target: plan.target, decisions}, null, 2));
  for (const p of prepared) {
    if (!(await readFile(p.file)).equals(p.before)) throw new Error(`Concurrent edit: ${p.row.path}; backup: ${backup}`);
    await rm(p.file);
    p.row.status = 'retired'; p.row.resolution = p.reason;
  }
  await writeFile(manifest, JSON.stringify(plan, null, 2) + '\n');
  await writeFile(path.join(backup, 'receipt.json'), JSON.stringify({head: plan.head, target: plan.target, decisions}, null, 2));
  return prepared.length;
}

async function withPlanLock<T>(directory: string, action: () => Promise<T>): Promise<T> {
  const lock = path.join(directory, '.operation.lock');
  const handle = await open(lock, 'wx');
  try {
    await handle.writeFile(JSON.stringify({pid: process.pid, started: new Date().toISOString()}));
    return await action();
  } finally {
    await handle.close();
    await rm(lock);
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === import.meta.filename) {
  const cli = cac('upstream-sync');
  cli.command('plan').option('--base <ref>', 'recorded upstream baseline')
    .option('--target <ref>', 'new upstream commit').option('--scope <paths>', 'comma-separated directory/file scopes')
    .option('--out <path>', 'new output directory relative to repository root')
    .option('--remote <name>', 'partial-clone blob source', {default: 'upstream'})
    .option('--retained-only', 'omit deleted trees; list new siblings in retained directories')
    .option('--upstream-prefix <path>', 'map a standalone dependency repository under this local directory')
    .action(async (o: {base: string; target: string; scope: string; out: string; remote: string; retainedOnly?: boolean; upstreamPrefix?: string}) => {
      if (!o.base || !o.target || !o.scope || !o.out) throw new Error('--base, --target, --scope and --out are required');
      await createPlan(root, o.base, o.target, o.scope.split(','), path.resolve(root, o.out), o.remote, o.retainedOnly, o.upstreamPrefix);
    });
  cli.command('apply').option('--plan <path>', 'plan directory').option('--selection <path>', 'JSON list of approved retained paths')
    .action(async (o: {plan: string; selection: string}) => {
      if (!o.plan || !o.selection) throw new Error('--plan and --selection are required');
      console.log(`Applied ${await withPlanLock(path.resolve(root, o.plan), () => applyPlan(root, path.resolve(root, o.plan), path.resolve(root, o.selection)))} retained files; not compiled`);
    });
  cli.command('resolve').option('--plan <path>', 'plan directory').option('--decisions <path>', 'JSON array of path, source (absolute), reason')
    .action(async (o: {plan: string; decisions: string}) => {
      if (!o.plan || !o.decisions) throw new Error('--plan and --decisions are required');
      console.log(`Recorded ${await withPlanLock(path.resolve(root, o.plan), () => resolvePlan(path.resolve(root, o.plan), path.resolve(root, o.decisions)))} conflict resolutions; not applied`);
    });
  cli.command('revise').option('--plan <path>', 'plan directory').option('--decisions <path>', 'reviewed corrections to already applied files')
    .action(async (o: {plan: string; decisions: string}) => {
      if (!o.plan || !o.decisions) throw new Error('--plan and --decisions are required');
      console.log(`Revised ${await withPlanLock(path.resolve(root, o.plan), () => revisePlan(root, path.resolve(root, o.plan), path.resolve(root, o.decisions)))} applied files; not compiled`);
    });
  cli.command('adopt').option('--plan <path>', 'plan directory').option('--decisions <path>', 'reviewed new sibling paths and reasons')
    .option('--remote <name>', 'partial-clone blob source', {default: 'upstream'})
    .action(async (o: {plan: string; decisions: string; remote: string}) => {
      if (!o.plan || !o.decisions) throw new Error('--plan and --decisions are required');
      console.log(`Adopted ${await withPlanLock(path.resolve(root, o.plan), () => adoptPlan(root, path.resolve(root, o.plan), path.resolve(root, o.decisions), o.remote))} new sources; not compiled`);
    });
  cli.command('retire').option('--plan <path>', 'plan directory').option('--decisions <path>', 'reviewed upstream deletions and reasons')
    .action(async (o: {plan: string; decisions: string}) => {
      if (!o.plan || !o.decisions) throw new Error('--plan and --decisions are required');
      console.log(`Retired ${await withPlanLock(path.resolve(root, o.plan), () => retirePlan(root, path.resolve(root, o.plan), path.resolve(root, o.decisions)))} obsolete sources; not compiled`);
    });
  cli.command('restore').option('--plan <path>', 'explicitly scoped plan directory')
    .option('--decisions <path>', 'reviewed previously deleted paths with native dependency evidence')
    .option('--remote <name>', 'partial-clone blob source', {default: 'upstream'})
    .action(async (o: {plan: string; decisions: string; remote: string}) => {
      if (!o.plan || !o.decisions) throw new Error('--plan and --decisions are required');
      console.log(`Restored ${await withPlanLock(path.resolve(root, o.plan), () => adoptPlan(root, path.resolve(root, o.plan), path.resolve(root, o.decisions), o.remote, true))} reviewed native dependencies; not compiled`);
    });
  cli.help();
  cli.parse();
}
