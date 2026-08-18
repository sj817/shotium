/// <reference types="node" />

import {EventEmitter} from 'events';

export interface Clip {
  x: number;
  y: number;
  width: number;
  height: number;
}

export interface PageGotoParams {
  /** Milliseconds before the load is abandoned. Default 30000. */
  timeout?: number;
  /**
   * `load` waits for parsing to finish, the load event to fire and every
   * request to complete. `networkidle` additionally waits for a 500ms window
   * with nothing in flight, which matters for documents that keep fetching
   * after the load event -- CSS that pulls in more CSS, or a font a late style
   * change brought in.
   */
  waitUntil?: 'load'|'networkidle';
}

export interface Viewport {
  /** CSS pixels. Default 1280. */
  width?: number;
  /** CSS pixels. Default 720. */
  height?: number;
}

export interface ScreenshotOptions {
  /** An http/https/file URL, or a local path. */
  file: string;
  /** Default `png`. */
  type?: 'png'|'jpeg'|'webp';
  /** Capture the whole document rather than the viewport. */
  fullPage?: boolean;
  /**
   * Capture the box of the first element matching this CSS selector. Resolved
   * inside the renderer with Document::querySelector -- there is no JavaScript
   * engine, so nothing is injected into the page.
   */
  selector?: string;
  /** 1-100, `jpeg` and `webp` only. Default 90. */
  quality?: number;
  /** Device scale factor, 0.01-8. Default 1. */
  scale?: number;
  /**
   * Keep the alpha channel instead of painting the page's white backdrop.
   * Rejected for `jpeg`, which has no alpha channel.
   */
  omitBackground?: boolean;
  /** Write the image here instead of returning it, saving a round trip. */
  path?: string;
  pageGotoParams?: PageGotoParams;
  /** A region of the document, in CSS pixels. */
  clip?: Clip;
  /** The viewport the document is laid out in. */
  viewport?: Viewport;
  /**
   * Let the document read `file:` subresources. Off by default: a library does
   * not get to decide for its caller that a document may read the filesystem it
   * is rendered on.
   */
  allowFileAccess?: boolean;
  /** How many times to re-send after a crash or a timeout. Default 0. */
  retry?: number;
}

export interface StartOptions {
  /** Path to `shot.exe`. Default `$SHOTIUM_BINARY`, then `./bin/shot.exe`. */
  binary?: string;
  /** Worker processes. Default half the cores, at least one. */
  workers?: number;
  /** Root of the per-worker HTTP disk caches. `null` disables caching. */
  cacheDir?: string|null;
  /** Extra flags passed to every worker. */
  args?: string[];
}

export interface WorkerEvent {
  worker: number;
  code?: number|null;
  signal?: string|null;
}

export interface Runtime extends EventEmitter {
  readonly running: boolean;
  start(options?: StartOptions): Runtime;
  stop(): Promise<void>;
  /** Resolves to the image, or to null when `path` was given. */
  screenshot(options: ScreenshotOptions): Promise<Buffer|null>;

  on(event: 'ready', listener: (info: {workers: number}) => void): this;
  on(event: 'exit', listener: (event: WorkerEvent) => void): this;
  on(event: 'crash', listener: (event: WorkerEvent) => void): this;
  on(event: 'timeout',
     listener: (event: {worker: number, timeout: number}) => void): this;
  on(event: 'worker-restart',
     listener: (event: {worker: number, reason: string}) => void): this;
  on(event: 'stderr',
     listener: (event: {worker: number, line: string}) => void): this;
}

export declare const runtime: Runtime;
export declare function screenshot(options: ScreenshotOptions):
    Promise<Buffer|null>;
