[English](README.md) · **简体中文**

# shotium

用裁剪过的 Chromium 做静态截图。DOM、CSS、布局、绘制、字体、图片、HTTP —— 一个页面「看起来正确」所需要的全部。

**没有 JavaScript 引擎。** V8 不是被禁用，是被删掉了：连同当初用来承载它的整个浏览器层，一起从源码树里移除。剩下的是直接使用的 Blink，最终是一个自包含的可执行文件 —— Windows 上 41 MB，压缩后 12.8 MB —— 启动耗时不到一秒。

```js
import shotium from '@shotkit/shotium';

shotium.runtime.start({workers: 4});

const png = await shotium.screenshot({
  file: 'https://example.com',
  viewport: {width: 1280, height: 720},
  fullPage: true,
});

await shotium.runtime.stop();
```

---

## 这个仓库是什么

一个删掉了绝大部分内容的 Chromium 分支，加上两件属于本项目的东西：

| | |
|---|---|
| `shot/` | 引擎。约 1,800 行 C++，直接驱动 Blink |
| `shotium/` | npm 包。进程池、队列、重试、类型声明 —— 纯 JavaScript，另外带一个原生 addon |
| `tools/shot/` | 检查脚本：协议、几何、网络，以及 JavaScript 层 |
| `bench/` | 基准测试：对同一棵树构建出的 Chromium，以及对 puppeteer 和 playwright |
| `docs/cut-progress.md` | 这棵树是怎么裁的，以及裁的过程中弄坏了什么 |
| `docs/shotium-plan.md` | 当前设计，以及明确不做的部分 |

其余部分是 Chromium 本身，移除了 1.4 GB 构建过程从不读取的内容，所以克隆下来是 0.57 GB 而不是 2 GB。

## 它不是什么

不是浏览器。没有 `//content`，没有渲染进程、GPU 进程、合成器、沙箱、扩展、开发者工具，也没有 V8。

由此带来的限制值得直说，因为这些是产品边界，不是待修的缺陷列表：

- **靠脚本把自己构建出来的页面，拍出来是一张空白页。** 没有 `<script>`，没有框架 hydration，没有客户端路由。如果目标是一个没有服务端渲染的 React 应用，选错工具了。
- `selector` 在渲染器内部用 `Document::querySelector` 求值。不会向页面注入任何东西，因为没有可以用来注入的东西。
- **宿主决定有哪些字体可用，但不决定字体怎么光栅化。** 文本使用灰度抗锯齿、固定伽马，不涉及次像素几何，也不读取宿主的 ClearType 设置 —— 所以同一个平台上，任何进程渲染同一个页面都得到逐字节相同的结果。两个**不同**平台之间仍有一丝差异，落在字形边缘：CoreText 和 DirectWrite 对 advance（字形步进）的取整方式不同。另外，宿主没有的字体，页面也拿不到。
- **声明了旧式编码的页面会渲染成乱码。** `ForceSynchronousDocumentInstall` 安装文档时把编码写死为 UTF-8，而显式指定的编码优先级高于 `<meta charset>`，所以页面里的 `shift_jis` 或 `gbk` 声明永远不会被采纳。`tools/shot/charset_check.py` 每轮都会实测这件事，而不是假定它没问题；将来修复它的人应当重跑这个脚本。
- 没有 SSRF 防护，除了响应体大小上限之外没有任何资源限制。哪些 URL 允许被请求，由调用方决定。

## 使用

```bash
npm install @shotkit/shotium
```

这是一个 ES module，而且只有这一种形态。`import` 在哪里都能用；`require()` 需要
Node 20.19 或 22.12 以上，更老的版本上 CommonJS 调用方用
`await import('@shotkit/shotium')`。

只发一种格式是有意的。这个包里的东西全是进程级单例 —— `runtime` 就是那一个池，
daemon 用配置的哈希寻址以保证两个调用方找到同一个进程，而 blink 会直接拒绝在一个
已经有过引擎的进程里再起第二个。同时提供 CJS 和 ESM 两份的包，会让同时用两种方式
引它的调用方各拿到两份：多出一个没人要的池，和一个起不来的引擎 —— 而且报错的地方
离那句 `require` 很远。

两个入口，除此之外的路径都不可达：

| | |
|---|---|
| `@shotkit/shotium` | `runtime`、`screenshot`、`daemon`、`Runtime` |
| `@shotkit/shotium/native` | `native`、`screenshot`、`NativeRuntime` |

