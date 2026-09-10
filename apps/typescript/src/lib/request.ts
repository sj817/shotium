import type {
  CacheMode,
  Clip,
  PageGotoParams,
  ScreenshotOptions,
  ScreenshotTilesOptions,
  TileOptions,
} from '../types.js';

const DEFAULT_TIMEOUT_MS = 30000;
const MAX_TILE_HEIGHT = 32000;

// What actually goes down the pipe: ScreenshotOptions with the viewport
// flattened -- see toRequest below for why.
export interface WireRequest {
  file: string;
  type?: 'png'|'jpeg'|'webp';
  fullPage?: boolean;
  selector?: string;
  quality?: number;
  scale?: number;
  omitBackground?: boolean;
  path?: string;
  pageGotoParams?: PageGotoParams;
  clip?: Clip;
  allowFileAccess?: boolean;
  cache?: CacheMode;
  headers?: Record<string, string>;
  tile?: TileOptions;
  width?: number;
  height?: number;
}

// Everything the worker understands, and nothing else. An unknown field is a
// typo, and a typo that is silently dropped is a screenshot that quietly
// ignored what was asked for -- so this rejects rather than filters.
//
// It is a runtime check even though the argument has a type, because the
// argument having a type says nothing about a caller who is not compiled
// against it: a JavaScript program, or a JSON body from somewhere else.
const WIRE_FIELDS = new Set([
  'file',
  'type',
  'fullPage',
  'selector',
  'quality',
  'scale',
  'omitBackground',
  'path',
  'pageGotoParams',
  'clip',
  'viewport',
  'allowFileAccess',
  'cache',
  'headers',
  'tile',
]);

// One ScreenshotOptions, checked and flattened into what goes on the wire.
//
// It lives here rather than in index.ts because the engine in this process and
// the daemon both send it: a request that is valid through one entry point and
// rejected through the other would be a difference nobody asked for.
function toRequest(options: ScreenshotOptions): WireRequest {
  if (!options || typeof options !== 'object') {
    throw new TypeError('shotium: screenshot(options) needs an object');
  }
  if (typeof options.file !== 'string' || options.file.length === 0) {
    throw new TypeError('shotium: options.file is required');
  }
  // A tiles request answers with a list and a screenshot with one image; the
  // two are different calls, and a caller who put `tile` on screenshot() is
  // told which one they wanted rather than handed the first tile.
  if ('tile' in options && (options as ScreenshotTilesOptions).tile !==
                                undefined) {
    throw new TypeError(
        'shotium: tile is an option of screenshotTiles(), not screenshot()');
  }
  return toWire(options);
}

/**
 * One ScreenshotTilesOptions, checked and flattened. The same checks as
 * toRequest and one more: `tile.height` has to be there, because without it
 * there is nothing to cut by.
 */
function toTilesRequest(options: ScreenshotTilesOptions): WireRequest {
  if (!options || typeof options !== 'object') {
    throw new TypeError('shotium: screenshotTiles(options) needs an object');
  }
  if (typeof options.file !== 'string' || options.file.length === 0) {
    throw new TypeError('shotium: options.file is required');
  }
  const tile = options.tile;
  if (!tile || typeof tile !== 'object' || typeof tile.height !== 'number') {
    throw new TypeError(
        'shotium: screenshotTiles() needs tile.height, the most CSS pixels ' +
        'one tile covers');
  }
  if (!Number.isInteger(tile.height) || tile.height < 1 ||
      tile.height > MAX_TILE_HEIGHT) {
    throw new TypeError(
        `shotium: tile.height must be an integer from 1 to ${MAX_TILE_HEIGHT}`);
  }
  return toWire(options);
}

function toWire(options: ScreenshotOptions): WireRequest {
  const request: Record<string, unknown> = {};
  for (const [key, value] of Object.entries(options)) {
    if (value === undefined) {
      continue;
    }
    if (!WIRE_FIELDS.has(key)) {
      throw new TypeError(`shotium: unknown option "${key}"`);
    }
    request[key] = value;
  }

  // The viewport is flattened because the worker takes width and height at the
  // top level: it is one screenshot's frame, not a nested object on the wire.
  if (request.viewport) {
    const {width, height} = request.viewport as {
      width?: number,
      height?: number,
    };
    delete request.viewport;
    if (width !== undefined) {
      request.width = width;
    }
    if (height !== undefined) {
      request.height = height;
    }
  }
  return request as unknown as WireRequest;
}

function timeoutFor(options: ScreenshotOptions): number {
  const timeout = options.pageGotoParams && options.pageGotoParams.timeout;
  return typeof timeout === 'number' ? timeout : DEFAULT_TIMEOUT_MS;
}

export {
  DEFAULT_TIMEOUT_MS,
  MAX_TILE_HEIGHT,
  WIRE_FIELDS,
  timeoutFor,
  toRequest,
  toTilesRequest,
};
