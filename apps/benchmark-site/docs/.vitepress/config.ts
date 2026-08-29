import {access, cp} from 'node:fs/promises';
import {join} from 'node:path';
import {fileURLToPath, URL} from 'node:url';
import {defineConfig} from 'vitepress';

const benchmarkResults = fileURLToPath(new URL('../../../../benchmark-results', import.meta.url));

function siteBase(value: string | undefined): string {
  if (!value) return '/';
  const withLeadingSlash = value.startsWith('/') ? value : `/${value}`;
  return withLeadingSlash.endsWith('/') ? withLeadingSlash : `${withLeadingSlash}/`;
}

const base = siteBase(process.env.BENCHMARK_SITE_BASE);

export default defineConfig({
  lang: 'zh-CN',
  title: 'Shotium 六平台性能报告',
  description: 'Shotium 六平台公平基准测试、正式排名与失败证据。',
  base,
  cleanUrls: true,
  appearance: false,
  head: [
    ['link', {rel: 'icon', type: 'image/svg+xml', href: `${base}favicon.svg`}],
  ],
  themeConfig: {
    logo: '/favicon.svg',
    siteTitle: 'Shotium 性能报告',
    nav: [
      {text: '六平台结果', link: '/#results'},
      {text: 'GitHub', link: 'https://github.com/sj817/shotium'},
    ],
    search: {
      provider: 'local',
      options: {
        translations: {
          button: {
            buttonText: '搜索',
            buttonAriaLabel: '搜索报告',
          },
          modal: {
            displayDetails: '显示详细列表',
            resetButtonTitle: '清除搜索条件',
            backButtonTitle: '关闭搜索',
            noResultsText: '没有找到相关结果',
            footer: {
              selectText: '选择',
              selectKeyAriaLabel: '回车',
              navigateText: '切换',
              navigateUpKeyAriaLabel: '向上',
              navigateDownKeyAriaLabel: '向下',
              closeText: '关闭',
              closeKeyAriaLabel: '退出',
            },
          },
        },
      },
    },
    sidebarMenuLabel: '菜单',
    returnToTopLabel: '返回顶部',
    skipToContentLabel: '跳转到报告内容',
    langMenuLabel: '切换语言',
    socialLinks: [
      {icon: 'github', link: 'https://github.com/sj817/shotium'},
    ],
    footer: {
      message: '同平台、同场景、同运行器内公平比较',
      copyright: 'Shotium 基准结果归档',
    },
  },
  vite: {
    define: {
      __VUE_I18N_FULL_INSTALL__: true,
      __VUE_I18N_LEGACY_API__: false,
      __INTLIFY_DROP_MESSAGE_COMPILER__: false,
      __VUE_PROD_DEVTOOLS__: false,
    },
    ssr: {
      noExternal: ['vue-i18n'],
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
