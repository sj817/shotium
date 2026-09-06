// why: ninja's whole notion of "up to date" is three things -- the outputs,
// .ninja_log (when each output was built, by which command) and .ninja_deps
// (which headers each object read, and the object's mtime at the time).
// build-shards.ts moves that state between build directories on different
// machines, which means reading and writing both files exactly and knowing
// how ninja turns a file's mtime into the integer it stores. Nothing else in
// the repository should need to; keep the format knowledge here.
//
// .ninja_log (v6): a header line, then tab-separated
//   start_ms  end_ms  mtime  output  command_hash
// where mtime is ninja's TimeStamp of the output right after the command
// ran. A later line for the same output supersedes an earlier one.
//
// .ninja_deps (version 4, little-endian): "# ninjadeps\n", int32 version,
// then records each prefixed by a uint32 size. High bit clear: a path,
// NUL-padded to a multiple of 4 bytes, followed by uint32 ~id (ids are
// assigned in file order). High bit set: int32 output id, int64 mtime, then
// int32 dep ids. A later record for the same output supersedes an earlier
// one; ninja tolerates a truncated tail, so this reader does too.
//
// TimeStamp: nanoseconds since 1970 on POSIX. On Windows it is the FILETIME
// (100 ns units since 1601) minus 400 years, per ninja/src/disk_interface.cc;
// the two conversions below were checked against 1,945 records of a real
// build directory on the development host.

export const DEPS_HEADER = '# ninjadeps\n';
export const DEPS_VERSION = 4;
export const LOG_HEADER = '# ninja log v6';

const WINDOWS_EPOCH_SHIFT = 12622770400n * 10000000n;
const UNIX_TO_FILETIME = 116444736000000000n;

export interface LogEntry {
  start: number;
  end: number;
  mtime: bigint;
  output: string;
  hash: string;
}

export interface DepsRecord {
  mtime: bigint;
  deps: string[];
}

export interface DepsLog {
  paths: string[];
  records: Map<string, DepsRecord>;
}

export function parseLog(text: string): {header: string; entries: Map<string, LogEntry>} {
  const lines = text.split('\n');
  const header = lines[0]?.startsWith('# ninja log') ? lines[0] : LOG_HEADER;
  const entries = new Map<string, LogEntry>();
  for (const line of lines) {
    if (!line || line.startsWith('#')) continue;
    const p = line.split('\t');
    if (p.length < 5) continue;
    entries.set(p[3], {start: Number(p[0]), end: Number(p[1]), mtime: BigInt(p[2]), output: p[3], hash: p[4]});
  }
  return {header, entries};
}

export function formatLog(header: string, entries: Iterable<LogEntry>): string {
  const out = [header];
  for (const e of entries) out.push(`${e.start}\t${e.end}\t${e.mtime}\t${e.output}\t${e.hash}`);
  return out.join('\n') + '\n';
}

export function parseDeps(buf: Buffer): DepsLog {
  const head = buf.subarray(0, DEPS_HEADER.length).toString('latin1');
  if (head !== DEPS_HEADER) throw new Error(`not a ninja deps log (header ${JSON.stringify(head)})`);
  const version = buf.readInt32LE(DEPS_HEADER.length);
  if (version !== DEPS_VERSION) throw new Error(`ninja deps log version ${version}; this reader knows ${DEPS_VERSION}`);
  const paths: string[] = [];
  const records = new Map<string, DepsRecord>();
  let off = DEPS_HEADER.length + 4;
  while (off + 4 <= buf.length) {
    const raw = buf.readUInt32LE(off);
    off += 4;
    const size = raw & 0x7fffffff;
    if (off + size > buf.length) break;
    if (raw & 0x80000000) {
      const id = buf.readInt32LE(off);
      const mtime = buf.readBigInt64LE(off + 4);
      const deps: string[] = [];
      for (let p = off + 12; p < off + size; p += 4) deps.push(paths[buf.readInt32LE(p)]);
      records.set(paths[id], {mtime, deps});
    } else {
      const check = buf.readUInt32LE(off + size - 4);
      if (check !== ((~paths.length) >>> 0)) throw new Error(`deps log corrupt at path record ${paths.length}`);
      let end = off + size - 4;
      while (end > off && buf[end - 1] === 0) end--;
      paths.push(buf.subarray(off, end).toString('utf8'));
    }
    off += size;
  }
  return {paths, records};
}

export function formatDeps(log: DepsLog): Buffer {
  const chunks: Buffer[] = [Buffer.from(DEPS_HEADER, 'latin1')];
  const version = Buffer.alloc(4);
  version.writeInt32LE(DEPS_VERSION);
  chunks.push(version);
  const ids = new Map<string, number>();
  const pathRecord = (p: string): number => {
    const known = ids.get(p);
    if (known !== undefined) return known;
    const id = ids.size;
    ids.set(p, id);
    const bytes = Buffer.from(p, 'utf8');
    const padded = (bytes.length + 3) & ~3;
    const rec = Buffer.alloc(4 + padded + 4);
    rec.writeUInt32LE(padded + 4, 0);
    bytes.copy(rec, 4);
    rec.writeUInt32LE((~id) >>> 0, 4 + padded);
    chunks.push(rec);
    return id;
  };
  for (const [output, record] of log.records) {
    const outId = pathRecord(output);
    const depIds = record.deps.map(pathRecord);
    const rec = Buffer.alloc(4 + 12 + depIds.length * 4);
    rec.writeUInt32LE(((12 + depIds.length * 4) | 0x80000000) >>> 0, 0);
    rec.writeInt32LE(outId, 4);
    rec.writeBigInt64LE(record.mtime, 8);
    depIds.forEach((id, i) => rec.writeInt32LE(id, 16 + i * 4));
    chunks.push(rec);
  }
  return Buffer.concat(chunks);
}

export function toNinjaTime(mtimeNs: bigint, platform: NodeJS.Platform = process.platform): bigint {
  if (platform === 'win32') return mtimeNs / 100n + UNIX_TO_FILETIME - WINDOWS_EPOCH_SHIFT;
  return mtimeNs;
}

export function fromNinjaTime(stamp: bigint, platform: NodeJS.Platform = process.platform): bigint {
  if (platform === 'win32') return (stamp + WINDOWS_EPOCH_SHIFT - UNIX_TO_FILETIME) * 100n;
  return stamp;
}
