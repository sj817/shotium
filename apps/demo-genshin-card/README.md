# demo-genshin-card

English · [简体中文](./README.zh.md)

A horizontal profile card template (1600 × 900, 16:9) styled after the Genshin Impact character Faruzan. Built with standard HTML, CSS, and vanilla JavaScript served via Vite, requiring no frameworks or backend runtime. This page serves as the capture target for [demo-express](../demo-express/README.md).

![preview](preview.png)

## Layout

```text
demo-genshin-card/
├─ index.html          structure and every inline SVG ornament (Vision, constellation, scroll corners)
├─ style.css           visual styling; colours, shadows and radii are CSS variables
├─ script.js           profile data, rendering, proportional scaling, background particles
├─ preview.png         the rendered result
└─ assets/
   ├─ character/faruzan-splash.png     official splash art, transparent PNG, 2048 × 1024
   ├─ icons/faruzan-icon.png           official avatar icon
   ├─ icons/weapon-elegy.png           official icon of the weapon "Elegy for the End"
   ├─ background/namecard-faruzan.png  the "Mechanism" namecard pattern behind the build panel
   └─ fonts/
      ├─ HYWenHei-Extended.ttf         the game's font, used throughout
      └─ GenshinSumeru.ttf             the Sumeru script decoration on the right edge
```

## Run

```bash
pnpm install --frozen-lockfile
pnpm run dev
```

Navigate to the URL printed by Vite. Fonts and image assets are served over HTTP. The layout is optimized for desktop browsers, especially Chromium-based engines such as Chrome and Edge.

## Editing the profile

`profileData` at the top of `script.js` holds every piece of text:

```js
const profileData = {
  nickname: '知论派前辈',
  uid: '100000001',
  server: '天空岛',
  adventureRank: 60,
  worldLevel: 8,
  // signature, achievements, Spiral Abyss, level, constellation, weapon, stats ...
};
```

Save changes and reload. Query parameters can dynamically override configuration fields without modifying the source file:

```text
index.html?nickname=Traveler&uid=123456789&critRate=75.0%
```

## Replacing the character art

1. Put a new transparent splash into `assets/character/`
2. Change the `src` of `.character-art` in `index.html`
3. If the composition shifts, adjust `width`, `left` and `top` of `.character-art` in `style.css`, and the two gradient stops of its `mask-image`

## Capturing

The card layout has a baseline dimension of 1600 × 900 CSS pixels with proportional viewport scaling. Visual animations are designed as ambient background loops that automatically pause when `prefers-reduced-motion` is active. For automated capture (as demonstrated in `demo-express`), target `selector: '#card'`; for manual CLI captures, configure a viewport of at least 1760 × 1000 with `scale: 2` for high-resolution output.

## Sources

| Asset | Origin |
|---|---|
| Splash art, avatar icon, weapon icon, namecard | Official game assets, mirrored by [enka.network](https://enka.network) (`UI_Gacha_AvatarImg_Faruzan` and others) |
| Character facts (constellation "Flosculi Implexi", Haravatat, weapon internal name) | [Project Amber / gi.yatta.moe](https://gi.yatta.moe) public API |
| Anemo element vector path | [frzyc/genshin-optimizer](https://github.com/frzyc/genshin-optimizer) (MIT) |
| HYWenHei Extended font | [cawa-93/HYWenHei-Extended-Font](https://github.com/cawa-93/HYWenHei-Extended-Font) |
| Sumeru script font | [thomas200593/genshin-fonts-collections](https://github.com/thomas200593/genshin-fonts-collections), a community font |

The Vision emblem, mechanism disc, constellation chart, scroll corners and wind streaks are hand-drawn SVG and CSS in this directory.

Game assets and the HYWenHei font belong to miHoYo / HoYoverse and Hanyi Fonts. This card is a personal, non-commercial demo; do not use it commercially.

## Considerations and limitations

- Artwork, icons, and typeface assets are copyrighted game materials intended exclusively for non-commercial personal demonstration
- Glassmorphism styling relies on CSS `backdrop-filter`, gracefully falling back to semi-transparent backgrounds in software-rendered or unsupported GPU environments
- Optimized strictly for desktop landscape viewing; responsive portrait layout is not implemented
- The page does not include an in-browser export action; use the shotium CLI or SDK for capturing

## License

The code in this directory is BSD-3-Clause like the rest of the repository; assets remain the property of their respective owners. See [LICENSE](../../LICENSE)

