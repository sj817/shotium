// The deps-log format is binary and hand-written here; a mistake in it does
// not fail loudly, it makes ninja quietly rebuild everything a shard sent.
// So: round trips, the exact byte layout of one small log, and the two
// TimeStamp conversions against values read from a real build directory.
import assert from 'node:assert/strict';
import {mkdirSync, mkdtempSync, rmSync, statSync, utimesSync, writeFileSync} from 'node:fs';
import {tmpdir} from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {execaSync} from 'execa';

import {
  DEPS_HEADER, LOG_HEADER, assignShards, formatDeps, formatLog, fromNinjaTime, parseDeps, parseLog, toNinjaTime,
} from './lib/ninja-state.ts';

test('shards: longest first onto the least-loaded shard, deterministic, every object once', () => {
  const objects = ['a.o', 'b.o', 'c.o', 'd.o', 'e.o', 'f.o'];
  const cost: Record<string, number> = {'a.o': 10, 'b.o': 9, 'c.o': 8, 'd.o': 2, 'e.o': 2, 'f.o': 1};
  const shards = assignShards(objects, 3, (o) => cost[o]);
  assert.deepEqual(shards, [['a.o', 'f.o'], ['b.o', 'e.o'], ['c.o', 'd.o']]);
  const again = assignShards([...objects].reverse(), 3, (o) => cost[o]);
  assert.deepEqual(again, shards);
  assert.deepEqual(shards.flat().sort(), objects);
});

test('shards: unknown costs come from the path, Blink jumbo units heaviest', () => {
  const objects = ['obj/third_party/blink/renderer/core/core/core_shot_jumbo_css_0.o', 'obj/base/base/base_shot_jumbo_root_0.o',
    'obj/net/net/x.o', 'obj/net/net/y.o', 'obj/net/net/z.o', 'obj/net/net/w.o'];
  const shards = assignShards(objects, 2, () => undefined);
  assert.equal(shards[0][0], objects[0]);
  assert.equal(shards[1][0], objects[1]);
  assert.equal(shards[0].length + shards[1].length, 6);
});

test('deps log: format then parse gives the same records, ids assigned in order', () => {
  const records = new Map([
    ['obj/a.o', {mtime: 1788674105279902600n, deps: ['../../a.cc', '../../a.h', '../../b.h']}],
    ['obj/b.o', {mtime: 1788674106000000000n, deps: ['../../b.cc', '../../b.h']}],
  ]);
  const buf = formatDeps({paths: [], records});
  assert.equal(buf.subarray(0, DEPS_HEADER.length).toString('latin1'), DEPS_HEADER);
  assert.equal(buf.readInt32LE(DEPS_HEADER.length), 4);
  const back = parseDeps(buf);
  assert.deepEqual(back.paths, ['obj/a.o', '../../a.cc', '../../a.h', '../../b.h', 'obj/b.o', '../../b.cc']);
  assert.deepEqual([...back.records.entries()], [...records.entries()]);
});

test('deps log: one path record is NUL-padded to 4 bytes and closed by ~id', () => {
  const buf = formatDeps({paths: [], records: new Map([['x.o', {mtime: 5n, deps: []}]])});
  let off = DEPS_HEADER.length + 4;
  const size = buf.readUInt32LE(off);
  assert.equal(size, 8); // "x.o" -> 4 bytes padded + 4 bytes checksum
  assert.equal(buf.subarray(off + 4, off + 8).toString('latin1'), 'x.o\0');
  assert.equal(buf.readUInt32LE(off + 8), (~0) >>> 0);
  off += 4 + size;
  const raw = buf.readUInt32LE(off);
  assert.equal((raw & 0x80000000) >>> 0, 0x80000000);
  assert.equal(raw & 0x7fffffff, 12);
  assert.equal(buf.readInt32LE(off + 4), 0);
  assert.equal(buf.readBigInt64LE(off + 8), 5n);
});