引擎是 41 MB 的 Chromium，每个平台和架构各有一份，所以它不在这个包里，而在另外六个包里 —— `@shotkit/shotium-win-x64`、`@shotkit/shotium-mac-arm64`，以及其余四个 —— 以 `optionalDependencies` 声明，并带上 `os` 和 `cpu`。npm 只装匹配当前机器的那一个，其余五个跳过。没有 postinstall 脚本，也不从 registry 之外下载任何东西：lockfile 锁住的就是你拿到的。

同样这些引擎也在 [releases 页面]，每个版本六个压缩包，给不从 npm 安装的调用方：

| | x64 | arm64 |
|---|---|---|
| Windows | `shotium-win-x64-v0.1.0.7z` | `shotium-win-arm64-v0.1.0.7z` |
| macOS | `shotium-mac-x64-v0.1.0.7z` | `shotium-mac-arm64-v0.1.0.7z` |
| Linux | `shotium-linux-x64-v0.1.0.7z` | `shotium-linux-arm64-v0.1.0.7z` |

每个压缩包解开后是一个目录，目录名只包含平台和架构 —— `shotium-win-x64/`、`shotium-mac-arm64/` —— 里面是可执行文件和两个 `.pak` 文件。版本号写在压缩包上而不是目录里，所以新版本解压到同一位置就是原地覆盖旧版本。

用 `SHOTIUM_BINARY` 把包指向解压后的引擎，它的优先级高于平台包：

```bash
export SHOTIUM_BINARY=/path/to/shotium-win-x64/shotium.exe    # 非 Windows 平台是 `shotium`
```

两者都没有时，包会在 `index.js` 同级目录下找 `bin/shotium.exe` —— macOS 和 Linux 上是 `bin/shotium`。

```js
import {runtime, screenshot} from '@shotkit/shotium';

runtime.on('crash',   ({worker})          => console.warn('worker', worker, 'died'));
runtime.on('timeout', ({worker, timeout}) => console.warn('worker', worker, 'hung'));
runtime.on('stderr',  ({line})            => console.error(line));

runtime.start({
  workers: 4,                              // 默认：核心数的一半
  cacheDir: '/var/tmp/shotium-cache',       // null 表示关闭 HTTP 缓存
});

await screenshot({file: 'https://example.com', path: 'out.png'});
```

`screenshot()` 返回一个 `Buffer`；如果传了 `path`，文件由 worker 自己写，此时返回 `null`。

### 常驻守护进程

`runtime` 的 worker 活多久，取决于持有它的那个 Node 进程活多久。命令行、CI 步骤、队列 worker 都是「为了一张图而存在」的进程，对它们来说，起 worker 就是这张图的主要成本。

`daemon` 是同一个进程池，放在一个 socket 后面 —— Windows 上是命名管道，其他平台是 unix socket —— 下一个进程连上去时，它已经起好并且预热过了：

```js
import {daemon} from '@shotkit/shotium';

const client = await daemon.connect({workers: 4});   // 没有就先拉起一个
const png = await client.screenshot({file: 'https://example.com'});
client.close();

await daemon.status();    // {running, pid, workers, served, warm, ...}
await daemon.stop();
```

```bash
npx @shotkit/shotium https://example.com -o out.png    # 第一次调用会把守护进程拉起来
npx @shotkit/shotium daemon status
```

守护进程的端点是它自身配置的哈希 —— 二进制、worker 数、缓存目录、附加 flag —— 所以客户端不会悄悄连上一个「用别的东西渲图」的池子。最后一个客户端离开五分钟后它自己退出；一条连接上可以同时跑多个请求。

### 或者，就在本进程里

`runtime` 和 `daemon` 都是把 blink 放在 worker 进程里。原生引擎把它放在这里：

```js
import {native} from '@shotkit/shotium/native';

const png = await native.screenshot({file: 'https://example.com'});
native.purge({releaseWorkingSet: true});   // 一批完了之后
await native.stop();
```

选项一样，字节也一样 —— `tools/shot/native_check.cjs` 会拿同一份文档去比对可执行文件的输出，因为两条通向 blink 的路径如果可能不一致，那就是两个引擎。不同的是形状：一个进程而不是五个，一张图约 **31 ms** 而不是 47 ms，工作集约 **81 MiB** 而不是 190 MiB。

