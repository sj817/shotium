// Attribute shotium.exe's bytes to the code that produced them.
//
// The question "what is in the binary" cannot be answered from the GN graph.
// A target being in the dependency closure does not mean it contributed
// bytes: /OPT:REF drops sections nothing references and /OPT:ICF folds
// identical ones, so the only authority on what survived is the linker's own
// record.
//
// This reads that record out of the PDB rather than re-linking with /MAP. The
// PDB's section-contribution table is written *after* the layout is decided,
// so every entry in it is a range of the final image, named by the object
// file it came from. symbol_level = 0 is enough -- contributions are not
// debug info.
//
//   pnpm size:report out/Shot/shotium.exe [--depth 3] [--top 40] [--detail]
//                    [--by-object SUBSTRING] [--csv table.csv]
//
// Reported per component, where a component is the source directory of the
// object file (obj/<dir>/<target>/<file>.obj -> <dir>), rolled up to --depth
// for the headline and printed in full with --detail. The sum of
// contributions is smaller than the file on disk, and the difference is
// reported rather than hidden: the import and relocation tables, resources,
// section padding and the PE headers are made by the linker and belong to no
// object file. Windows only: it reads a PDB.

import {existsSync, readFileSync, statSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';

import {resolve, root} from './lib/repo.ts';

// `SC[.text]  | mod = 12, 0001:0000, size = 68, data crc = ..., reloc crc = ...`
const SC_RE = /SC\[([^\]]+)\]\s*\|\s*mod = (\d+), ([0-9a-fA-F]+):([0-9a-fA-F]+), size = (\d+)/;
// `Mod 0000 | `o:\fake\prefix\obj\shot\shot\main.obj`:`
const MOD_RE = /^\s*Mod (\d+) \| `([^`]*)`/;

function findPdbutil(): string {
  const exe = process.platform === 'win32' ? 'llvm-pdbutil.exe' : 'llvm-pdbutil';
  const tool = resolve('third_party/llvm-build/Release+Asserts/bin', exe);
  if (existsSync(tool)) return tool;
  throw new Error(`could not find ${tool} -- is the clang toolchain unpacked?`);
}

// Section sizes straight from the PE header, as the denominator. The
// contribution table is a claim about the image; this is what the image
// actually is.
function peSections(binary: string): Array<[string, number, number]> {
  const data = readFileSync(binary);
  if (data.subarray(0, 2).toString('latin1') !== 'MZ') throw new Error(`${binary} is not a PE image`);
  const pe = data.readUInt32LE(0x3c);
  if (data.subarray(pe, pe + 4).toString('latin1') !== 'PE\0\0') throw new Error(`${binary} has no PE signature`);
  const nsections = data.readUInt16LE(pe + 6);
  const optSize = data.readUInt16LE(pe + 20);
  const table = pe + 24 + optSize;
  const out: Array<[string, number, number]> = [];
  for (let i = 0; i < nsections; i++) {
    const entry = table + i * 40;
    const name = data.subarray(entry, entry + 8).toString('latin1').replace(/\0+$/, '');
    out.push([name, data.readUInt32LE(entry + 8), data.readUInt32LE(entry + 16)]);
  }
  return out;
}

// obj/<dir>/<target>/<file>.obj -> <dir>; everything else kept verbatim. The
// target segment is dropped because it is a build-system name, not a place
// in the source tree.
function componentOf(module: string): string {
  const p = module.replace(/\\/g, '/');
  const idx = p.toLowerCase().lastIndexOf('/obj/');
  if (idx < 0) {
    const base = path.basename(p);
    return `[${base || p}]`;
  }
  const parts = p.slice(idx + 5).split('/');
  if (parts.length <= 2) return parts[0] ?? '[?]';
  return parts.slice(0, -2).join('/');
}

const rollUp = (component: string, depth: number) => component.startsWith('[') ? component : component.split('/').slice(0, depth).join('/');
const commas = (n: number) => n.toLocaleString('en-US');

async function main(binaryArg: string, o: {depth: number; top: number; detail: boolean; byObject?: string; csv?: string}): Promise<number> {
  const binary = resolve(binaryArg);
  let pdb = `${binary}.pdb`;
  if (!existsSync(pdb)) pdb = binary.replace(/\.[^.]+$/, '') + '.pdb';
  if (!existsSync(pdb)) throw new Error(`no PDB beside ${binaryArg}`);
  const tool = findPdbutil();

  process.stderr.write('reading modules...\n');
  const modules = new Map<number, string>();
  const mods = await execa(tool, ['dump', '--modules', pdb], {cwd: root, reject: false, maxBuffer: 1 << 30, stderr: 'ignore'});
  for (const line of mods.stdout.split(/\r?\n/)) {
    const m = MOD_RE.exec(line);
    if (m) modules.set(Number(m[1]), m[2]);
  }

  process.stderr.write(`reading ${modules.size} modules' section contributions...\n`);
  const sizes = new Map<string, number>();  // "component\0section" -> bytes
  const perSection = new Map<string, number>();
  const files = new Map<string, Set<number>>();
  const seen = new Set<string>();
  let dupes = 0;
  const contribs = await execa(tool, ['dump', '--section-contribs', pdb], {cwd: root, reject: false, maxBuffer: 1 << 30, stderr: 'ignore'});
  for (const line of contribs.stdout.split(/\r?\n/)) {
    const m = SC_RE.exec(line);
    if (!m) continue;
    const [, section, mod, sectIdx, offset, sizeText] = m;
    const size = Number(sizeText);
    if (size === 0) continue;
    // A contribution is a range of the image. Two entries at the same
    // address would be double counting.
    const key = `${parseInt(sectIdx, 16)}:${parseInt(offset, 16)}`;
    if (seen.has(key)) {
      dupes++;
      continue;
    }
    seen.add(key);
    const raw = modules.get(Number(mod)) ?? `[unknown module ${mod}]`;
    let comp: string;
    if (o.byObject !== undefined) {
      const p = raw.replace(/\\/g, '/');
      if (!p.includes(o.byObject)) continue;
      const idx = p.toLowerCase().lastIndexOf('/obj/');
      comp = idx >= 0 ? p.slice(idx + 5) : p;
    } else {
      comp = componentOf(raw);
    }
    const k = `${comp}\0${section}`;
    sizes.set(k, (sizes.get(k) ?? 0) + size);
    perSection.set(section, (perSection.get(section) ?? 0) + size);
    files.set(comp, (files.get(comp) ?? new Set()).add(Number(mod)));
  }

  const totals = new Map<string, number>();
  for (const [k, size] of sizes) {
    const comp = k.split('\0')[0];
    totals.set(comp, (totals.get(comp) ?? 0) + size);
  }
  const attributed = [...totals.values()].reduce((s, v) => s + v, 0);
  const onDisk = statSync(binary).size;
  const mb = (n: number) => (n / 1048576).toFixed(2).padStart(8);

  console.log('');
  console.log('shotium.exe size composition');
  console.log('='.repeat(78));
  console.log(`binary            ${binaryArg}`);
  console.log(`on disk           ${commas(onDisk).padStart(14)} bytes  ${mb(onDisk)} MB`);
  console.log(`attributed        ${commas(attributed).padStart(14)} bytes  ${mb(attributed)} MB  (${(100 * attributed / onDisk).toFixed(1)}% of the file)`);
  console.log(`unattributed      ${commas(onDisk - attributed).padStart(14)} bytes  ${mb(onDisk - attributed)} MB  (import/reloc tables,`);
  console.log(`                  ${''.padStart(14)}         ${''.padStart(8)}   resources, padding, PE headers)`);
  if (dupes) console.log(`overlapping contributions skipped: ${dupes}`);

  console.log('');
  console.log("PE sections (the file's own account)");
  console.log('-'.repeat(78));
  console.log(`${'section'.padEnd(12)} ${'virtual'.padStart(16)} ${'raw'.padStart(16)} ${'attributed'.padStart(16)}`);
  for (const [name, vsize, rawsize] of peSections(binary)) {
    console.log(`${name.padEnd(12)} ${commas(vsize).padStart(16)} ${commas(rawsize).padStart(16)} ${commas(perSection.get(name) ?? 0).padStart(16)}`);
  }

  const rolled = new Map<string, number>(), rolledFiles = new Map<string, number>();
  for (const [comp, size] of totals) rolled.set(rollUp(comp, o.depth), (rolled.get(rollUp(comp, o.depth)) ?? 0) + size);
  for (const [comp, mods] of files) rolledFiles.set(rollUp(comp, o.depth), (rolledFiles.get(rollUp(comp, o.depth)) ?? 0) + mods.size);

  const table = (title: string, items: Array<[string, number]>, counts: Map<string, number>) => {
    console.log('');
    console.log(title);
    console.log('-'.repeat(78));
    console.log(`${'component'.padEnd(64)} ${'bytes'.padStart(14)} ${'MB'.padStart(7)} ${'% bin'.padStart(6)}`);
    let shown = 0;
    for (const [comp, size] of [...items].sort((a, b) => b[1] - a[1]).slice(0, o.top)) {
      console.log(`${comp.slice(0, 64).padEnd(64)} ${commas(size).padStart(14)} ${(size / 1048576).toFixed(2).padStart(7)} ${(100 * size / onDisk).toFixed(1).padStart(5)}%   ${counts.get(comp) ?? 0} objs`);
      shown += size;
    }
    const rest = items.reduce((s, [, v]) => s + v, 0) - shown;
    if (rest > 0) console.log(`${'(everything else)'.padEnd(64)} ${commas(rest).padStart(14)} ${(rest / 1048576).toFixed(2).padStart(7)} ${(100 * rest / onDisk).toFixed(1).padStart(5)}%`);
  };
  table(`by component, depth ${o.depth}`, [...rolled.entries()], rolledFiles);
  if (o.detail) table('by source directory', [...totals.entries()], new Map([...files.entries()].map(([c, m]) => [c, m.size])));

  if (o.csv) {
    const lines = ['component,section,bytes'];
    for (const [k, size] of [...sizes.entries()].sort((a, b) => b[1] - a[1])) {
      const [comp, section] = k.split('\0');
      lines.push(`"${comp}","${section}",${size}`);
    }
    writeFileSync(resolve(o.csv), lines.join('\n') + '\n');
    process.stderr.write(`wrote ${o.csv}\n`);
  }
  return 0;
}

const cli = cac('size-report');
cli.command('<binary>', 'attribute the image bytes to source directories through the PDB')
    .option('--depth <n>', 'directory depth for the headline rollup', {default: 3})
    .option('--top <n>', 'rows to print', {default: 40})
    .option('--detail', 'also print every component at full directory depth')
    .option('--by-object <substring>', 'list the individual object files whose path contains this instead of a rollup')
    .option('--csv <file>', 'write the full per-component table here')
    .action(async (binary: string, options: {depth: number; top: number; detail?: boolean; byObject?: string; csv?: string}) => {
      try {
        process.exitCode = await main(binary, {depth: Number(options.depth), top: Number(options.top), detail: options.detail === true, byObject: options.byObject, csv: options.csv});
      } catch (error) {
        console.error(error instanceof Error ? error.message : String(error));
        process.exitCode = 1;
      }
    });
cli.help();
cli.parse();
