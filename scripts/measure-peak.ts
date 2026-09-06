// Peak memory of one shotium run, and the scenario list the memory work used.
//
//   pnpm measure-peak run <label> <exe> [args...]     # one command
//   pnpm measure-peak scenarios [name]               # tiny | b1v | b1 | b1j | b2 | b2t | b2j | b2w | all
//
// `run` prints peak working set, peak private bytes, wall time and the exit
// code, one line per run. Node cannot read another process's memory counters,
// so the platform's own record is used: on Windows the .NET Process object's
// PeakWorkingSet64 / PeakPagedMemorySize64 after the process exits, on Linux
// and macOS `/usr/bin/time -v` (max RSS; private bytes are reported as the
// same figure, which is the closest that interface offers). Report the two
// separately; discardable shared segments only appear in the working set.
//
// With SHOT_PROFILE=1 and --verbose the engine logs "shot: mem <stage>" lines;
// PEAK_VERBOSE=1 prints them under each run.
//
// `scenarios` is the list from the memory work: a tiny page, and the two
// Bilibili articles that are the reference cases, as viewport, full page,
// tiles, JPEG and WebP. MEASURE_DIR, EXE, B1 and B2 override the paths.

import {existsSync, mkdirSync, writeFileSync} from 'node:fs';
import path from 'node:path';

import {cac} from 'cac';
import {execa} from 'execa';

import {powershell} from './lib/measured-process.ts';
import {resolve} from './lib/repo.ts';

interface Peak {
  peakWorkingSet: number;
  peakPrivate: number;
  wallMs: number;
  exitCode: number;
  stderr: string;
}

async function measure(exe: string, args: string[], env: NodeJS.ProcessEnv): Promise<Peak> {
  const started = Date.now();
  if (process.platform === 'win32') {
    // The .NET Process object is polled every 2 ms while the process runs:
    // its counters stop being readable the moment it exits. stderr goes to a
    // file so the engine's own log survives.
    const errFile = path.join(process.env.TEMP ?? '.', `measure-peak-${process.pid}.err`);
    const script = [
      '$psi = New-Object System.Diagnostics.ProcessStartInfo',
      `$psi.FileName = ${ps(exe)}`,
      ...args.map((a) => `$psi.ArgumentList.Add(${ps(a)})`),
      '$psi.UseShellExecute = $false',
      '$psi.RedirectStandardError = $true',
      '$psi.RedirectStandardOutput = $true',
      '$p = [System.Diagnostics.Process]::Start($psi)',
      '$peakWs = 0; $peakPriv = 0',
      '$errTask = $p.StandardError.ReadToEndAsync()',
      '$outTask = $p.StandardOutput.ReadToEndAsync()',
      'while (-not $p.HasExited) {',
      '  try { $p.Refresh(); if ($p.PeakWorkingSet64 -gt $peakWs) { $peakWs = $p.PeakWorkingSet64 }; if ($p.PrivateMemorySize64 -gt $peakPriv) { $peakPriv = $p.PrivateMemorySize64 } } catch {}',
      '  Start-Sleep -Milliseconds 2',
      '}',
      `[System.IO.File]::WriteAllText(${ps(errFile)}, $errTask.Result)`,
      'Write-Output ("{0} {1} {2}" -f $peakWs, $peakPriv, $p.ExitCode)',
    ].join('\n');
    const result = await execa(powershell(), ['-NoProfile', '-Command', script], {env, reject: false});
    const [ws, priv, code] = result.stdout.trim().split(/\s+/).map(Number);
    const {readFileSync, rmSync} = await import('node:fs');
    const stderr = existsSync(errFile) ? readFileSync(errFile, 'utf8') : '';
    rmSync(errFile, {force: true});
    if (result.exitCode !== 0 || Number.isNaN(code)) console.error(result.stderr);
    return {peakWorkingSet: ws || 0, peakPrivate: priv || 0, wallMs: Date.now() - started, exitCode: Number.isNaN(code) ? -1 : code, stderr};
  }
  const result = await execa('/usr/bin/time', ['-v', exe, ...args], {env, reject: false, all: true});
  const m = /Maximum resident set size \(kbytes\): (\d+)/.exec(result.all ?? '');
  const rss = m ? Number(m[1]) * 1024 : 0;
  return {peakWorkingSet: rss, peakPrivate: rss, wallMs: Date.now() - started, exitCode: result.exitCode ?? -1, stderr: result.all ?? ''};
}

