/*
 * Tree-shaken ECharts registration. Imported dynamically by the chart
 * components, so none of this reaches the SSR bundle or the first paint.
 */
import {use} from 'echarts/core';
import {BarChart} from 'echarts/charts';
import {GridComponent, TooltipComponent} from 'echarts/components';
import {SVGRenderer} from 'echarts/renderers';

use([BarChart, GridComponent, TooltipComponent, SVGRenderer]);
