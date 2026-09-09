import fs from 'node:fs';
import {createRequire} from 'node:module';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

import type {CaptureStats, CacheEntry, CacheClearResult} from '../types.js';
import type {WireRequest} from './request.js';
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
  stats?: CaptureStats;
}

/** One tile of a tiles capture, as the addon hands it over. */
export interface NativeTile {
  image: Buffer;
  x: number;
  y: number;
  width: number;
  height: number;
  /** The file the engine wrote, when the request named a `path`. */
  path?: string;
}

/** A tiles capture's answer: the tiles in document order, and the stats. */
export interface NativeTiles {
  tiles: NativeTile[];
  stats?: CaptureStats;
}

/** Internal versioned Node-API contract; independent of the public C ABI. */
export interface NativeBinding {
  bindingVersion: number;
  create(options: Record<string, unknown>): Engine;
  destroy(engine: Engine): void;
  purge(engine: Engine, releaseWorkingSet: boolean): void;
  status(engine: Engine): {cacheDir: string|null; cacheActive: boolean};
  capture(engine: Engine, request: WireRequest): Promise<NativeCapture>;
  captureTiles(engine: Engine, request: WireRequest): Promise<NativeTiles>;
  cache(engine: Engine|null, clearing: boolean, options: Record<string, unknown>):
      Promise<Array<Omit<CacheEntry, 'dir'>>|Omit<CacheClearResult, 'dir'>>;
}

// GN's output is authoritative in a checkout. Installed packages only use
// their platform dependency; there is no stale node-gyp fallback.
function* candidates(): Generator<string> {
  // Resolve the platform package only when the local build is absent. In a
  // checkout, package resolution can traverse a large pnpm tree even though
  // its result will never be loaded.
  if (fs.existsSync(path.join(HERE, '..', '..', '..', '..', 'shot', 'BUILD.gn'))) {
    yield path.join(HERE, '..', '..', '..', '..', 'out', 'Shot', 'shotium.node');
  }
  const dir = platformPackage.packageDir();
  if (dir) {
    yield path.join(dir, 'shotium.node');
  }
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
  const tried: string[] = [];
  for (const candidate of candidates()) {
    tried.push(candidate);
    if (!fs.existsSync(candidate)) {
      continue;
    }
    // Not wrapped in a try: a .node that is there and will not load is a
    // broken installation, and the loader's own message -- a missing
    // dependency, an architecture mismatch -- says more than anything that
    // could be substituted for it.
    const loaded = require(candidate) as NativeBinding;
    if (loaded.bindingVersion !== 1) {
      throw new Error(`shotium: incompatible native binding at ${candidate}; rebuild shot_node or reinstall matching platform packages`);
    }
    binding = loaded;
    loadedFrom = path.dirname(candidate);
    return binding;
  }
  const expected = platformPackage.packageName();
  throw new Error(
      'shotium: no engine for this platform.\n' +
      `  looked in:\n    ${tried.join('\n    ')}\n` +
      (expected ?
           `  It ships in ${expected}, which pnpm installs as an optional ` +
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