它放弃的，是单独一个进程本来白送的两件事。**只有一个渲染器** —— blink 是进程级单例，`worker_threads` 共享同一个进程，所以无论多少个调用方，请求都是串行的，进程池那个四并发在这里没有对应物。以及**没有崩溃隔离**：渲染器挂了会把宿主程序一起带走，而进程池会换一个 worker 重试。

下面是一个带 C ABI 的共享库，[`shot/shot_api.h`](shot/shot_api.h) —— 八个函数、不透明指针、进去 JSON 出来字节，没有任何内存的所有权跨过这道缝。上面的 addon 是一层很薄的 Node-API，它不读自己搬的东西。这个头文件并不绑定 node；ctypes、cgo、libloading 都能直接用。

两者都以预构建的形式放在平台包里，和它们所属的引擎挨着，因为底下那个库是一次 Chromium 构建，`npm install` 做不了这件事。`@shotkit/shotium` 本身两者都不需要。

### 选项

```ts
interface ScreenshotOptions {
  file: string                        // http/https/file URL，或本地路径
  type?: 'png' | 'jpeg' | 'webp'      // 默认 png
  fullPage?: boolean                  // 整个文档，不只是视口
  selector?: string                   // 只截取某一个元素的盒子
  quality?: number                    // 1-100，仅 jpeg 和 webp，默认 90
  scale?: number                      // 设备像素比，0.01-8，默认 1
  omitBackground?: boolean            // 保留 alpha 通道，而不是填白
  path?: string                       // 写到这里，而不是返回字节
  viewport?: { width?: number; height?: number }        // 默认 1280x720
  pageGotoParams?: {
    timeout?: number                  // 毫秒，默认 30000
    waitUntil?: 'load' | 'networkidle'
  }
  clip?: { x: number; y: number; width: number; height: number }
  allowFileAccess?: boolean           // 允许文档读取 file: 子资源
  retry?: number                      // 崩溃或超时后重发
}
```

`selector`、`clip` 和 `fullPage` 都在指定截取区域，而且指的是不同的区域，所以同时给两个是错误，不会去猜。同理还有：给 png 传 `quality`，给 jpeg 传 `omitBackground` —— jpeg 没有 alpha 通道。能解析但无法满足的字段一律拒绝，不会静默忽略。

### 直接用二进制

```bash
shotium https://example.com --width 1280 --height 720 -o out.png
shotium --file page.html --full-page --type webp --quality 85 -o out.webp
shotium --serve --cache-dir /var/tmp/shotium-cache    # 常驻 worker，见 shot/shot_server.h
shotium --help
```

---

## 实测数据

在同一份语料上对 puppeteer 和 playwright：十个静态本地文档，1280x720，PNG，只截视口，`waitUntil: 'load'`，每张截图一个新页面，每个样本一棵全新进程树，每格 7 次，取中位数。方法和全部口径写在 [`bench/cross/`](bench/cross/README.md)；完整报告——每一列、每个场景，以及失败的样本——在 [`bench/cross/RESULTS.md`](bench/cross/RESULTS.md)。测试机是一台 32 核 Windows 台式机，测的时候它还在干别的事。

| 引擎 | 冷启动 | 单张 | 单张（复用页面） | 十张 · 四并发 | 该场景内存 |
|---|--:|--:|--:|--:|--:|
| **shotium** | **352 ms** | **47 ms** | — | **237 ms**（42 张/秒） | **256 MiB** / 私有 **73** |
| puppeteer · chrome-headless-shell | 946 ms | 133 ms | 61 ms | 905 ms（11 张/秒） | 647 MiB / 私有 180 |
| puppeteer · headless Chrome | 1559 ms | 132 ms | 50 ms | 1890 ms（5 张/秒） | 1287 MiB / 私有 379 |
| playwright · chrome-headless-shell | 962 ms | 150 ms | **33 ms** | 1171 ms（9 张/秒） | 652 MiB / 私有 215 |
| playwright · headless Chrome | 1385 ms | 146 ms | 45 ms | 1276 ms（8 张/秒） | 789 MiB / 私有 282 |

「冷启动」是一整个进程：node 启动、`require`、拉起引擎、截一张图。「单张」是引擎已经在跑之后再多截一张的边际成本，启动完全不在里面。

