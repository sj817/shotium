# demo-hello

[English](./README.md) · 简体中文

提供一个最小化的静态 HTML 页面测试用例：仅包含标题、段落及基础内联样式，无任何外部子资源请求。主要用于快速排查环境配置或验证基础渲染链路。

## 用法

```bash
shotium hello.html --width 800 --height 600 -o hello.png
```

目前各语言示例工程默认渲染 [`apps/demo-card/card.html`](../demo-card/README.zh.md)；本示例作为独立测试文件保留供调试参考。

## 许可证

本项目遵循与上游 Chromium 一致的 BSD-3-Clause 开源协议，详情参见 [LICENSE](../../LICENSE)

