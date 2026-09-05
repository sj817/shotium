// Expose .claude/skills to agents that look in .agents/skills (Codex, Gemini
// CLI, Copilot, Antigravity). The canonical copies live in .claude/skills so
// Claude Code needs no setup; this makes .agents/skills/<name> point at each of
// them. Junctions on Windows (no admin, no Developer Mode), symlinks elsewhere.
// .agents/skills is gitignored; run this once per checkout, and again after
// adding a skill.
//
//   pnpm skills:link
//   pnpm skills:link --remove

import {lstat, mkdir, readdir, rmdir, symlink, unlink} from 'node:fs/promises';
import path from 'node:path';

import {cac} from 'cac';

const root = path.resolve(import.meta.dirname, '..', '..');
const source = path.join(root, '.claude', 'skills');
const target = path.join(root, '.agents', 'skills');
const onWindows = process.platform === 'win32';

const cli = cac('link-skills').option('--remove', 'remove the links instead of creating them');
cli.help();
const {options} = cli.parse();
if (options.help) process.exit(0);

async function linkKind(file: string): Promise<'link' | 'dir' | 'missing'> {
  try {
    const stats = await lstat(file);
    return stats.isSymbolicLink() ? 'link' : 'dir';
  } catch {
    return 'missing';
  }
}

// A directory junction is removed with rmdir on Windows; unlink reports EPERM.
// Neither touches the directory it points at.
async function removeLink(file: string): Promise<void> {
  await (onWindows ? rmdir(file) : unlink(file));
}

const skills = (await readdir(source, {withFileTypes: true}))
                   .filter((entry) => entry.isDirectory())
                   .map((entry) => entry.name)
                   .sort();

if (options.remove) {
  for (const name of skills) {
    const link = path.join(target, name);
    if ((await linkKind(link)) === 'link') {
      await removeLink(link);
      console.log(`removed ${name}`);
    }
  }
} else {
  await mkdir(target, {recursive: true});
  for (const name of skills) {
    const link = path.join(target, name);
    switch (await linkKind(link)) {
      case 'link':
        console.log(`ok      ${name}`);
        break;
      case 'dir':
        console.log(`skip    ${name}: a real directory is in the way`);
        break;
      case 'missing':
        await symlink(path.join(source, name), link, onWindows ? 'junction' : 'dir');
        console.log(`linked  ${name}`);
        break;
    }
  }
}
