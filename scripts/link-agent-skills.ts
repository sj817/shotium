// Expose .claude/skills to agents that look in .agents/skills (Codex, Gemini
// CLI, Copilot, Antigravity). The canonical copies live in .claude/skills so
// Claude Code needs no setup; this makes .agents/skills/<name> point at each of
// them. Junctions on Windows (no admin, no Developer Mode), symlinks elsewhere.
// .agents/skills is gitignored; run this once per checkout, and again after
// adding, renaming or deleting a skill -- links whose skill is gone are pruned,
// so a rename does not leave the other agents loading a directory that no
// longer resolves.
//
// Only links into this checkout's .claude/skills are ever removed. The
// directory is gitignored precisely so that it can hold per-machine links to
// skills that live elsewhere; those are reported and left alone, unless one
// carries the name of a checked-in skill, in which case the checked-in skill
// wins and the old target is printed so the change is visible.
//
//   pnpm skills:link
//   pnpm skills:link --remove

import {lstat, mkdir, readdir, readlink, rmdir, symlink, unlink} from 'node:fs/promises';
import path from 'node:path';

import {cac} from 'cac';

const root = path.resolve(import.meta.dirname, '..');
const source = path.join(root, '.claude', 'skills');
const target = path.join(root, '.agents', 'skills');
const onWindows = process.platform === 'win32';

const cli = cac('link-skills').option('--remove', 'remove the links instead of creating them');
// The default command is what makes cac reject `--remov`; without it an
// unknown option parses silently and the run creates links it was asked to
// remove.
cli.command('', 'link .claude/skills into .agents/skills').action(() => {});
cli.help();
let options: Record<string, unknown>;
try {
  options = cli.parse().options;
} catch (error) {
  console.error(`link-skills: ${error instanceof Error ? error.message : String(error)}`);
  process.exit(2);
}
if (options.help) process.exit(0);

// What is at .agents/skills/<name>. The link kinds are decided by where the
// link points, read with readlink, not by whether it resolves: lstat succeeds
// on a dangling junction, and stat succeeds on a junction into a different
// checkout, so neither can tell a leftover from a correct link.
//
//   ok       points at .claude/skills/<name> of this checkout
//   ours     points elsewhere under this checkout's .claude/skills (a rename
//            left it behind, or it dangles); safe to remove
//   foreign  points outside .claude/skills: someone else's link, keep it
//   dir      a real directory, never touched
//   missing  nothing there
type Entry = {kind: 'ok' | 'ours' | 'foreign' | 'dir' | 'missing'; to?: string};

async function inspect(name: string): Promise<Entry> {
  const link = path.join(target, name);
  let stats;
  try {
    stats = await lstat(link);
  } catch {
    return {kind: 'missing'};
  }
  if (!stats.isSymbolicLink()) return {kind: 'dir'};
  const to = path.resolve(path.dirname(link), await readlink(link));
  if (samePath(to, path.join(source, name))) return {kind: 'ok', to};
  const inside = path.relative(source, to);
  const ours = inside !== '' && !inside.startsWith('..') && !path.isAbsolute(inside);
  return {kind: ours ? 'ours' : 'foreign', to};
}

// path.relative is case-insensitive on win32 and case-sensitive elsewhere,
// which is the file system's own rule on each.
function samePath(a: string, b: string): boolean {
  return path.relative(a, b) === '';
}

// A directory junction is removed with rmdir on Windows; unlink reports EPERM.
// Neither touches the directory it points at.
async function removeLink(name: string): Promise<void> {
  const link = path.join(target, name);
  await (onWindows ? rmdir(link) : unlink(link));
}

async function makeLink(name: string): Promise<void> {
  await symlink(path.join(source, name), path.join(target, name), onWindows ? 'junction' : 'dir');
}

async function names(dir: string): Promise<string[]> {
  return (await readdir(dir, {withFileTypes: true}))
      .filter((entry) => entry.isDirectory() || entry.isSymbolicLink())
      .map((entry) => entry.name)
      .sort();
}

// The source must exist: an unreadable .claude/skills is an error, not an
// empty skill set, or a sparse checkout would prune every link and exit 0.
// The target may be absent -- that is the state before the first run.
const skills = await names(source);
const linked = await names(target).catch((error: NodeJS.ErrnoException) => {
  if (error.code === 'ENOENT') return [] as string[];
  throw error;
});

// Remove the links under `names` that are this checkout's, and say which
// ones were left because they are not.
async function prune(candidates: string[], label: string): Promise<void> {
  for (const name of candidates) {
    const entry = await inspect(name);
    if (entry.kind === 'ok' || entry.kind === 'ours') {
      await removeLink(name);
      console.log(`${label} ${name}`);
    } else if (entry.kind === 'foreign') {
      console.log(`keep    ${name} -> ${entry.to}: not a link into .claude/skills`);
    }
  }
}

if (options.remove) {
  await prune(linked, 'removed');
  // Leave nothing behind if this checkout has no other .agents content; rmdir
  // refuses a non-empty directory, which is the signal to stop.
  for (const dir of [target, path.dirname(target)]) {
    try {
      await rmdir(dir);
    } catch {
      break;
    }
  }
} else {
  await mkdir(target, {recursive: true});
  await prune(linked.filter((name) => !skills.includes(name)), 'pruned ');
  for (const name of skills) {
    const entry = await inspect(name);
    switch (entry.kind) {
      case 'ok':
        console.log(`ok      ${name}`);
        break;
      case 'dir':
        console.log(`skip    ${name}: a real directory is in the way`);
        break;
      case 'ours':
      case 'foreign':
        // The name belongs to a checked-in skill; whatever the link pointed
        // at before, it points at that skill now, and the old target is
        // printed so an overridden per-machine link does not vanish silently.
        await removeLink(name);
        await makeLink(name);
        console.log(`relink  ${name} (was -> ${entry.to})`);
        break;
      case 'missing':
        await makeLink(name);
        console.log(`linked  ${name}`);
        break;
    }
  }
}