内存写成两个数，因为一棵进程树没有单一的那个数。前一个是工作集之和，也就是任务管理器加出来的数——它对进程间共享的页面按进程重复计费：四个 shotium worker 各自映射同一份 43 MiB 的 `shotium.exe`，二十一个 chrome 进程各自映射同一份 `chrome.dll`。后一个是私有工作集之和：只属于某一个进程的页面，一份不重复。真实开销在两者之间，这两个数把它夹住。

shotium 输掉的那一列反而是值得说的一列。让**同一个页面**在文档之间跳转，比每次新建一个页面快，Chrome 允许这么做；而 shotium 每个请求都要建一个 `Page` 再拆掉，没有对应的做法。这个优势到并发就停了：四个页面同时跑，Chrome 要付四个渲染进程和接近一个 GB，而四个 worker 同时应付四个请求正是 shotium 的形状。

引擎已经起着的时候——命令行调用、队列 worker、请求处理器——一个**全新进程**为一张图付出的是：

| 引擎 | 客户端端到端 | 连接 | 截图 | 常驻 | 仅引擎 | 常驻进程数 |
|---|--:|--:|--:|--:|--:|--:|
| **shotium 守护进程** | **250 ms** | **2.3 ms** | **57 ms** | **58 MiB** | **2.8 MiB** | 5 |
| puppeteer · chrome-headless-shell | 512 ms | 17 ms | 170 ms | 355 MiB | 287 MiB | 8 |
| puppeteer · headless Chrome | 588 ms | 19 ms | 204 ms | 587 MiB | 519 MiB | 12 |
| playwright · chrome-headless-shell | 764 ms | 38 ms | 189 ms | 272 MiB | 154 MiB | 5 |
| playwright · headless Chrome | 680 ms | 34 ms | 228 ms | 400 MiB | 299 MiB | 7 |

两边连的都是不是自己拉起来的东西：shotium 走命名管道，puppeteer 走 `browserWSEndpoint`，playwright 连 `launchServer()`。常驻那几列是「什么都不干的时候」各自的开销，每个引擎都先晾十五秒再采样。「仅引擎」是把 node 进程去掉之后剩下的部分，对 shotium 来说那几乎就是全部：队列安静十秒之后，worker 会回收 blink 的堆、丢掉缓存、把页面交还给操作系统，于是四个常驻渲染器加起来只占 2.8 MiB，58 MiB 里剩下的是管着它们的那个 node。下一个请求要付大约 8 ms 的软缺页把它们拿回来——见 [`shot/shot_runtime.h`](shot/shot_runtime.h) 里的 `PurgeMemory` 和 `ReleaseWorkingSet`。其中「把页面交还给操作系统」这一步是 Windows 的工作集裁剪，回收堆和丢缓存不是；Linux 上 PartitionAlloc 的回收器走 `madvise(MADV_DONTNEED)`，堆在那一步之前就已经还回去了。这张表本身也只是 Windows 的数：它测自一台主机，Linux 还没有对应的一轮。

**这里没有测的是脚本。** 语料全是静态文档，因为那才是 shotium 能拍的东西。页面靠脚本把自己建出来的场景，这些数字一概不适用——那种页面拍出来是空白的，任何跑分都改不了这一点。

---

## 构建引擎

这是一个 Chromium checkout，构建方式和 Chromium 一样 —— **但它是一个分支，这改变了一件事：`gclient` 不能管理 `src`。** 必须设置 `"managed": False`，否则下一次 `gclient sync` 会把整棵树重置回上游，本仓库的全部改动都会消失。

需要 [depot_tools] 在 `PATH` 上、约 40 GB 磁盘，以及对应平台的工具链：

| | |
|---|---|
| Windows | 装有 Windows SDK 的 Visual Studio；目标为 arm64 时还需要 arm64 工具集 |
| macOS | Xcode。`FORCE_MAC_TOOLCHAIN` 保持不设置 —— 密闭版 Xcode 需要 Google 之外无人拥有的访问权限 |
| Linux | `sudo ./build/install-build-deps.sh --no-prompt --no-nacl` |

这棵树把 Windows SDK 固定在 **10.0.28000**。这个固定值是一个目录名，不是兼容性声明，而且它写在两个必须保持一致的文件里：`build/vs_toolchain.py` 和 `build/toolchain/win/setup_toolchain.py`。如果本机装的是别的 SDK，用下面的方式指定，不要去改这两个文件 —— 同时要指定出自同一个 SDK 的 NTDDI 符号：

