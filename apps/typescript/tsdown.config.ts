import {defineConfig} from 'tsdown';

// Two entry points, one bundle each, plus whatever chunks they share.
//
// index is what a caller imports; daemon_main is spawned by path, which is the
// reason it is an entry rather than something the bundler was free to inline
// or rename. Everything lands directly in dist/, one directory below the
// package root -- lib/binding.ts and lib/daemon.ts compute paths relative to
// their own module URL and expect exactly that depth.
//
// ESM and nothing else. The package is a set of process-wide singletons -- one
// engine, which blink will not start twice, and one daemon per configuration
// -- so a dual build would hand a caller who reached it both ways two of each.
export default defineConfig({
  entry: ['src/index.ts', 'src/daemon_main.ts'],
  outDir: 'dist',
  format: 'esm',
  platform: 'node',
  target: 'node18',
  // .js, not the .mjs an esm build defaults to. The package is `"type":
  // "module"`, so .js already means an ES module here, and the extension is
  // load-bearing in two places that would otherwise have to spell it twice:
  // the `exports` map, and the path lib/client.ts spawns daemon_main by.
  outExtensions: () => ({js: '.js', dts: '.d.ts'}),
  // The published types, generated from the source rather than kept beside it
  // -- a hand-written .d.ts is a second description of the same thing, and the
  // two only agree until someone edits one of them. Declaration maps are off:
  // they are a jump-to-definition nobody has asked for.
  dts: {sourcemap: false},
  // Sourcemaps are on, and src/ is in the published `files`, so a stack trace
  // out of a failed screenshot names a line of TypeScript rather than a column
  // of bundle.
  sourcemap: true,
  clean: true,
  // Not minified on purpose: a stack trace out of a screenshot that failed is
  // worth more than the twenty kilobytes.
  minify: false,
  treeshake: true,
});
