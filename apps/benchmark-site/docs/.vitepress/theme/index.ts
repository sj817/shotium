import DefaultTheme from 'vitepress/theme';
import type {Theme} from 'vitepress';
import {
  ElAlert,
  ElButton,
  ElCard,
  ElCol,
  ElCollapse,
  ElCollapseItem,
  ElConfigProvider,
  ElEmpty,
  ElIcon,
  ElLink,
  ElOption,
  ElPagination,
  ElPopover,
  ElRow,
  ElSelect,
  ElStatistic,
  ElSwitch,
  ElTable,
  ElTableColumn,
  ElTag,
  ElTooltip,
} from 'element-plus';
import 'element-plus/dist/index.css';
import 'element-plus/theme-chalk/dark/css-vars.css';
import Layout from './Layout.vue';
import BenchmarkReport from '../../components/BenchmarkReport.vue';
// Order matters: Element Plus first, then the tokens (the single source of
// colour, type and glass parameters), the glass material + VitePress
// overrides, and finally the report styles that map Element Plus's variables
// onto the tokens.
import './tokens.css';
import './glass.css';
import './report.css';

const elementPlusComponents = [
  ElAlert,
  ElButton,
  ElCard,
  ElCol,
  ElCollapse,
  ElCollapseItem,
  ElConfigProvider,
  ElEmpty,
  ElIcon,
  ElLink,
  ElOption,
  ElPagination,
  ElPopover,
  ElRow,
  ElSelect,
  ElStatistic,
  ElSwitch,
  ElTable,
  ElTableColumn,
  ElTag,
  ElTooltip,
] as const;

export default {
  extends: DefaultTheme,
  Layout,
  enhanceApp({app}) {
    for (const component of elementPlusComponents) app.use(component);
    app.component('BenchmarkReport', BenchmarkReport);
  },
} satisfies Theme;
