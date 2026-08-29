import fs from 'node:fs';

const ARCH = new Map([
  [0x3e, 'x64'],
  [0xb7, 'arm64'],
  [0x8664, 'x64'],
  [0xaa64, 'arm64'],
  [0x01000007, 'x64'],
  [0x0100000c, 'arm64'],
]);

function parsePe(buffer) {
  if (buffer.length < 0x40 || buffer.toString('ascii', 0, 2) !== 'MZ') return null;
  const offset = buffer.readUInt32LE(0x3c);
  if (offset + 6 > buffer.length || buffer.toString('ascii', offset, offset + 4) !== 'PE\0\0') {
    return [];
  }
  return [ARCH.get(buffer.readUInt16LE(offset + 4)) || 'unknown'];
}

function parseElf(buffer) {
  if (buffer.length < 20 || !buffer.subarray(0, 4).equals(Buffer.from([0x7f, 0x45, 0x4c, 0x46]))) {
    return null;
  }
  const littleEndian = buffer[5] === 1;
  const machine = littleEndian ? buffer.readUInt16LE(18) : buffer.readUInt16BE(18);
  return [ARCH.get(machine) || 'unknown'];
}

function parseMachO(buffer) {
  if (buffer.length < 8) return null;
  const magicBe = buffer.readUInt32BE(0);
  const magicLe = buffer.readUInt32LE(0);
  if ([0xfeedface, 0xfeedfacf].includes(magicBe)) {
    return [ARCH.get(buffer.readUInt32BE(4)) || 'unknown'];
  }
  if ([0xfeedface, 0xfeedfacf].includes(magicLe)) {
    return [ARCH.get(buffer.readUInt32LE(4)) || 'unknown'];
  }
  const isFatBe = [0xcafebabe, 0xcafebabf].includes(magicBe);
  const isFatLe = [0xcafebabe, 0xcafebabf].includes(magicLe);
  if (!isFatBe && !isFatLe) return null;
  const read32 = isFatBe ? Buffer.prototype.readUInt32BE : Buffer.prototype.readUInt32LE;
  const count = read32.call(buffer, 4);
  const stride = (isFatBe ? magicBe : magicLe) === 0xcafebabf ? 32 : 20;
  const architectures = [];
  for (let index = 0; index < count; index += 1) {
    const offset = 8 + (index * stride);
    if (offset + 4 > buffer.length) break;
    architectures.push(ARCH.get(read32.call(buffer, offset)) || 'unknown');
  }
  return [...new Set(architectures)];
}

export function binaryArchitectures(file) {
  try {
    const descriptor = fs.openSync(file, 'r');
    try {
      const size = Math.min(fs.fstatSync(descriptor).size, 64 * 1024);
      const buffer = Buffer.alloc(size);
      fs.readSync(descriptor, buffer, 0, size, 0);
      return parsePe(buffer) || parseElf(buffer) || parseMachO(buffer) || [];
    } finally {
      fs.closeSync(descriptor);
    }
  } catch {
    return [];
  }
}

export function isNativeBinary(file, architecture = process.arch) {
  const architectures = binaryArchitectures(file);
  return {architectures, native: architectures.includes(architecture)};
}
