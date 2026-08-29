import DefaultTheme from 'vitepress/theme';
import type {Theme} from 'vitepress';
import BenchmarkDashboard from '../../components/BenchmarkDashboard.vue';
import {i18n} from '../../lib/i18n';
import './custom.css';

export default {
  extends: DefaultTheme,
  enhanceApp({app}) {
    app.use(i18n);
    app.component('BenchmarkDashboard', BenchmarkDashboard);
  },
} satisfies Theme;
