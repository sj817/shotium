import { statSync } from 'node:fs';
import shotium, { screenshot } from '@shotkit/shotium';

// One engine per process: Blink's process-wide statics
// cannot be re-initialised, so start() is idempotent.
shotium.start();

function shoot() {
  return screenshot({
    file: 'card.html',        // URL, local path or file://
    viewport: { width: 720, height: 380 },
    scale: 2,                 // device pixel ratio
    type: 'png',
    path: 'card.png',         // to disk; `image` is then null
  });
}

// The first capture also pays for Blink, Skia and font warm-up.
for (const pass of ['cold', 'warm']) {
  const { render, total } = (await shoot()).stats.timing;
  console.log(`${pass}  render ${render.toFixed(1)} ms  total ${total.toFixed(1)} ms`);
}

const kb = (statSync('card.png').size / 1024).toFixed(1);
console.log(`card.png  1440x760  ${kb} KB`);

await shotium.stop();
