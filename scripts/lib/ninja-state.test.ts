// The build-statement reader: which paths of a GN-written graph can make an
// output stale, and which .ninja files belong to the graph at all.
import assert from 'node:assert/strict';
import {mkdirSync, mkdtempSync, rmSync, writeFileSync} from 'node:fs';
import {tmpdir} from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {buildInputs, graphInputs} from './ninja-state.ts';

test('build statements: explicit and implicit inputs, escapes undone, order-only and validations dropped', () => {
  const text = [
    'rule cxx',
    '  command = clang ${in}',
    'build obj/a.o: cxx ../../shot/a.cc | ../../build/toolchain.py || obj/gen.stamp',
    'build gen/build/modules/module.modulemap: gen_rule | ../../x.py C$:/Program$ Files/MSVC/include/a.h $',
    '    C$:/Program$ Files/MSVC/include/b.h',
    'build obj/b.o | obj/b.d: cxx sdk/xcode_links/MacOSX.sdk/SDKSettings.json |@ obj/check.stamp',
    'build obj/c.o: cxx ${root}/c.cc ../../shot/c.cc',
    'build phony_target: phony',
    '  description = not a build statement: build x: y z',
  ].join('\n');
  assert.deepEqual(buildInputs(text), [
    '../../shot/a.cc', '../../build/toolchain.py',
    '../../x.py', 'C:/Program Files/MSVC/include/a.h', 'C:/Program Files/MSVC/include/b.h',
    'sdk/xcode_links/MacOSX.sdk/SDKSettings.json',
    '../../shot/c.cc',
  ]);
  assert.deepEqual(buildInputs('build out$:put: r in$$put\r\nbuild o: r a\r\n'), ['in$put', 'a']);
});

test('the graph is what build.ninja reaches; a stale .ninja file gn left behind is not read', () => {
  const dir = mkdtempSync(path.join(tmpdir(), 'ninja-graph-'));
  try {
    mkdirSync(path.join(dir, 'obj', 'shot'), {recursive: true});
    mkdirSync(path.join(dir, 'obj', 'gone'), {recursive: true});
    writeFileSync(path.join(dir, 'build.ninja'), 'build build.ninja: gn | ../../BUILD.gn\nsubninja toolchain.ninja\n');
    writeFileSync(path.join(dir, 'toolchain.ninja'), 'include rules.ninja\nsubninja obj/shot/shot.ninja\n');
    writeFileSync(path.join(dir, 'rules.ninja'), 'rule cxx\n  command = clang\n');
    writeFileSync(path.join(dir, 'obj', 'shot', 'shot.ninja'), 'build obj/shot/a.o: cxx ../../shot/a.cc\n');
    writeFileSync(path.join(dir, 'obj', 'gone', 'gone.ninja'), 'build obj/gone/a.o: cxx /usr/include/stale.h\n');
    assert.deepEqual([...graphInputs(dir)].sort(), ['../../BUILD.gn', '../../shot/a.cc']);
  } finally {
    rmSync(dir, {recursive: true, force: true});
  }
});
