import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import {binaryArchitectures} from '../src/binary-architecture.ts';

test('detects PE, ELF and Mach-O architectures without executing them', () => {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'shotium-arch-test-'));
  try {
    const pe = Buffer.alloc(128);
    pe.write('MZ');
    pe.writeUInt32LE(64, 0x3c);
    pe.write('PE\0\0', 64, 'binary');
    pe.writeUInt16LE(0xaa64, 68);
    const elf = Buffer.alloc(64);
    Buffer.from([0x7f, 0x45, 0x4c, 0x46]).copy(elf);
    elf[5] = 1;
    elf.writeUInt16LE(0x3e, 18);
    const macho = Buffer.alloc(32);
    macho.writeUInt32BE(0xfeedfacf, 0);
    macho.writeUInt32BE(0x0100000c, 4);
    for (const [name, bytes] of [['pe', pe], ['elf', elf], ['macho', macho]] as const) {
      fs.writeFileSync(path.join(root, name), bytes);
    }
    assert.deepEqual(binaryArchitectures(path.join(root, 'pe')), ['arm64']);
    assert.deepEqual(binaryArchitectures(path.join(root, 'elf')), ['x64']);
    assert.deepEqual(binaryArchitectures(path.join(root, 'macho')), ['arm64']);
  } finally {
    fs.rmSync(root, {recursive: true, force: true});
  }
});
