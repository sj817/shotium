// The deps-log format is binary and hand-written here; a mistake in it does
// not fail loudly, it makes ninja quietly rebuild everything a shard sent.
// So: round trips, the exact byte layout of one small log, and the two
// TimeStamp conversions against values read from a real build directory.
import assert from 'node:assert/strict';
import test from 'node:test';

import {
  DEPS_HEADER, LOG_HEADER, formatDeps, formatLog, fromNinjaTime, parseDeps, parseLog, toNinjaTime,
} from './lib/ninja-state.ts';

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