```bash
export CHROMIUM_WIN_SDK_VERSION=10.0.26100.0
# 以及在 out/Shot/args.gn 里：  win_ntddi_version = "NTDDI_WIN11_GE"
```

两个都要给。`win_ntddi_version` 默认是 `NTDDI_WIN11_BR`，而预处理器没见过的 NTDDI 标识符会当作 0 参与比较，不会报错 —— 于是指定一个该 SDK 并未定义的符号，会静默关掉所有按版本保护的声明，几千行之后才以 `unknown type name` 的形式爆出来。正确做法是取该 SDK 的 `shared/sdkddkver.h` 里真正定义过的最高 `NTDDI_WIN*`。CI 就是这么做的，因为 Chromium 自己的工具链包在 Google 之外下载不到，而托管 runner 上只有它镜像里自带的那个 SDK。

```bash
mkdir shotium-build && cd shotium-build

cat > .gclient <<'EOF'
solutions = [{
  "name": "src",
  "url": "https://github.com/sj817/shotium.git",
  "managed": False,
  "custom_deps": {},
  "custom_vars": {"checkout_configuration": "small"},
}]
target_os = ["win"]          # 或者 ["mac"]、["linux"]
EOF

gclient sync --nohooks --no-history
gclient runhooks

cd src

# ICU 数据集是生成的，不入库：third_party/icu 是 DEPS checkout，gclient 会把它丢掉。
# 这一步必须在 gn gen 之前跑。
python3 tools/shot/icu_repack.py \
  third_party/icu/cast/icudtl.dat \
  third_party/icu/shot/icudtl.dat --preset shot

mkdir -p out/Shot
echo 'import("//build/args/shot.gn")' > out/Shot/args.gn
gn gen out/Shot
ninja -C out/Shot shot
```

**`target_os` 决定了哪些 DEPS 条目根本存不存在。** 这棵树里签入的 `.gclient` 写的是 `["win"]`，所以在另一个平台上沿用它去 sync，会漏掉所有按该平台门控的条目，然后在很久以后失败 —— 报的是缺少头文件，而不是任何能指出真正原因的信息。

在 macOS 和 Linux 上，`import` 里改用 `build/args/shot-mac.gn` 或 `build/args/shot-linux.gn`。这两个文件都会 import `shot.gn` 再补上本平台需要的部分，所以配置决策仍然只有一处来源。

用 `import` 而不是 `--args="$(cat ...)"`：这个文件里有带引号的值，被插值进 shell 参数之后就不成立了。

`build/args/shot.gn` 是发布配置：official build、ThinLTO、`-Os`、关闭 DCHECK，以及一份逐条写明原因的功能关闭列表。里面每一个参数都是一个记录在案的体积或正确性决策。

### 并行度

Blink 的布局代码在最重的时候需要**每个编译进程 1.1–1.3 GB** —— `-j 20` 的峰值在 25 GB 左右。按轻量翻译单元的占用去估 `-j` 再外推，结果就是两小时之后收到九条 `LLVM ERROR: out of memory`。按峰值算预算，不要按平均值。

### 检查

```bash
python tools/shot/serve_check.py   out/Shot/shotium.exe   # 协议、几何、编码器
python tools/shot/net_check.py     out/Shot/shotium.exe   # http、重定向、缓存、TLS
node   tools/shot/node_check.cjs   out/Shot/shotium.exe   # 进程池、重试、崩溃隔离
node   tools/shot/daemon_check.cjs out/Shot/shotium.exe   # 常驻守护进程
node   tools/shot/native_check.cjs out/Shot/shotium.exe   # 进程内引擎
python tools/shot/charset_check.py out/Shot/shotium.exe   # 旧式编码与 ICU 裁剪的关系
python tools/shot/demo_check.py    out/Shot/shotium.exe   # 84 个 reftest
```

104 项检查加 84 个 reftest（参考比对测试）。需要 Node.js 的是三个 `.cjs`，它们在 Windows 和 Linux 上都跑 —— 守护进程在一边监听命名管道，在另一边监听 unix socket，只在一个平台上跑等于只检查了一半。其中值得单独说明的几条：

