// Read a ninja log for what went wrong, without the 6 KB of flags per failure.
//
// A `ninja -k 0` run over this tree echoes the full clang-cl command line
// after every FAILED: entry. At 200 failures that is a megabyte of noise
// around the twenty lines that say what is wrong. Two views of the same log:
//
//   classes   group by the diagnostic text (identifiers inside quotes kept:
//             'undeclared identifier X' is a different problem for each X),
//             with the file list per class. What build-engine prints after
//             a build, and what drives the next edit.
//   failures  one entry per failed edge with its first diagnostic, then a
//             count per target; optionally every diagnostic line, or the
//             commonest kinds first with the quoted parts collapsed.

// clang-cl diagnostics: path(line,col): error: message
const DIAG = /^(.*?)\((\d+),(\d+)\): (error|fatal error): (.*)$/;
// The class view's variant, which does not keep the column.
const CLASS_ERROR = /^(\S+?)\((\d+),\d+\): (?:error|fatal error): (.*)$/;
const LINK = /^(?:lld-link|LINK): (?:error|warning): (.*)$/;
const CLASS_LINK = /^(?:lld-link|ninja): (?:error|fatal error): (.*)$/;
const FAILED = /^FAILED: (?:\[code=\d+\] )?(\S+)/;
const PROGRESS = /^\[\d+\/\d+\]/;
// "no member named 'foo' in 'blink::Bar'" and the same for 'baz' are one
// problem, not two; collapsing the quoted parts groups them.
const QUOTED = /'[^']*'/g;

export interface Failure {
  object: string;
  block: string[];
}

// One (object, diagnostic lines) per failed edge.
export function parseFailures(log: string): Failure[] {
  const failures: Failure[] = [];
  let current: string[] | null = null;
  let object = '';
  const lines = log.split(/\r?\n/);
  if (lines.length && lines[lines.length - 1] === '') lines.pop();
  for (const line of lines) {
    const m = FAILED.exec(line);
    if (m) {
      if (current !== null) failures.push({object, block: current});
      object = m[1];
      current = [];
      continue;
    }
    if (current === null) continue;
    if (PROGRESS.test(line)) {
      failures.push({object, block: current});
      current = null;
      object = '';
      continue;
    }
    // The echoed command line is one enormous line starting with the compiler
    // path; nothing else in the block is remotely that long.
    if (line.startsWith('..\\..\\third_party\\llvm-build') || line.length > 900) continue;
    current.push(line);
  }
  if (current !== null) failures.push({object, block: current});
  return failures;
}

// The first real diagnostic in a block, without the include chain.
export function messageOf(block: string[]): string {
  for (const line of block) {
    const d = DIAG.exec(line);
    if (d) return d[5].trim();
    const l = LINK.exec(line);
    if (l) return l[1].trim();
  }
  for (const line of block) {
    if (line.trim()) return line.trim();
  }
  return '(no diagnostic)';
}

export function whereOf(block: string[]): string {
  for (const line of block) {
    const d = DIAG.exec(line);
    if (d) return `${d[1].replace(/\\/g, '/')}:${d[2]}`;
  }
  return '';
}

export interface ClassSummary {
  failed: number;
  classes: Map<string, string[]>;  // diagnostic text -> sites (file:line, or <link>)
}

export function classify(log: string): ClassSummary {
  const classes = new Map<string, string[]>();
  let failed = 0;
  for (const line of log.split(/\r?\n/)) {
    if (line.startsWith('FAILED:')) {
      failed++;
      continue;
    }
    const e = CLASS_ERROR.exec(line);
    if (e) {
      classes.set(e[3], [...(classes.get(e[3]) ?? []), `${e[1]}:${e[2]}`]);
      continue;
    }
    const l = CLASS_LINK.exec(line);
    if (l) classes.set(l[1], [...(classes.get(l[1]) ?? []), '<link>']);
  }
  return {failed, classes};
}

// The class view, as build-engine prints it after a build.
export function formatClasses(log: string, options: {limit?: number; files?: boolean} = {}): string {
  const {failed, classes} = classify(log);
  const limit = options.limit ?? 40;
  const lines = [`${failed} FAILED edge(s), ${classes.size} distinct diagnostic(s)`];
  const ordered = [...classes.entries()].sort((a, b) => b[1].length - a[1].length).slice(0, limit);
  for (const [text, sites] of ordered) {
    lines.push(`${String(sites.length).padStart(4)}  ${text}`);
    if (options.files) {
      const unique = [...new Set(sites)].sort();
      for (const s of unique.slice(0, 8)) lines.push(`        ${s}`);
      if (unique.length > 8) lines.push(`        ... ${unique.length - 8} more`);
    }
  }
  return lines.join('\n');
}

// The per-edge view: every failure with its first diagnostic (or all of
// them), then a count per target.
export function formatFailures(log: string, options: {full?: boolean} = {}): string {
  const failures = parseFailures(log);
  if (failures.length === 0) return '';
  const byTarget = new Map<string, number>();
  const lines: string[] = [];
  for (const {object, block} of failures) {
    const target = object.includes('/') ? object.slice(0, object.lastIndexOf('/')) : object;
    byTarget.set(target, (byTarget.get(target) ?? 0) + 1);
    lines.push(`=== ${object}`);
    if (options.full) {
      for (const line of block) lines.push(`  ${line}`);
    } else {
      const where = whereOf(block);
      lines.push(`  ${where}${where ? ': ' : ''}${messageOf(block)}`);
    }
  }
  lines.push('');
  for (const [target, count] of [...byTarget.entries()].sort((a, b) => b[1] - a[1])) {
    lines.push(`${String(count).padStart(5)}  ${target}`);
  }
  lines.push(`${String(failures.length).padStart(5)}  TOTAL`);
  return lines.join('\n');
}

// The commonest kinds first, quoted parts collapsed.
export function formatKinds(log: string, top: number): string {
  const failures = parseFailures(log);
  if (failures.length === 0) return '';
  const kinds = new Map<string, number>();
  const examples = new Map<string, [string, string]>();
  for (const {object, block} of failures) {
    const msg = messageOf(block);
    const key = msg.replace(QUOTED, "'_'");
    kinds.set(key, (kinds.get(key) ?? 0) + 1);
    if (!examples.has(key)) examples.set(key, [object, msg]);
  }
  const lines: string[] = [];
  for (const [key, count] of [...kinds.entries()].sort((a, b) => b[1] - a[1]).slice(0, top)) {
    const [object, msg] = examples.get(key)!;
    lines.push(`${String(count).padStart(4)}  ${key}`);
    lines.push(`      e.g. ${object}: ${msg}`);
  }
  lines.push('', `${failures.length} failing edge(s), ${kinds.size} distinct kind(s)`);
  return lines.join('\n');
}