test('deps log: a later record for the same output wins, a truncated tail is tolerated', () => {
  const first = formatDeps({paths: [], records: new Map([['x.o', {mtime: 1n, deps: ['h1']}]])});
  const second = formatDeps({paths: [], records: new Map([['x.o', {mtime: 2n, deps: ['h2']}]])});
  // Append only the deps record of the second log, re-pointing its ids at the
  // first log's table (x.o = 0, h1 = 1) plus a new path h2 = 2.
  const h2 = Buffer.alloc(4 + 4 + 4);
  h2.writeUInt32LE(8, 0);
  h2.write('h2\0\0', 4, 'latin1');
  h2.writeUInt32LE((~2) >>> 0, 8);
  const rec = Buffer.alloc(4 + 16);
  rec.writeUInt32LE((16 | 0x80000000) >>> 0, 0);
  rec.writeInt32LE(0, 4);
  rec.writeBigInt64LE(2n, 8);
  rec.writeInt32LE(2, 16);
  const merged = Buffer.concat([first, h2, rec, Buffer.from([1, 2, 3])]);
  const back = parseDeps(merged);
  assert.deepEqual(back.records.get('x.o'), {mtime: 2n, deps: ['h2']});
  assert.equal(second.length > 0, true);
});

test('build log: parse then format keeps the header and the last entry per output', () => {
  const text = [LOG_HEADER, '1\t2\t100\tobj/a.o\tdeadbeef', '3\t4\t200\tobj/a.o\tdeadbeef', '5\t6\t300\tobj/b.o\tcafe', ''].join('\n');
  const {header, entries} = parseLog(text);
  assert.equal(header, LOG_HEADER);
  assert.equal(entries.get('obj/a.o')?.mtime, 200n);
  assert.equal(formatLog(header, entries.values()), [LOG_HEADER, '3\t4\t200\tobj/a.o\tdeadbeef', '5\t6\t300\tobj/b.o\tcafe', ''].join('\n'));
});

test('TimeStamp: POSIX is nanoseconds, Windows is FILETIME shifted by 400 years', () => {
  assert.equal(toNinjaTime(1788674105279902600n, 'linux'), 1788674105279902600n);
  assert.equal(fromNinjaTime(7n, 'darwin'), 7n);
  // Read from a real .ninja_deps on the development host: this file mtime
  // (statSync bigint) produced this record.
  const fileNs = 1788673842963356300n;
  const record = 8103770429633563n;
  assert.equal(toNinjaTime(fileNs, 'win32'), record);
  assert.equal(fromNinjaTime(record, 'win32'), fileNs);
});

// A cold run of the sharded build left 438 edges for the final job, and
// ninja named the reason: liballoc_error_handler_impl.a was 191 ms older
// than the .o it archives, which no single machine can produce. Every job
// builds that pair -- it is in the generator closure -- so the merge chose
// the archive from one job and the object from another, and inverted an
// order that must hold. This builds the same situation in a temporary
// directory: a shard whose pair is older than the target's own, which is
// what the final job looks like once it compiles a slice of its own.
test('merge: an output copied from a shard cannot end up older than an input kept from another', () => {
  const root = mkdtempSync(path.join(tmpdir(), 'shot-merge-'));
  const OBJ = 'obj/rust/allocator/impl.o';
  const LIB = 'obj/rust/allocator/libimpl.a';
  // Seconds, not nanoseconds: utimes takes a double, and the point of the
  // test is the ordering, not the resolution.
  const write = (dir: string, objAt: number, libAt: number): void => {
    for (const [rel, when] of [[OBJ, objAt], [LIB, libAt]] as [string, number][]) {
      const file = path.join(dir, rel);
      mkdirSync(path.dirname(file), {recursive: true});
      writeFileSync(file, rel);
      utimesSync(file, when, when);
    }
    const stamp = (rel: string): bigint => toNinjaTime(statSync(path.join(dir, rel), {bigint: true}).mtimeNs);
    writeFileSync(path.join(dir, '.ninja_log'), formatLog(LOG_HEADER, [
      {start: 0, end: 1, mtime: stamp(OBJ), output: OBJ, hash: 'aa'},
      {start: 1, end: 2, mtime: stamp(LIB), output: LIB, hash: 'bb'},
    ]));
  };

  const target = path.join(root, 'out');
  const shard = path.join(root, 'shard');
  // The shard built the pair a minute before the final job built its own.
  write(target, 1788718341, 1788718342);
  write(shard, 1788718280, 1788718281);

  const result = execaSync('node', ['--experimental-strip-types', path.join(import.meta.dirname, 'build-shards.ts'),
    'merge', '--build-dir', target, '--shard', shard], {reject: false});
  assert.equal(result.exitCode, 0, result.stderr);

  const objAt = statSync(path.join(target, OBJ), {bigint: true}).mtimeNs;
  const libAt = statSync(path.join(target, LIB), {bigint: true}).mtimeNs;
  rmSync(root, {recursive: true, force: true});
  assert.ok(objAt < libAt,
    `the archive must stay newer than its object: .o ${objAt} vs .a ${libAt}`);
});
