import fs from 'node:fs';
import path from 'node:path';
import AjvModule from 'ajv/dist/2020.js';
import {APP_ROOT} from './constants.ts';

const Ajv = (AjvModule as any).default || AjvModule;
const ajv = new Ajv({allErrors: true});
const schema = JSON.parse(fs.readFileSync(path.join(APP_ROOT, 'schema', 'platform-result.schema.json'), 'utf8'));
const validate = ajv.compile(schema);

export function validatePlatformResult(result) {
  if (!validate(result)) {
    throw new Error(`platform result schema failed: ${ajv.errorsText(validate.errors, {separator: '; '})}`);
  }
  return result;
}
