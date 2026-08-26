const DEFAULT_TIMEOUT_MS = 30000;
// How much longer than the page's own deadline a supervisor waits before
// deciding the worker is not going to answer at all. The worker fails a slow
// page by itself and replies; this margin covers process startup and the
// encode, and firing it means something worse than a slow page.
const SUPERVISOR_MARGIN_MS = 10000;

// Everything the worker understands, and nothing else. An unknown field is a
// typo, and a typo that is silently dropped is a screenshot that quietly
// ignored what was asked for -- so this rejects rather than filters.
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
]);

// One ScreenshotOptions, checked and flattened into what goes on the wire.
//
// It lives here rather than in index.js because the in-process pool and the
// daemon both send it: a request that is valid through one entry point and
// rejected through the other would be a difference nobody asked for.
function toRequest(options) {
  if (!options || typeof options !== 'object') {
    throw new TypeError('shotium: screenshot(options) needs an object');
  }
  if (typeof options.file !== 'string' || options.file.length === 0) {
    throw new TypeError('shotium: options.file is required');
  }

  const request = {};
  for (const [key, value] of Object.entries(options)) {
    if (value === undefined) {
      continue;
    }
    // retry is the supervisor's, not the worker's: it decides how many times a
    // request is re-sent, which is not something the worker could act on.
    if (key === 'retry') {
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
    const {width, height} = request.viewport;
    delete request.viewport;
    if (width !== undefined) {
      request.width = width;
    }
    if (height !== undefined) {
      request.height = height;
    }
  }
  return request;
}

function timeoutFor(options) {
  const timeout = options.pageGotoParams && options.pageGotoParams.timeout;
  return typeof timeout === 'number' ? timeout : DEFAULT_TIMEOUT_MS;
}

export {
  DEFAULT_TIMEOUT_MS,
  SUPERVISOR_MARGIN_MS,
  WIRE_FIELDS,
  timeoutFor,
  toRequest,
};
