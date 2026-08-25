'use strict';

// Puts shot_api.h somewhere it is the only thing there, and prints where.
//
// binding.gyp calls this at generate time and uses the result as the addon's
// one include directory. The obvious thing -- pointing the include directory
// straight at shot/ -- does not survive a case-insensitive filesystem:
//
//   src/shot/version:1:10: error: expected ';' after top level declarator
//       1 | MAJOR=153
//
// That is libc++'s <string> including <version>, the compiler searching the
// include directories before the system ones, and macOS answering `version`
// with `shot/VERSION`, which is this tree's build number file. The same trap is
// set on Windows -- NTFS is case-insensitive too -- and springs only because
// MSVC's <string> happens not to reach for <version>.
//
// Renaming the file is not available: shot/VERSION is what this fork calls
// chrome/VERSION, and //base, //build and a dozen .gni files name it. Nor is
// -iquote, which would say exactly the right thing and has a different
// spelling in each of the three generators node-gyp drives. Copying the one
// header the addon needs into a directory of its own is what is left, and it
// has the advantage that nothing about it can be undone by an STL that starts
// including one more thing.
//
// SHOT_INCLUDE_DIR still names where shot_api.h is *found*; it just is not
// handed to the compiler any more.

const fs = require('fs');
const path = require('path');

const HEADER = 'shot_api.h';

const source = process.env.SHOT_INCLUDE_DIR ||
    path.resolve(__dirname, '..', '..', 'shot');
const staged = path.resolve(__dirname, 'build', 'include');

const from = path.join(source, HEADER);
if (!fs.existsSync(from)) {
  process.stderr.write(
      `stage_header.js: no ${HEADER} in ${source}\n` +
      '  Set SHOT_INCLUDE_DIR to the directory holding it.\n');
  process.exit(1);
}

fs.mkdirSync(staged, {recursive: true});
fs.copyFileSync(from, path.join(staged, HEADER));

// gyp takes this whole line as the variable's value, so it is the only thing
// written to stdout.
process.stdout.write(staged);
