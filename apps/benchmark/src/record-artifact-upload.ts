import fs from 'node:fs';
import path from 'node:path';
import {booleanArg, parseArgs, recoverNpmRunValues} from './args.ts';

const options = parseArgs(process.argv.slice(2));
recoverNpmRunValues(options, [
  'record', 'uploaded', 'artifactId', 'artifactUrl', 'artifactDigest',
]);

if (!options.record) {
  throw new Error('usage: record-artifact-upload --record <artifact.json> --uploaded <true|false>');
}

const recordFile = path.resolve(String(options.record));
if (!fs.existsSync(recordFile)) throw new Error(`artifact record does not exist: ${recordFile}`);
const record = JSON.parse(fs.readFileSync(recordFile, 'utf8'));
const uploaded = booleanArg(options.uploaded, false);
Object.assign(record, {
  uploaded,
  upload_recorded_utc: new Date().toISOString(),
  actions_artifact_id: uploaded && options.artifactId ? String(options.artifactId) : null,
  actions_artifact_url: uploaded && options.artifactUrl ? String(options.artifactUrl) : null,
  actions_artifact_digest: uploaded && options.artifactDigest ? String(options.artifactDigest) : null,
});
fs.writeFileSync(recordFile, `${JSON.stringify(record, null, 2)}\n`);
process.stdout.write(`${uploaded ? 'uploaded' : 'upload-failed'}\n`);
