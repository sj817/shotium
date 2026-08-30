import {createReadStream, statSync} from 'node:fs';
import {access, cp} from 'node:fs/promises';
import {join, normalize} from 'node:path';
import {fileURLToPath, URL} from 'node:url';
import {defineConfig, type Plugin} from 'vitepress';

/*
 * The result archive lives at the repository root. In dev it is served
 * straight from there by the middleware below; at build time buildEnd copies
 * it into dist so the deployed site reads the very same files. The site's
 * source never embeds a snapshot of any result.
 */
const benchmarkResults = fileURLToPath(new URL('../../../../benchmark-results', import.meta.url));

function serveBenchmarkResults(): Plugin {
  return {
    name: 'serve-benchmark-results',
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        const rawUrl = req.url?.split('?')[0] || '';
        const marker = '/benchmark-results/';
        const idx = rawUrl.indexOf(marker);
        if (idx !== -1) {
          const subPath = decodeURIComponent(rawUrl.slice(idx + marker.length));
          const safePath = normalize(join(benchmarkResults, subPath));
          if (safePath.startsWith(benchmarkResults)) {
            try {
              const stat = statSync(safePath);
              if (stat.isFile()) {
                if (safePath.endsWith('.json')) {
                  res.setHeader('Content-Type', 'application/json; charset=utf-8');
                } else if (safePath.endsWith('.jsonl')) {
                  res.setHeader('Content-Type', 'application/x-ndjson; charset=utf-8');
                } else if (safePath.endsWith('.md') || safePath.endsWith('.csv')) {
                  res.setHeader('Content-Type', 'text/plain; charset=utf-8');
                }
                res.setHeader('Access-Control-Allow-Origin', '*');
                createReadStream(safePath).pipe(res);
                return;
              }
            } catch {}
          }
        }
        next();
      });
    },
  };
}

function siteBase(value: string | undefined): string {
  if (!value) return '/';
  const withLeadingSlash = value.startsWith('/') ? value : `/${value}`;
  return withLeadingSlash.endsWith('/') ? withLeadingSlash : `${withLeadingSlash}/`;
}

const base = siteBase(process.env.BENCHMARK_SITE_BASE);

/*
 * Two locales, one report. VitePress owns the language switch (rendered on the
 * right of the nav bar) and the appearance toggle; the report reads the active
 * locale from `useData().lang` and never draws its own language button.
 */
export default defineConfig({
  base,
  cleanUrls: true,
  appearance: true,
  lastUpdated: false,
  head: [
    ['link', {rel: 'icon', type: 'image/svg+xml', href: `${base}favicon.svg`}],
    ['meta', {name: 'theme-color', content: '#146c43'}],
  ],
  locales: {
    root: {
      label: '简体中文',
      lang: 'zh-CN',
      title: 'Shotium 六平台性能报告',
      description: 'Shotium 在六个 OS/CPU 平台上与 Puppeteer、Playwright 的同机公平基准、排名与失败证据。',
      themeConfig: {
        siteTitle: 'Shotium 性能报告',
        sidebarMenuLabel: '菜单',
        returnToTopLabel: '返回顶部',
        skipToContentLabel: '跳到报告内容',
        langMenuLabel: '切换语言',
        darkModeSwitchLabel: '外观',
        lightModeSwitchTitle: '切换到浅色',
        darkModeSwitchTitle: '切换到深色',
        footer: {
          message: '基于 Apache-2.0 / MIT 协议开源 · 由 <a href="https://github.com/sj817" target="_blank" rel="noreferrer" style="color: var(--brand); font-weight: 600;">sj817</a> 维护 · 数据由 GitHub Actions 自动化归档',
          copyright: 'Copyright © 2026 Shotium Project · <a href="https://github.com/sj817/shotium" target="_blank" rel="noreferrer" style="color: var(--brand); font-weight: 600;">GitHub: sj817/shotium</a>',
        },
      },
    },
    en: {
      label: 'English',
      lang: 'en',
      link: '/en/',
      title: 'Shotium Six-platform Benchmark',
      description: 'Same-machine fair benchmark of Shotium against Puppeteer and Playwright on six OS/CPU platforms, with rankings and failure evidence.',
      themeConfig: {
        siteTitle: 'Shotium Benchmark',
        langMenuLabel: 'Change language',
        footer: {
          message: 'Released under the Apache-2.0 / MIT License · Maintained by <a href="https://github.com/sj817" target="_blank" rel="noreferrer" style="color: var(--brand); font-weight: 600;">sj817</a> · Automated via GitHub Actions',
          copyright: 'Copyright © 2026 Shotium Project · <a href="https://github.com/sj817/shotium" target="_blank" rel="noreferrer" style="color: var(--brand); font-weight: 600;">GitHub: sj817/shotium</a>',
        },
      },
    },
  },
  themeConfig: {
    logo: '/favicon.svg',
    socialLinks: [
      {icon: 'github', link: 'https://github.com/sj817/shotium'},
    ],
  },
  vite: {
    plugins: [serveBenchmarkResults()],
    define: {
      __VUE_PROD_DEVTOOLS__: false,
    },
    ssr: {
      // ESM-only chart stack must be bundled into the SSR build, not required by Node.
      noExternal: ['element-plus', '@element-plus/icons-vue', 'vue-echarts', 'echarts'],
    },
  },
  async buildEnd(siteConfig) {
    await access(join(benchmarkResults, 'index.json'));
    await cp(benchmarkResults, join(siteConfig.outDir, 'benchmark-results'), {
      recursive: true,
      force: true,
    });
  },
});
