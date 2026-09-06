// The one reporting format every check suite uses.
//
//   == section ==
//     PASS  label   detail
//     FAIL  label   detail
//
//   ALL CHECKS PASSED | N CHECK(S) FAILED
//
// It is the format the Python and CommonJS suites printed, kept because the
// engine workflows and the people reading their logs know it, and because a
// suite is one process per engine build: one flat list of labelled assertions
// says more than a test runner's tree would.

import {createHash} from 'node:crypto';

export class Checks {
  failures = 0;

  check(ok: boolean, label: string, detail: string = ''): boolean {
    console.log(`  ${ok ? 'PASS' : 'FAIL'}  ${label}${detail ? '   ' + detail : ''}`);
    if (!ok) this.failures += 1;
    return ok;
  }

  skip(label: string, detail: string = ''): void {
    console.log(`  SKIP  ${label}${detail ? '   ' + detail : ''}`);
  }

  section(title: string): void {
    console.log(`\n== ${title} ==`);
  }

  // Prints the summary line and returns the process exit code.
  finish(): number {
    console.log(`\n${this.failures ? `${this.failures} CHECK(S) FAILED` : 'ALL CHECKS PASSED'}`);
    return this.failures ? 1 : 0;
  }
}

export function sha256(data: Uint8Array | string): string {
  return createHash('sha256').update(data).digest('hex');
}
