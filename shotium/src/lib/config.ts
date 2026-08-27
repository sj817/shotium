import type {StartOptions} from '../types.js';

// StartOptions with every hole filled in. `cacheDir` is still nullable here
// because null is an answer -- "no disk cache" -- and not an absent one.
export interface ResolvedStartOptions {
  cacheDir: string|null;
  userAgent?: string;
  resourceDir?: string;
}

// The one place that decides what "no options" means.
//
// It is shared rather than duplicated because the daemon's address is a hash of
// its configuration: if two callers filled in defaults even slightly
// differently, one would compute an address no daemon is listening on and
// start a second engine next to the first one that was already warm. See
// endpoint.ts.
//
// The default for `cacheDir` is null -- no disk cache. A program holding the
// engine is often short-lived, and a cache it never reads twice is a directory
// it leaves behind. The daemon, which is the case where a cache does pay for
// itself, is also the case where the caller is already passing options.
function resolveStartOptions(options: StartOptions = {}): ResolvedStartOptions {
  return {
    cacheDir: options.cacheDir ?? null,
    userAgent: options.userAgent,
    resourceDir: options.resourceDir,
  };
}

export {resolveStartOptions};
