import {existsSync, statSync} from 'node:fs';
import path from 'node:path';

import {InputMtimes} from './input-mtimes.ts';

// These tracked inputs can be older than a restored header from another
// branch. Git commit times alone cannot tell ninja that its contents differ.
// Include overrides, templates and shared generator helpers, not only JSON5.
export function isRuntimeFeatureInput(relativePath: string): boolean {
  return relativePath === 'third_party/blink/renderer/platform/runtime_enabled_features.json5'
    || relativePath === 'third_party/blink/renderer/platform/runtime_enabled_features.override.json5'
    || (relativePath.startsWith('third_party/blink/renderer/build/scripts/')
        && /\.(py|tmpl)$/.test(relativePath));
}

export function createRuntimeFeatureMtimes(buildDir: string, headTime: number): InputMtimes {
  const log = path.join(buildDir, '.ninja_log');
  // Old caches have no record of the generator inputs. Invalidate them once,
  // even when checking out an older revision. All shards derive the same time
  // from the restored log, not from their independent wall clocks.
  const firstTime = Math.max(headTime, existsSync(log) ? statSync(log).mtimeMs / 1000 + 1 : headTime);
  return new InputMtimes(path.join(buildDir, 'ci-runtime-feature-inputs.json'), firstTime, headTime);
}
