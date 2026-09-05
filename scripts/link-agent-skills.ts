// Expose .claude/skills to agents that look in .agents/skills (Codex, Gemini
// CLI, Copilot, Antigravity). The canonical copies live in .claude/skills so
// Claude Code needs no setup; this makes .agents/skills/<name> point at each of
// them. Junctions on Windows (no admin, no Developer Mode), symlinks elsewhere.
// .agents/skills is gitignored; run this once per checkout, and again after
// adding, renaming or deleting a skill -- links whose skill is gone are pruned,
// so a rename does not leave the other agents loading a directory that no
// longer resolves.
//
//   pnpm skills:link
//   pnpm skills:link --remove

import {lstat, mkdir, readdir, rmdir, stat, symlink, unlink} from 'node:fs/promises';
import path from 'node:path';

import {cac} from 'cac';

const root = path.resolve(import.meta.dirname, '..');
const source = path.join(root, '.claude', 'skills');
const target = path.join(root, '.agents', 'skills');
const onWindows = process.platform === 'win32';

const cli = cac('link-skills').option('--remove', 'remove the links instead of creating them');
cli.help();
const {options} = cli.parse();
if (options.help) process.exit(0);

// `stale` is a link whose target is gone -- what a renamed or deleted skill
// leaves behind. lstat alone cannot see it: it succeeds on a dangling junction,
// so reporting on lstat only would call the leftover `ok` and keep agents
// loading a skill directory that no longer resolves.
type LinkKind = 'link' | 'stale' | 'dir' | 'missing';

async function linkKind(file: string): Promise<LinkKind> {
  let stats;
  try {
    stats = await lstat(file);
  } catch {
    return 'missing';
  }
  if (!stats.isSymbolicLink()) return 'dir';
  try {
    await stat(file);
    return 'link';
  } catch {
    return 'stale';
  }
}

// A directory junction is removed with rmdir on Windows; unlink reports EPERM.
// Neither touches the directory it points at.
async function removeLink(file: string): Promise<void> {
  await (onWindows ? rmdir(file) : unlink(file));
}

async function entriesOf(dir: string): Promise<string[]> {
  try {
    return (await readdir(dir, {withFileTypes: true}))
        .filter((entry) => entry.isDirectory() || entry.isSymbolicLink())
        .map((entry) => entry.name)
        .sort();
  } catch {
    return [];
  }
}

// Both branches enumerate .agents/skills, not .claude/skills: a link is only
// reachable from the directory it lives in, and the ones worth acting on are
// exactly the ones whose skill is gone from the source.
const skills = await entriesOf(source);
const linked = await entriesOf(target);

async function prune(names: string[], label: string): Promise<void> {
  for (const name of names) {
    const link = path.join(target, name);
    const kind = await linkKind(link);
    if (kind !== 'link' && kind !== 'stale') continue;
    await removeLink(link);
    console.log(`${label} ${name}`);
  }
}

if (options.remove) {
  await prune(linked, 'removed');
  // Leave nothing behind if this checkout has no other .agents content.
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
    const link = path.join(target, name);
    switch (await linkKind(link)) {
      case 'link':
        console.log(`ok      ${name}`);
        break;
      case 'dir':
        console.log(`skip    ${name}: a real directory is in the way`);
        break;
      case 'stale':
        // A skill directory that moved: the link name is still wanted, the
        // target it holds is not.
        await removeLink(link);
        await symlink(path.join(source, name), link, onWindows ? 'junction' : 'dir');
        console.log(`relink  ${name}`);
        break;
      case 'missing':
        await symlink(path.join(source, name), link, onWindows ? 'junction' : 'dir');
        console.log(`linked  ${name}`);
        break;
    }
  }
}
