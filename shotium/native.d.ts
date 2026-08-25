// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {ScreenshotOptions} from './index';

export interface NativeStartOptions {
  /**
   * Root of the HTTP disk cache. `null` disables caching entirely, which is
   * the default here: an in-process engine is often a short-lived program, and
   * a cache it never reads twice is a directory it leaves behind.
   */
  cacheDir?: string|null;

  /** Overrides the built-in user agent string. */
  userAgent?: string;

  /**
   * Where `shot_data.pak` and `shot_strings.pak` are. Defaults to the
   * directory the native engine was loaded from, which is where they ship.
   */
  resourceDir?: string;
}

export interface PurgeOptions {
  /**
   * Also ask the OS to take the engine's pages back. The next screenshot pays
   * them back in soft page faults -- a few milliseconds -- so this is for when
   * there may not be a next one soon.
   */
  releaseWorkingSet?: boolean;
}

/**
 * The engine, in this process.
 *
 * Same options and same output as `runtime`, and a different set of trades.
 * There is no worker process, so there is nothing to start, nothing to find on
 * disk and no pipe: a screenshot costs about a third less than through the
 * pool, and the whole thing is one process instead of five.
 *
 * What it gives up is what a separate process was providing for free. One
 * renderer, because blink is a process-wide singleton and `worker_threads`
 * share the process, so requests are serialised however many callers there
 * are. And no crash isolation: a renderer that dies takes the host program
 * with it, where the pool would have retried.
 *
 * Use this for a program that takes screenshots one at a time and would rather
 * not run a pool. Use `runtime` or `daemon` for a service.
 */
export interface NativeRuntime {
  readonly running: boolean;

  /** Starts the engine. Calling it twice is a no-op. */
  start(options?: NativeStartOptions): NativeRuntime;

  /** Stops the engine, after whatever is queued. */
  stop(): Promise<void>;

  /**
   * Hands back what the engine is holding but can rebuild.
   *
   * The resident worker does this for itself on a timer, because it can watch
   * its own request stream go quiet. Here the queue belongs to the caller, so
   * the caller is the one who knows a batch has ended.
   */
  purge(options?: PurgeOptions): void;

  /**
   * Renders one screenshot. Resolves to the encoded image, or to `null` when
   * `path` was given and the engine wrote the file itself.
   */
  screenshot(options: ScreenshotOptions): Promise<Buffer|null>;
}

export declare const NativeRuntime: {
  new(): NativeRuntime;
};

export declare const native: NativeRuntime;
export declare function screenshot(options: ScreenshotOptions):
    Promise<Buffer|null>;
