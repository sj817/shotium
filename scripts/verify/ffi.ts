// Exercise the shipped C ABI from five real language runtimes, including failure
// stats and Unicode paths. Compare bytes against the CLI from that same delivery.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {copyFileSync, existsSync, mkdirSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';
import {cac} from 'cac';
import {execa} from 'execa';
import {resolve} from '../lib/repo.ts';

// The page every demo renders, at the viewport its README and the recorded CLI
// session use. The demos hard-code the same viewport, which is what makes
// "identical to the CLI" a byte comparison rather than a resemblance.
const FIXTURE = 'apps/demo-card/card.html';
const VIEWPORT = ['--width', '720', '--height', '380'];

interface Options {
  libraryDir: string;
  output: string;
  languages: string;
  maven: string;
  sourceDir?: string;
}

async function main(options: Options): Promise<void> {
  // A Maven given as a path is relative to the repository root like every other
  // path here; a bare name is looked up on PATH.
  const maven = /[\\/]/.test(options.maven) ? resolve(options.maven) : options.maven;
  const directory = resolve(options.libraryDir);
  const output = resolve(options.output);
  mkdirSync(output, {recursive: true});
  const fixtureDirectory = path.join(output, 'Unicode 路径');
  mkdirSync(fixtureDirectory, {recursive: true});
  const fixture = path.join(fixtureDirectory, 'card 示例.html');
  copyFileSync(resolve(FIXTURE), fixture);
  const win = process.platform === 'win32';
  const suffix = win ? '.exe' : '';
  const library = path.join(directory, win ? 'shotium.dll' : process.platform === 'darwin' ? 'libshotium.dylib' : 'libshotium.so');
  const run = (command: string, args: string[], cwd?: string) => execa(command, args, {cwd, timeout: 300000, env: {DOTNET_NOLOGO: '1'}});
  const baseline = path.join(output, 'cli.png');
  await run(path.join(directory, `shotium${suffix}`), ['--file', fixture, ...VIEWPORT, '-o', baseline]);
  const hash = (file: string) => createHash('sha256').update(readFileSync(file)).digest('hex');
  const expected = hash(baseline);
  const results: {language: string; imageSha256: string; failureStats: boolean}[] = [];
  const languages = options.languages.split(',');
  if (options.sourceDir && languages.length !== 1) throw new Error('--source-dir applies to exactly one --languages entry');
  for (const language of languages) {
    // A demo lives in apps/<language>; an extracted language ZIP is that same
    // directory under another name, which is all --source-dir points at.
    const source = (...parts: string[]) => options.sourceDir ? resolve(options.sourceDir, ...parts) : resolve('apps', language, ...parts);
    let command: string;
    let args: string[] = [];
    switch (language) {
      case 'python':
        command = win ? 'python' : 'python3';
        args = [source('screenshot.py')];
        break;
      case 'go':
        command = path.join(output, `go-demo${suffix}`);
        await run('go', ['build', '-mod=readonly', '-o', command, '.'], source());
        break;
      case 'rust':
        await run('cargo', ['build', '--release', '--locked', '--manifest-path', source('Cargo.toml')]);
        command = source('target', 'release', `shotium-demo${suffix}`);
        break;
      case 'csharp':
        await run('dotnet', ['build', source('ShotiumDemo.csproj'), '-c', 'Release', '--nologo']);
        command = 'dotnet';
        args = [source('bin', 'Release', 'net8.0', 'ShotiumDemo.dll')];
        break;
      case 'java':
        if (win) {
          // mvn.cmd re-parses its arguments through cmd.exe; hand them over via
          // PowerShell instead, so spaces and Unicode survive both launchers.
          await execa('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command',
            '& $env:SHOT_MAVEN -f $env:SHOT_POM package dependency:copy-dependencies -B -q; if (-not $?) { exit 1 }; exit $LASTEXITCODE'], {
            timeout: 300000, env: {SHOT_MAVEN: path.normalize(maven), SHOT_POM: source('pom.xml')},
          });
        } else {
          await run(maven, ['-f', source('pom.xml'), 'package', 'dependency:copy-dependencies', '-B', '-q']);
        }
        command = 'java';
        args = ['-cp', `${source('target', 'classes')}${path.delimiter}${source('target', 'dependency', '*')}`, 'ShotiumDemo'];
        break;
      default: throw new Error(`unknown language: ${language}`);
    }
    const png = path.join(output, `${language}.png`);
    const captureArgs = (input: string, destination: string) => {
      if (language !== 'java') return [...args, directory, input, destination];
      const config = path.join(output, 'java-config.json');
      writeFileSync(config, JSON.stringify({libraryDir: directory, input, output: destination}));
      return [...args, '--config', config];
    };
    const success = await run(command, captureArgs(fixture, png));
    assert.equal(hash(png), expected, `${language}: image differs from CLI`);
    assert.match(success.stdout, /"timing"/, `${language}: missing success stats`);
    const failedOutput = path.join(output, `${language}-failure-${process.pid}.png`);
    const failure = await execa(command, captureArgs(path.join(fixtureDirectory, 'missing.html'), failedOutput), {timeout: 30000, reject: false});
    assert.notEqual(failure.exitCode, 0, `${language}: missing file succeeded`);
    assert.match(failure.stderr, /capture failed \(2\)/, `${language}: missing capture error`);
    assert.match(failure.stdout, /"timing"/, `${language}: missing failure stats`);
    assert.equal(existsSync(failedOutput), false, `${language}: wrote an image on failure`);
    results.push({language, imageSha256: expected, failureStats: true});
    console.log(`PASS ${language}: CLI-identical image, Unicode input, capture error and failure stats`);
  }
  writeFileSync(path.join(output, 'report.json'), JSON.stringify({platform: process.platform, arch: process.arch, library, librarySha256: hash(library), results}, null, 2) + '\n');
}

const cli = cac('pnpm verify:ffi');
cli.command('', 'verify C ABI language demos; paths resolve against the repository root')
    .option('--library-dir <dir>', 'extracted C ABI delivery or engine build', {default: 'out/Shot'})
    .option('--output <dir>', 'evidence directory', {default: 'out/ffi-check'})
    .option('--source-dir <dir>', 'one language demo directory, such as an extracted language ZIP (default apps/<language>)')
    .option('--languages <list>', 'comma-separated language list', {default: 'python,go,rust,csharp,java'})
    .option('--maven <command>', 'Maven executable', {default: process.platform === 'win32' ? 'mvn.cmd' : 'mvn'})
    .action(async (options: Options) => {
      try { await main(options); } catch (error) { console.error(error); process.exitCode = 1; }
    });
cli.help();
cli.parse();
