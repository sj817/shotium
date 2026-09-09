// Pin the Node-API headers and Windows import libraries used by GN. No V8
// headers are included by the addon; the SDK is a build input, never shipped.
import {createHash} from 'node:crypto';
import {mkdir, readFile, writeFile} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

import {execa} from 'execa';

const version = '22.20.0';
const files = {
  'node-v22.20.0-headers.tar.gz': '22ba4692576442821f70156219752835a920e9ea2c0113a50fd9ebecb0ea6bbd',
  'win-x64/node.lib': 'f9d314af772d5d4d9c0f6bd483fa204240fb9c962f874d46334b95e59900575e',
  'win-arm64/node.lib': 'e5a9e8ac859070e477a2a0613d18cfe7061d4f9ec883c49f3b7ee0b06af30f69',
};
const root = path.resolve(import.meta.dirname, '..');
export async function prepareNodeSdk(): Promise<void> {
  const dest = path.join(root, 'out', 'node-sdk');
  await mkdir(dest, {recursive: true});
  for (const [name, sha] of Object.entries(files)) {
    if (name.startsWith('win-') && process.platform !== 'win32') continue;
    const target = path.join(dest, name);
    let bytes = await readFile(target).catch(() => Buffer.alloc(0));
    const digest = (data: Buffer) => createHash('sha256').update(data).digest('hex');
    if (digest(bytes) !== sha) {
      const response = await fetch(`https://nodejs.org/dist/v${version}/${name}`);
      if (!response.ok) throw new Error(`Node SDK ${name}: HTTP ${response.status}`);
      bytes = Buffer.from(await response.arrayBuffer());
      if (digest(bytes) !== sha) throw new Error(`Node SDK checksum mismatch: ${name}`);
      await mkdir(path.dirname(target), {recursive: true});
      await writeFile(target, bytes);
    }
    if (name.endsWith('.tar.gz')) {
      await execa('tar', ['-xzf', target, '-C', dest]);
    } else {
      // node.lib also exports V8/cppgc and OpenSSL. Importing those symbols
      // into an embedded Blink/BoringSSL build conflicts with our own core.
      // Preserve only the actual Node-API exports from the verified archive.
      const symbols = [...new Set(Array.from(bytes.toString('latin1')
        .matchAll(/\b((?:napi|node_api)_[a-zA-Z0-9_]+)(?=\0)/g), match => match[1]))].sort();
      if (!symbols.includes('napi_create_threadsafe_function') || symbols.length < 100) {
        throw new Error(`Could not read Node-API exports from ${name}`);
      }
      const definition = path.join(path.dirname(target), 'node-api.def');
      await writeFile(definition, `LIBRARY node.exe\nEXPORTS\n${symbols.join('\n')}\n`);
      await execa(path.join(root, 'third_party/llvm-build/Release+Asserts/bin/lld-link.exe'), [
        '/lib', `/def:${definition}`, `/out:${path.join(path.dirname(target), 'node-api.lib')}`,
        `/machine:${name.startsWith('win-arm64') ? 'arm64' : 'x64'}`,
      ]);
    }
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  await prepareNodeSdk();
}
