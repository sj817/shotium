// The entry point of a detached daemon process.
//
// The configuration arrives as one base64 argument rather than as flags,
// because it contains paths that a Windows command line would otherwise quote
// badly, and because the client and the daemon have to agree on it exactly:
// the endpoint is a hash of these fields, so a value mangled in transit would
// produce a daemon listening where nobody looks. See endpoint.ts.
//
// It is a build entry of its own, and not a chunk, because lib/client.ts
// spawns it by path -- `node dist/daemon_main.js <base64 json>` -- and a name
// the bundler chose would be a name that changes.

import {Daemon} from './lib/daemon.js';
import type {DaemonOptions} from './types.js';

async function main(): Promise<void> {
  const encoded = process.argv[2];
  if (!encoded) {
    process.stderr.write('shotium: daemon_main expects a base64 config\n');
    process.exit(2);
  }
  const options = JSON.parse(Buffer.from(encoded, 'base64').toString('utf8')) as
      DaemonOptions;
  const daemon = new Daemon(options);

  daemon.on('stderr', ({worker, line}: {worker: number, line: string}) => {
    process.stderr.write(`shotium worker ${worker}: ${line}\n`);
  });
  for (const event of ['crash', 'timeout', 'worker-restart', 'worker-error',
                       'idle-exit']) {
    daemon.on(event, (payload: {error?: unknown}) => {
      // An Error does not survive JSON.stringify -- it comes out as {} -- and
      // its message is the whole point of logging a worker that would not
      // start.
      const detail = payload && payload.error ?
          {
            ...payload,
            error: String(
                (payload.error as Error).message ?? payload.error),
          } :
          payload;
      process.stderr.write(
          `shotium daemon ${event}: ${JSON.stringify(detail)}\n`);
    });
  }
  // An 'error' with nobody listening is thrown by EventEmitter itself, which
  // would turn a socket that failed after binding -- something the daemon can
  // survive -- into a dead pool.
  daemon.on('error', (error: Error) => {
    process.stderr.write(
        `shotium daemon error: ${(error && error.message) || error}\n`);
  });

  try {
    await daemon.listen();
  } catch (error) {
    // Losing the race to bind is the ordinary outcome when two clients start a
    // daemon at the same moment: the other one is up, this one is not needed,
    // and the client that spawned it will connect to the winner. Anything else
    // is a real failure and says so.
    if ((error as NodeJS.ErrnoException | null)?.code === 'EADDRINUSE') {
      process.exit(0);
    }
    process.stderr.write(`shotium: daemon failed to start: ${error}\n`);
    process.exit(1);
  }

  const shutdown = () => {
    daemon.close().then(() => process.exit(0), () => process.exit(1));
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
  daemon.on('close', () => process.exit(0));
}

void main();