- **同一份文档，通过 HTTP 取回和从磁盘读取，必须产出完全相同的字节。** 传输方式不应该在图片里看得出来。
- **`clip` 和 `selector` 必须找到同一个盒子，精确到字节。** 它们是同一个矩形的两种指定方式；如果结果不一致，其中一个是错的。
- **请求处理到一半被杀掉的 worker，必须在另一个 worker 上返回同样的图片。** 整个多进程设计成立与否，就取决于这一条。
- **另一个进程必须找到已有的守护进程，而不是再起一个；拿到的字节必须和进程内的池子一致。** 两个会各自漂移的入口，就是两个渲染器。
- **每个 reftest 都用「裁剪不可能弄坏的 CSS」来表达预期结果** —— 一对必须渲染成相同像素的页面，而不是一张存好的基准图。基准图在每次裁剪之后都要重新确认，会把真正的回归淹没在噪声里。没有参考页的用例则是冒烟测试：必须能渲染出来、颜色多于一种，并且跑两次得到相同的字节。

`tools/shot/size_report.py` 通过 PDB 的 section contributions，把二进制的每一个字节归属到它来自的目标文件，既不需要 `/MAP` 重链接，也不需要提高 `symbol_level`。

---

## 工作原理

```
shotium (npm, plain JS)          pool · lifecycle · on() events · retry
     │
     │  stdio: length-prefixed JSON request / length-prefixed binary response
     ▼
shotium.exe --serve                 resident worker, one Blink per process
     │
     ▼
Blink   DOM → CSS → layout → paint → raster → encode
```

`shot_renderer.cc` 就是完整的渲染流程：

```
Page::CreateNonOrdinary + LocalFrame + LocalFrameView
  → LocalFrame::ForceSynchronousDocumentInstall("text/html", bytes, url)
  → LocalFrameView::UpdateAllLifecyclePhases()   style, layout, prepaint, paint
  → LocalFrameView::GetPaintRecord()             the paint phase's output
  → SkiaPaintCanvas over an SkSurface            CPU raster
  → PNG / JPEG / WebP
```

这不是对任何东西的重新实现。它就是 Blink 内部渲染 SVG 图片时已经在用的形态：一个完整文档，但不可能拥有自己的渲染进程。参见 `core/svg/graphics/isolated_svg_document_host.cc`。

**渲染为什么放在独立进程里：** Blink 是进程级单例。`Platform::InitializeBlink()` 只会建一次 WTF 和各个 partition，`cppgc::InitializeProcess()` 藏在 `NoDestructor` 后面，主线程调度器绑定在单一线程上。所以一个进程同一时刻只渲染一个文档，并发就意味着更多进程，而崩溃隔离是同一个事实的副产品。

**网络**是直接链接 `//net` —— `URLRequest`、HTTP 缓存、BoringSSL —— 上面没有 `//services/network`。那个 service 是多进程 Chrome 为这些对象套的 mojo 外壳，目的是让沙箱里的渲染器能够访问它们；而这里的 worker 本身就是独立进程，套上去只是一根连向自己的管道。HTTPS、HTTP/2、重定向、brotli、内存 cookie jar 和可选的磁盘缓存是开启的；HTTP/3 和代理没有。

## 体积

`icu_use_data_file = false` 会把 ICU 的数据表链接进可执行文件，所以没有 `icudtl.dat` 需要随包发布。实际发布的是二进制加两个 `.pak` 文件，后者合计 118 KB。

| | 未压缩 | .7z |
|---|---:|---:|
| Windows x64 | 41.5 MB | **12.80 MB** |
| Windows arm64 | | **10.56 MB** |
| macOS arm64 | 38.4 MB | **12.29 MB** |
| Linux x64 | 70.2 MB | **15.80 MB** |

起点是 336 MB。macOS 那一行的数字来自统一压缩格式之前该任务产出的 `tar.xz`；两种格式都是 LZMA2，所以预期变化在 KB 量级而不是 MB 量级。Linux 的未压缩体积是这张表里唯一还没有解释的数字 —— 同一份源码，70 MB 的 ELF 对 41 MB 的 PE，这个差距应当去测量，而不是找个说法糊过去。

`docs/cut-progress.md` 记录了完整过程，包括测量方法，以及下一批几 MB 在哪里 —— 都是量出来的，不是猜的。

## 许可

沿用 Chromium 的许可，未作改动：BSD-3-Clause，见 [LICENSE](LICENSE)。

[depot_tools]: https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up
[releases 页面]: https://github.com/sj817/shotium/releases
