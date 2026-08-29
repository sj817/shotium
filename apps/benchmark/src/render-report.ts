import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {parseArgs, recoverNpmRunValues} from './args.ts';
import {PLATFORM_IDS} from './constants.ts';
import {renderLatest, renderReport, renderSummaryCsv} from './report.ts';

function readJson(file: string): any {
  return JSON.parse(fs.readFileSync(file, 'utf8'));
}

function writeDerivedFile(file: string, content: string): void {
  const temporary = `${file}.tmp-${process.pid}`;
  const backup = `${file}.bak-${process.pid}`;
  fs.writeFileSync(temporary, content);
  const existed = fs.existsSync(file);
  let backupMade = false;
  try {
    if (existed) {
      fs.renameSync(file, backup);
      backupMade = true;
    }
    fs.renameSync(temporary, file);
  } catch (error) {
    fs.rmSync(temporary, {force: true});
    if (backupMade && fs.existsSync(backup) && !fs.existsSync(file)) {
      try {
        fs.renameSync(backup, file);
      } catch (restoreError) {
        throw new AggregateError([error, restoreError],
            `could not replace ${file}; original remains at ${backup}`);
      }
    }
    throw error;
  } finally {
    if (fs.existsSync(file)) fs.rmSync(backup, {force: true});
  }
}

export function renderArchivedResult(resultDirectory: string) {
  const directory = path.resolve(resultDirectory);
  const manifestFile = path.join(directory, 'manifest.json');
  if (!fs.existsSync(manifestFile)) throw new Error(`missing archived manifest: ${manifestFile}`);
  const manifest = readJson(manifestFile);
  if (!manifest || typeof manifest !== 'object' || !Array.isArray(manifest.platforms) ||
      typeof manifest.shotium_version !== 'string') {
    throw new Error('archived manifest is not a benchmark manifest');
  }
  const platforms: any[] = [];
  for (const platform of PLATFORM_IDS) {
    const summaryFile = path.join(directory, platform, 'summary.json');
    if (!fs.existsSync(summaryFile)) continue;
    const summary = readJson(summaryFile);
    if (summary?.platform !== platform || !Array.isArray(summary.scenarios) || !Array.isArray(summary.engines)) {
      throw new Error(`archived ${platform} summary has an invalid identity or report payload`);
    }
    platforms.push(summary);
  }
  if (!platforms.length) throw new Error('archived result contains no platform summaries to render');
  const outputs = {
    report: renderReport(platforms, manifest, 'en'),
    reportZh: renderReport(platforms, manifest, 'zh-CN'),
    csv: renderSummaryCsv(platforms),
  };
  writeDerivedFile(path.join(directory, 'report.md'), outputs.report);
  writeDerivedFile(path.join(directory, 'report.zh-CN.md'), outputs.reportZh);
  writeDerivedFile(path.join(directory, 'summary.csv'), outputs.csv);

  const resultsRoot = path.dirname(path.dirname(directory));
  const indexFile = path.join(resultsRoot, 'index.json');
  if (fs.existsSync(indexFile)) {
    const index = readJson(indexFile);
    if (Array.isArray(index?.results)) {
      writeDerivedFile(path.join(resultsRoot, 'LATEST.md'), renderLatest(index.results[0]));
    }
  }
  return {directory, platforms: platforms.map((platform) => platform.platform)};
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const options = parseArgs(process.argv.slice(2));
  recoverNpmRunValues(options, ['resultDirectory']);
  if (!options.resultDirectory) {
    throw new Error('usage: render-report --result-directory <archived-result-directory>');
  }
  const result = renderArchivedResult(String(options.resultDirectory));
  process.stdout.write(`${JSON.stringify(result)}\n`);
}