function ps(value: string): string {
  return `'${value.replace(/'/g, "''")}'`;
}

async function run(label: string, exe: string, args: string[]): Promise<number> {
  const peak = await measure(exe, args, {...process.env, SHOT_PROFILE: process.env.SHOT_PROFILE ?? '1'});
  const mb = (n: number) => Math.round(n / 1048576).toLocaleString('en-US').padStart(6);
  console.log(`${label.padEnd(28)} peak_ws=${mb(peak.peakWorkingSet)} MB  peak_private=${mb(peak.peakPrivate)} MB  wall=${String(peak.wallMs).padStart(6)} ms  exit=${peak.exitCode}`);
  if (process.env.PEAK_VERBOSE) {
    for (const line of peak.stderr.split(/\r?\n/)) {
      if (/shot: (mem|banded|raster)|ERROR|error/.test(line)) console.log(`    ${line.trim()}`);
    }
  }
  return peak.exitCode;
}

async function scenarios(only: string): Promise<number> {
  const dir = process.env.MEASURE_DIR ?? path.join(process.env.TEMP ?? '/tmp', 'shot-measure');
  mkdirSync(dir, {recursive: true});
  const tiny = path.join(dir, 'tiny.html');
  if (!existsSync(tiny)) writeFileSync(tiny, '<!doctype html><body style="margin:0;background:#fff"><p style="font:16px sans-serif">hello</p></body>');
  const exe = process.env.EXE ?? resolve('out/Shot', process.platform === 'win32' ? 'shotium.exe' : 'shotium');
  const b1 = process.env.B1 ?? 'D:/Downloads/bilibili_dynamic_DYNAMIC_TYPE_ARTICLE_1788403871415.html';
  const b2 = process.env.B2 ?? 'D:/Downloads/bilibili_dynamic_DYNAMIC_TYPE_ARTICLE_1788434008828.html';
  const out = (name: string) => path.join(dir, name);
  const list: Array<[string, string, string[]]> = [
    ['tiny', 'tiny viewport', ['--file', tiny, '--width', '1440', '--verbose', '--output', out('m_tiny.png')]],
    ['b1v', 'bili1 viewport', ['--file', b1, '--width', '1440', '--verbose', '--output', out('m_b1v.png')]],
    ['b1', 'bili1 fullPage png', ['--file', b1, '--width', '1440', '--full-page', '--verbose', '--output', out('m_b1.png')]],
    ['b1j', 'bili1 fullPage jpeg', ['--file', b1, '--width', '1440', '--full-page', '--type', 'jpeg', '--verbose', '--output', out('m_b1.jpg')]],
    ['b2', 'bili2 fullPage png', ['--file', b2, '--width', '1440', '--full-page', '--verbose', '--output', out('m_b2.png')]],
    ['b2t', 'bili2 tiles 8000', ['--file', b2, '--width', '1440', '--full-page', '--tile-height', '8000', '--verbose', '--output', out('m_b2t.png')]],
    ['b2j', 'bili2 fullPage jpeg', ['--file', b2, '--width', '1440', '--full-page', '--type', 'jpeg', '--verbose', '--output', out('m_b2.jpg')]],
    ['b2w', 'bili2 webp', ['--file', b2, '--width', '1440', '--full-page', '--type', 'webp', '--scale', '0.35', '--verbose', '--output', out('m_b2.webp')]],
  ];
  let worst = 0;
  for (const [name, label, args] of list) {
    if (only !== 'all' && only !== name) continue;
    worst = Math.max(worst, await run(label, exe, args));
  }
  return worst;
}

// `run` hands everything after the executable to it untouched, so it is read
// off argv directly rather than through cac, which would take the engine's
// own --file and --width for options of its own.
if (process.argv[2] === 'run') {
  const [label, exe, ...args] = process.argv.slice(3);
  if (!label || !exe) {
    console.log('usage: pnpm measure-peak run <label> <exe> [args...]');
    process.exitCode = 2;
  } else {
    process.exitCode = await run(label, resolve(exe), args);
  }
} else {
  const cli = cac('measure-peak');
  cli.command('run <label> <exe> [...args]', 'peak memory of one command (arguments pass through untouched)');
  cli.command('scenarios [name]', 'the memory-work scenario list (default all)')
      .action(async (name: string | undefined) => {
        process.exitCode = await scenarios(name ?? 'all');
      });
  cli.help();
  cli.parse();
}
