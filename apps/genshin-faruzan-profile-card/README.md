# 珐露珊 · 旅行者档案分享卡片

以《原神》角色「珐露珊」为视觉主角的横向个人资料分享卡片(1600 × 900,16:9)。
使用 Vite 托管纯 HTML + CSS + 原生 JavaScript，不包含 React、图层合成或后端服务。
全局使用《原神》游戏字体(汉仪文黑 / HYWenHei)。

![预览](preview.png)

## 目录结构

```text
faruzan-profile-card/
├─ index.html          页面结构与全部 SVG 装饰(神之眼、命之座、卷草角饰等)
├─ style.css           视觉样式(颜色/阴影/圆角统一由 CSS 变量管理)
├─ script.js           资料配置 + 渲染 + 卡片等比缩放 + 背景粒子
├─ preview.png         最终效果截图
└─ assets/
   ├─ character/faruzan-splash.png     珐露珊官方祈愿立绘(透明 PNG,2048×1024)
   ├─ icons/faruzan-icon.png           珐露珊官方头像图标
   ├─ icons/weapon-elegy.png           武器「终末嗟叹之诗」官方图标
   ├─ background/namecard-faruzan.png  珐露珊「机关」名片纹样(用作养成面板底纹)
   └─ fonts/
      ├─ HYWenHei-Extended.ttf         原神游戏字体(全局)
      └─ GenshinSumeru.ttf             须弥文装饰字体(仅右缘装饰条)
```

## 如何启动

安装依赖并启动 Vite:

```powershell
npm install
npm run dev
```

浏览器访问终端显示的本地地址。字体与图片均由 Vite 通过 HTTP 提供。

仅针对 PC 桌面浏览器设计(Chrome / Edge 最佳)。

## 如何修改玩家资料

打开 `script.js`,顶部的 `profileData` 配置对象集中管理全部文案数据:

```js
const profileData = {
  nickname: '知论派前辈',
  uid: '100000001',
  server: '天空岛',
  adventureRank: 60,
  worldLevel: 8,
  // ... 签名、成就、深渊、角色等级、命座、武器、词条等
};
```

改完保存刷新即可。也支持 **URL 参数**临时覆盖(无需改代码):

```text
index.html?nickname=旅行者&uid=123456789&critRate=75.0%
```

参数名与 `profileData` 的键一致。

## 如何替换角色图片

1. 将新的透明背景立绘放入 `assets/character/`;
2. 修改 `index.html` 中 `.character-art` 的 `src`;
3. 如构图偏移,在 `style.css` 中调整 `.character-art` 的 `width / left / top`
   以及遮罩渐隐位置(`mask-image` 两条渐变)。

## 推荐截图尺寸

- 卡片基准尺寸为 **1600 × 900**;页面会随窗口等比缩放,任意窗口下截取卡片区域均不会错位。
- 推荐把浏览器窗口调到 ≥ 1760 × 1000 后用外部工具框选截取卡片本体;
  需要 2x 高清图时可将系统缩放 / 浏览器缩放设为 200% 再截。
- 页面动画均为缓慢环境动效,任意时刻截图画面都是完整稳定的;
  系统开启「减弱动态效果」时动画自动停止(`prefers-reduced-motion`)。

## 素材与资料来源

| 素材 | 来源 |
| --- | --- |
| 珐露珊祈愿立绘 / 头像 / 武器图标 / 名片纹样 | 《原神》官方游戏资源,经 [enka.network](https://enka.network) UI 镜像获取(`UI_Gacha_AvatarImg_Faruzan` 等) |
| 角色资料核验(命之座「蔓藤花饰座」、室罗婆耽学院、武器内部名) | [Project Amber / gi.yatta.moe](https://gi.yatta.moe) 公开 API |
| 风元素符号矢量路径 | 开源项目 [frzyc/genshin-optimizer](https://github.com/frzyc/genshin-optimizer)(MIT) |
| 原神字体 HYWenHei Extended | [cawa-93/HYWenHei-Extended-Font](https://github.com/cawa-93/HYWenHei-Extended-Font) |
| 须弥文装饰字体 | [thomas200593/genshin-fonts-collections](https://github.com/thomas200593/genshin-fonts-collections)(社区手工字体) |

神之眼徽记、机关圆盘、命之座星图、卷草角饰、风场流线等装饰均为本项目手绘 SVG / CSS。

> 版权说明:《原神》游戏素材与「汉仪文黑」字体版权归 miHoYo / HoYoverse 及汉仪字库所有,
> 本项目仅用于个人非商业性质的资料卡片分享,请勿用于商业用途。

## 已知限制

- 游戏立绘、字体为官方素材,不可商用;
- 面板使用了 `backdrop-filter` 毛玻璃,在个别老旧显卡/浏览器上可能退化为纯半透明;
- 仅适配 PC 横屏浏览,未针对移动端竖屏做布局;
- 页面未内置导出图片功能(按需求使用外部截图工具)。
