import type {
  CacheMode,
  Clip,
  PageGotoParams,
  ScreenshotOptions,
} from '../types.js';

const DEFAULT_TIMEOUT_MS = 30000;

// What actually goes down the pipe: ScreenshotOptions with the viewport
// flattened -- see toRequest below for why.
export interface WireRequest {
  file: string;
  type?: 'png'|'jpeg'|'webp';
  pngCompression?: 'balanced'|'fast';
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
  'pngCompression',
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
  WIRE_FIELDS,
  timeoutFor,
  toRequest,
};
