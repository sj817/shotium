// Exercise the shipped C ABI from five real language runtimes, including failure
// stats and Unicode paths. Compare bytes against the CLI from that same delivery.
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {copyFileSync, existsSync, mkdirSync, readFileSync, writeFileSync} from 'node:fs';
import path from 'node:path';
import {cac} from 'cac';
import {execa} from 'execa';
import {resolve} from './lib/repo.ts';

async function main(options: {libraryDir: string; output: string; languages: string; maven: string}): Promise<void> {
  const directory = resolve(options.libraryDir);
  const output = resolve(options.output);
  mkdirSync(output, {recursive: true});
  const fixtureDirectory = path.join(output, 'Unicode 路径');
  mkdirSync(fixtureDirectory, {recursive: true});
  const fixture = path.join(fixtureDirectory, 'hello 示例.html');
  copyFileSync(resolve('apps/fixtures/hello.html'), fixture);
  const suffix = process.platform === 'win32' ? '.exe' : '';
  const library = path.join(directory, process.platform === 'win32' ? 'shotium.dll' : process.platform === 'darwin' ? 'libshotium.dylib' : 'libshotium.so');
  const run = (command: string, args: string[], cwd?: string) => execa(command, args, {cwd, timeout: 300000, env: {DOTNET_NOLOGO: '1'}});
  const baseline = path.join(output, 'cli.png');
  await run(path.join(directory, `shotium${suffix}`), ['--file', fixture, '--width', '800', '--height', '600', '-o', baseline]);
  const hash = (file: string) => createHash('sha256').update(readFileSync(file)).digest('hex');
  const expected = hash(baseline);
  const results: {language: string; imageSha256: string; failureStats: boolean}[] = [];
  for (const language of options.languages.split(',')) {
    let command: string;
    let args: string[] = [];
    switch (language) {
      case 'python':
        command = process.platform === 'win32' ? 'python' : 'python3';
        args = [resolve('apps/python/screenshot.py')];
        break;
      case 'go':
        command = path.join(output, `go-demo${suffix}`);
        await run('go', ['build', '-mod=readonly', '-o', command, '.'], resolve('apps/go'));
        break;
      case 'rust':
        await run('cargo', ['build', '--release', '--locked', '--manifest-path', resolve('apps/rust/Cargo.toml')]);
        command = resolve(`apps/rust/target/release/shotium-demo${suffix}`);
        break;
      case 'csharp':
        await run('dotnet', ['build', resolve('apps/csharp/ShotiumDemo.csproj'), '-c', 'Release', '--nologo']);
        command = 'dotnet';
        args = [resolve('apps/csharp/bin/Release/net8.0/ShotiumDemo.dll')];
        break;
      case 'java':
        if (process.platform === 'win32') {
          // Maven's batch launcher reparses cmd escaping. Pass paths as data to
          // PowerShell instead, so spaces and Unicode survive both launchers.
          await execa('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command',
            '& $env:SHOT_MAVEN -f $env:SHOT_POM package dependency:copy-dependencies -B -q; exit $LASTEXITCODE'], {
            timeout: 300000, env: {SHOT_MAVEN: path.normalize(options.maven), SHOT_POM: resolve('apps/java/pom.xml')},
          });
        } else {
          await run(options.maven, ['-f', resolve('apps/java/pom.xml'), 'package', 'dependency:copy-dependencies', '-B', '-q']);
        }
        command = 'java';
        args = ['-cp', `${resolve('apps/java/target/classes')}${path.delimiter}${resolve('apps/java/target/dependency/*')}`, 'ShotiumDemo'];
        break;
      default: throw new Error(`unknown language: ${language}`);
    }
    const png = path.join(output, `${language}.png`);
    const success = await run(command, [...args, directory, fixture, png]);
    assert.equal(hash(png), expected, `${language}: image differs from CLI`);
    assert.match(success.stdout, /"timing"/, `${language}: missing success stats`);
    const failedOutput = path.join(output, `${language}-failure-${process.pid}.png`);
    const failure = await execa(command, [...args, directory, path.join(fixtureDirectory, 'missing.html'), failedOutput], {timeout: 30000, reject: false});
    assert.notEqual(failure.exitCode, 0, `${language}: missing file succeeded`);
    assert.match(failure.stderr, /capture failed \(2\)/, `${language}: missing capture error`);
    assert.match(failure.stdout, /"timing"/, `${language}: missing failure stats`);
    assert.equal(existsSync(failedOutput), false, `${language}: wrote an image on failure`);
    results.push({language, imageSha256: expected, failureStats: true});
    console.log(`PASS ${language}: CLI-identical image, Unicode input, capture error and failure stats`);
  }
  writeFileSync(path.join(output, 'report.json'), JSON.stringify({platform: process.platform, arch: process.arch, library, librarySha256: hash(library), results}, null, 2) + '\n');
}
const cli = cac('check-ffi');
cli.command('', 'verify C ABI language demos; paths resolve against the repository root')
    .option('--library-dir <dir>', 'extracted C ABI delivery or engine build', {default: 'out/Shot'})
    .option('--output <dir>', 'evidence directory', {default: 'out/ffi-check'})
    .option('--languages <list>', 'comma-separated language list', {default: 'python,go,rust,csharp,java'})
    .option('--maven <command>', 'Maven executable', {default: process.platform === 'win32' ? 'mvn.cmd' : 'mvn'})
    .action(async (options: {libraryDir: string; output: string; languages: string; maven: string}) => {
      try { await main(options); } catch (error) { console.error(error); process.exitCode = 1; }
    });
cli.help();
cli.parse();
