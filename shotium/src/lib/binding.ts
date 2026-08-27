import fs from 'node:fs';
import {createRequire} from 'node:module';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

import * as platformPackage from './platform.js';

// A .node addon is a CommonJS artefact: there is no ESM loader for one.
const require = createRequire(import.meta.url);

// ESM has no __dirname. This is the same thing, from the module's own URL.
const HERE = path.dirname(fileURLToPath(import.meta.url));

/**
 * The engine handle the addon hands back. Opaque on purpose: everything that
 * can be done with it is a call on the binding below.
 */
export type Engine = unknown;

/** One capture's answer, as the addon hands it over. */
export interface NativeCapture {
  image: Buffer;
  /**
   * CaptureStats as JSON, unparsed. The addon carries JSON between the engine
   * and this layer without reading it -- anything it understood would be a
   * third opinion about the shape, and the third opinion is the one that
   * drifts. Undefined when the engine reported none.
   */
  stats?: string;
}

/** What native/binding.cc exports. See shot/shot_api.h for the C ABI. */
export interface NativeBinding {
  create(optionsJson: string): Engine;
  destroy(engine: Engine): void;
  purge(engine: Engine, releaseWorkingSet: boolean): void;
  status(engine: Engine): string;
  capture(engine: Engine, requestJson: string): Promise<NativeCapture>;
  /**
   * List or clear a cache directory. `engine` is nullable and that is the
   * interface: with one, the operation runs on the engine's thread and borrows
   * the backend it already holds; without one, the library opens the directory
   * itself. Resolves to JSON.
   */
  cache(engine: Engine|null, clearing: boolean, optionsJson: string):
      Promise<string>;
}

// Where the addon and the library beside it live.
//
// The platform package is what ships -- the .node sits next to the shared
// library it is linked against, which is the whole reason the two travel in
// one package rather than two. native/build/Release is where node-gyp puts a
// local build; it exists in a checkout and not in an install, so the two never
// compete in practice. Both paths are relative to this file's build output,
// which is one directory below the package root.
function candidates(): string[] {
  const found: string[] = [];
  const dir = platformPackage.packageDir();
  if (dir) {
    found.push(path.join(dir, 'shotium.node'));
  }
  found.push(
      path.join(HERE, '..', 'native', 'build', 'Release', 'shotium.node'));
  return found;
}

let binding: NativeBinding|null = null;
let loadedFrom: string|null = null;

/**
 * The addon, loaded once. Throws if there is none for this platform, which is
 * the only failure this package cannot work around: there is nothing else to
 * fall back to.
 */
export function load(): NativeBinding {
  if (binding) {
    return binding;
  }
  const tried = candidates();
  for (const candidate of tried) {
    if (!fs.existsSync(candidate)) {
      continue;
    }
    // Not wrapped in a try: a .node that is there and will not load is a
    // broken installation, and the loader's own message -- a missing
    // dependency, an architecture mismatch -- says more than anything that
    // could be substituted for it.
    binding = require(candidate) as NativeBinding;
    loadedFrom = path.dirname(candidate);
    return binding;
  }
  const expected = platformPackage.packageName();
  throw new Error(
      'shotium: no engine for this platform.\n' +
      `  looked in:\n    ${tried.join('\n    ')}\n` +
      (expected ?
           `  It ships in ${expected}, which npm installs as an optional ` +
               'dependency of this package. If the install skipped optional ' +
               'dependencies, it is not there.\n' :
           `  There is no build for ${process.platform}-${process.arch}.\n`));
}

/**
 * The directory the addon came from, or null before the first load(). The
 * resource packs ship beside it, which is what this is for.
 */
export function directory(): string|null {
  return loadedFrom;
}
