// apps/demo-card/card.html is the one page every C ABI demo renders, and the
// same page the README assets are made from. Each language directory carries
// its own copy so that a language ZIP is self-contained; this keeps the copies
// identical to the source. `--check` is what CI runs, so an edit to the source
// without a re-sync fails the checks job instead of shipping five stale pages.
import {createHash} from 'node:crypto';
import {copyFileSync, existsSync, readFileSync} from 'node:fs';
import {cac} from 'cac';
import {resolve} from '../lib/repo.ts';

export const SOURCE = 'apps/demo-card/card.html';
export const LANGUAGES = ['go', 'python', 'rust', 'csharp', 'java'] as const;

const sha256 = (file: string) => createHash('sha256').update(readFileSync(file)).digest('hex');

function main(check: boolean): void {
  const source = resolve(SOURCE);
  const expected = sha256(source);
  const stale: string[] = [];
  for (const language of LANGUAGES) {
    const target = resolve('apps', language, 'card.html');
    const current = existsSync(target) ? sha256(target) : null;
    if (current === expected) continue;
    if (check) {
      stale.push(`apps/${language}/card.html`);
      continue;
    }
    copyFileSync(source, target);
    console.log(`synced apps/${language}/card.html`);
  }
  if (stale.length) {
    console.error(`out of date with ${SOURCE}: ${stale.join(', ')}\nrun \`pnpm demo:sync\` and stage the copies`);
    process.exitCode = 1;
    return;
  }
  console.log(check ? `ok: ${LANGUAGES.length} copies match ${SOURCE}` : `ok: ${LANGUAGES.length} copies of ${SOURCE}`);
}

const cli = cac('pnpm demo:sync');
cli.command('', 'copy apps/demo-card/card.html into every language demo directory')
    .option('--check', 'fail instead of copying when a copy differs from the source')
    .action((options: {check?: boolean}) => {
      try { main(Boolean(options.check)); } catch (error) { console.error(error); process.exitCode = 1; }
    });
cli.help();
cli.parse();
