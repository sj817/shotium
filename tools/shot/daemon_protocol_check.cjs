'use strict';

// Checks the resident daemon's version boundary without loading an engine.
// Derived endpoints keep incompatible generations apart; an exact endpoint
// cannot, so connect() asks the peer's status and refuses it without sending a
// shutdown. This runs in the fast checks job after shotium has been bundled.

const assert = require('assert');
const crypto = require('crypto');
const fs = require('fs');
const net = require('net');
const os = require('os');
const path = require('path');

const shotium = require('../../shotium');

const PROTOCOL_VERSION = 2;

function frame(value) {
  const payload = Buffer.isBuffer(value) ?
      value :
      Buffer.from(JSON.stringify(value), 'utf8');
  const header = Buffer.alloc(4);
  header.writeUInt32LE(payload.length, 0);
  return Buffer.concat([header, payload]);
}

function fakeEndpoint(label) {
  const unique = `${label}-${process.pid}-${Date.now()}`;
  return process.platform === 'win32' ?
      `\\\\.\\pipe\\shotium-${unique}` :
      path.join(os.tmpdir(), `shotium-${unique}.sock`);
}

async function fakeDaemon(endpoint, readStatus) {
  const operations = [];
  const sockets = new Set();
  if (process.platform !== 'win32') {
    fs.rmSync(endpoint, {force: true});
  }

  const server = net.createServer((socket) => {
    sockets.add(socket);
    socket.on('close', () => sockets.delete(socket));
    let buffered = Buffer.alloc(0);
    socket.on('data', (chunk) => {
      buffered = buffered.length === 0 ?
          chunk :
          Buffer.concat([buffered, chunk]);
      while (buffered.length >= 4) {
        const length = buffered.readUInt32LE(0);
        if (buffered.length < 4 + length) {
          return;
        }
        const request = JSON.parse(
            buffered.subarray(4, 4 + length).toString('utf8'));
        buffered = buffered.subarray(4 + length);
        operations.push(request.op);
        if (request.op === 'status') {
          const status = readStatus();
          if (status === null) {
            continue;
          }
          socket.write(frame({id: request.id, ok: true, ...status}));
          socket.write(frame(Buffer.alloc(0)));
        } else {
          socket.write(frame({
            id: request.id,
            ok: false,
            error: `unexpected operation ${request.op}`,
          }));
          socket.write(frame(Buffer.alloc(0)));
        }
      }
    });
  });
  await new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(endpoint, resolve);
  });

  return {
    operations,
    server,
    async close() {
      for (const socket of sockets) {
        socket.destroy();
      }
      await new Promise((resolve) => server.close(resolve));
      if (process.platform !== 'win32') {
        fs.rmSync(endpoint, {force: true});
      }
    },
  };
}

async function main() {
  const oldEnvironmentEndpoint = process.env.SHOTIUM_ENDPOINT;
  delete process.env.SHOTIUM_ENDPOINT;

  const name = `protocol-check-${process.pid}`;
  const named = await shotium.daemon.status({name});
  assert.equal(named.running, false);
  assert(
      named.endpoint.includes(`${name}-v${PROTOCOL_VERSION}`),
      `named endpoint did not include protocol generation: ${named.endpoint}`);

  const resourceDir = path.resolve(os.tmpdir(), 'shotium-protocol-resources');
  const userAgent = `protocol-check/${process.pid}`;
  const identity = [
    PROTOCOL_VERSION,
    null,
    userAgent,
    resourceDir,
  ];
  const expectedKey = crypto.createHash('sha256')
                          .update(JSON.stringify(identity))
                          .digest('hex')
                          .slice(0, 16);
  const derived = await shotium.daemon.status({
    cacheDir: null,
    userAgent,
    resourceDir,
  });
  assert.equal(derived.running, false);
  assert(
      derived.endpoint.includes(expectedKey),
      `configuration endpoint omitted protocol generation: ${derived.endpoint}`);

  const endpoint = fakeEndpoint('legacy');
  let advertised = {version: '0.4.0'};
  const fake = await fakeDaemon(endpoint, () => advertised);
  try {
    await assert.rejects(
        shotium.daemon.connect({endpoint}),
        /wire protocol legacy\/unversioned .* requires 2/);

    advertised = {
      version: 'future',
      protocolVersion: PROTOCOL_VERSION,
      capabilities: ['screenshot'],
    };
    process.env.SHOTIUM_ENDPOINT = endpoint;
    await assert.rejects(
        shotium.daemon.connect(),
        /missing required capabilities: tiles/);

    advertised = {
      version: 'future',
      protocolVersion: PROTOCOL_VERSION,
      capabilities: ['screenshot', 'tiles'],
    };
    const client = await shotium.daemon.connect({endpoint, spawn: false});
    client.close();

    advertised = null;
    await assert.rejects(
        shotium.daemon.connect({
          endpoint,
          startTimeoutMs: 250,
        }),
        /did not complete the wire protocol handshake: timed out after 250ms/);

    assert(fake.server.listening, 'the compatibility check stopped the peer');
    assert.deepEqual(
        fake.operations, ['status', 'status', 'status', 'status']);
  } finally {
    if (oldEnvironmentEndpoint === undefined) {
      delete process.env.SHOTIUM_ENDPOINT;
    } else {
      process.env.SHOTIUM_ENDPOINT = oldEnvironmentEndpoint;
    }
    await fake.close();
  }

  console.log('ok: daemon protocol endpoints and handshake');
}

main().catch((error) => {
  console.error(error);
  process.exit(1);
});
