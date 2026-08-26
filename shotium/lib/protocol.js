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

const HEADER_BYTES = 4;

function encodeFrame(payload) {
  const header = Buffer.allocUnsafe(HEADER_BYTES);
  header.writeUInt32LE(payload.length, 0);
  return Buffer.concat([header, payload]);
}

function encodeRequest(request) {
  return encodeFrame(Buffer.from(JSON.stringify(request), 'utf8'));
}

// Reassembles frames out of whatever sizes the pipe hands over.
//
// A stream is not a sequence of messages: one read can carry half a header, or
// three responses and the start of a fourth. Everything downstream assumes
// whole frames, so this is the only place that has to know that.
class FrameReader {
  constructor() {
    this._buffer = Buffer.alloc(0);
  }

  push(chunk) {
    this._buffer =
        this._buffer.length === 0 ? chunk :
                                    Buffer.concat([this._buffer, chunk]);
  }

  // The next complete frame, or null when there is not one yet.
  next() {
    if (this._buffer.length < HEADER_BYTES) {
      return null;
    }
    const length = this._buffer.readUInt32LE(0);
    if (this._buffer.length < HEADER_BYTES + length) {
      return null;
    }
    const frame = this._buffer.subarray(HEADER_BYTES, HEADER_BYTES + length);
    this._buffer = this._buffer.subarray(HEADER_BYTES + length);
    return frame;
  }
}

export {HEADER_BYTES, encodeFrame, encodeRequest, FrameReader};
