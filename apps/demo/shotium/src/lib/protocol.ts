// The wire format shotium.exe --serve speaks, in both directions: a 4-byte
// little-endian length followed by that many bytes.
//
// Length-prefixed rather than line-delimited because the payload is binary and
// a newline inside a PNG is not a message boundary. See shot/shot_server.h for
// the same description from the other end.
//
//   ->  [len][{"file":"...","width":1248,...}]
//   <-  [len][{"ok":true,"bytes":97756}]  [len][<PNG bytes>]
//   <-  [len][{"ok":false,"error":"..."}] [0]

import type {DaemonCapability} from '../types.js';

const HEADER_BYTES = 4;

// The resident daemon's wire generation, independent of the npm package and
// the C ABI. Additive package releases may keep this number; a client that
// cannot safely talk to the previous daemon increments it. Derived endpoints
// include the generation, while callers that choose an exact endpoint get a
// status handshake instead.
const DAEMON_PROTOCOL_VERSION = 2;
const DAEMON_CAPABILITIES = [
  'screenshot',
  'tiles',
] as const satisfies readonly DaemonCapability[];

function encodeFrame(payload: Buffer): Buffer {
  const header = Buffer.allocUnsafe(HEADER_BYTES);
  header.writeUInt32LE(payload.length, 0);
  return Buffer.concat([header, payload]);
}

function encodeRequest(request: unknown): Buffer {
  return encodeFrame(Buffer.from(JSON.stringify(request), 'utf8'));
}

// Reassembles frames out of whatever sizes the pipe hands over.
//
// A stream is not a sequence of messages: one read can carry half a header, or
// three responses and the start of a fourth. Everything downstream assumes
// whole frames, so this is the only place that has to know that.
class FrameReader {
  private buffer: Buffer = Buffer.alloc(0);

  push(chunk: Buffer): void {
    this.buffer = this.buffer.length === 0 ?
        chunk :
        Buffer.concat([this.buffer, chunk]);
  }

  // The next complete frame, or null when there is not one yet.
  next(): Buffer|null {
    if (this.buffer.length < HEADER_BYTES) {
      return null;
    }
    const length = this.buffer.readUInt32LE(0);
    if (this.buffer.length < HEADER_BYTES + length) {
      return null;
    }
    const frame = this.buffer.subarray(HEADER_BYTES, HEADER_BYTES + length);
    this.buffer = this.buffer.subarray(HEADER_BYTES + length);
    return frame;
  }
}

export {
  DAEMON_CAPABILITIES,
  DAEMON_PROTOCOL_VERSION,
  HEADER_BYTES,
  encodeFrame,
  encodeRequest,
  FrameReader,
};
