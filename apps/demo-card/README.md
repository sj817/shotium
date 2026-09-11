# demo-card

English · [简体中文](./README.zh.md)

The canonical boarding-pass card template shared across shotium language demos: contains the self-contained HTML file, the demonstration Node.js script, and assets used for README media recordings.

## Files

| File | Role |
|---|---|
| `card.html` | Canonical boarding-pass template. Uses inline CSS with zero external subresources, dimensioned at 720×380 CSS pixels. Acts as the single source of truth for copies under `apps/{go,python,rust,csharp,java}/` |
| `card.mjs` | Node.js script demonstrated in the root README: illustrates `start()`, sequential captures, and `stop()` lifecycle |
| `cli-session.txt` | Recorded terminal session transcript rendered into `apps/docs/assets/example-cli.webp` |
| `demo.tape` | [VHS](https://github.com/charmbracelet/vhs) terminal recording script generating `apps/docs/assets/demo.gif` |

## Usage

After modifying `card.html`, synchronize updates across the language demos:

```bash
pnpm demo:sync           # copies card.html into apps/{go,python,rust,csharp,java}/
pnpm demo:sync --check   # CI consistency check: fails if any copy differs from source
```

Rebuild documentation media assets when template styling or scripts change:

```bash
pnpm docs:assets   # card.webp, example-node.webp, example-cli.webp (needs freeze and ffmpeg)
pnpm docs:demo     # demo.gif (needs vhs, ttyd, ffmpeg and bash)
```

Both commands initialize a temporary `.demo-run/` environment with the published `@pixel.js/shotium` npm package, ensuring documentation recordings reflect production package behavior rather than in-tree artifacts. This directory is gitignored.

Render the card manually via CLI:

```bash
shotium card.html --width 720 --height 380 --scale 2 -o card.png
```

## License

BSD-3-Clause, matching upstream Chromium. See [LICENSE](../../LICENSE)

