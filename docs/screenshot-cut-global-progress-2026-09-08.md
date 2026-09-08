# 静态截图引擎全局裁剪进度（stage85）

stage85（Windows x64 编译与运行验收完成）：stage84 已提交 985fa6b678ff（2482 文件、515884 行删除）。本轮直接修复源码/生成模板，落实用户授权的 style_engine include、Gesture 重复 case 和 ConnectionAllowlist 旧统计删除；因 Route 留待下次，恢复 19 个最小 URLPattern 库文件及 SafeUrlPattern cc/h 和原默认开关。EXE 42,728,448 B、DLL 42,726,400 B，addon 已重建并核对加载 DLL 的 SHA。7041 构建输入全部存在；serve/net/node/daemon/protocol/Bilibili 均通过；demos 串行 84 项（62 PASS、1 FUZZY、21 SMOKE）；原基线 183/183 SHA 未变，新结果 183/183 解码像素完全一致。accept --skip-build 完成，Chrome oracle 差异维持 1.524%。首次 jobs=4 demos 出现等待异常，已终止旧测试；独立用例和整套串行复测通过，并行异常原因未确认。六平台实际编译/发布尚未执行，运行 tracing 与 UkmRecorder/SourceId 仍在。证据：out/cut-stage85-build/、out/cut-stage85-checks/；简版 npm 对比见 out/cut-stage85-perf/。

stage84：按用户最新范围完成集中源码删除，stage83已提交19938989d484。用户明确重新授权31/32删除清单、49图修复及84诊断具体差异；核对原文SHA后删除2039个Perfetto离线processor文件及16个ICU无用文件，直接移除六处UKM记录和core三条builder依赖/一条crash_key依赖/失效network:test_support。另删除354个Crashpad/components文件和8个UKM生成文件，收其余4条GN依赖、无用include与OWNERS。保留SVG实际处理、Zstd/CountUse、导航/查找状态、CHECK和FCP。当前实际Perfetto tracing后端与UkmRecorder/SourceId基础链仍在，不宣称所有诊断实现已经清空。Perfetto Python离线客户端配套由唯一子代理收尾；随后集中编译和运行验收，不扩展Blink交互及浏览器尾巴。构建通过前仍不能宣称可发版。证据out/cut-stage84-approved/、out/cut-stage84-crash-ukm/及out/cut-stage84-perfetto-python/。

stage83：stage82 已提交312c42b33a37。本轮21源码路径（4D+17E），删除无人消费的Performance-Observer网络头解析/策略schema/ParsedHeaders字段及Blink转换占位、独占capture-early-failures参数；删除PerformanceMarkOrMeasure协议、Entry/Mark/Measure虚函数转换及ResourceTiming无用include，共301行。保留实际计时数据、加载/FCP与安全头解析。Blink声明式上报host/timer/FlushPerformanceEntries整组删除已撤回：local_dom_window.cc:1008受限调用及window_performance.cc:1460仍在，需未来完整收口，不能宣称全部性能上报已移除。保护交集零、删除符号残留零、4个GN语法与diff及8文件保留正文对照通过；未图/编译/运行/像素验收。证据out/cut-stage83-combined/、out/cut-stage83-declarative/、out/cut-stage83-mark-mojo/。全局粗估65%±10、源码约80%，预计剩4–6个大源码批次及2–4个集中修复验收批次，非保证；受限Route/core/Worker/TrustToken/ReportingOptions/Blob/Perfetto/ICU/CSS诊断、UKM/Crashpad/tracing/PerformanceMonitor、Blink交互/AX/probes/lifecycle、根目录/第三方复核和最终六平台验证仍未完成。

stage82：stage81 已提交8e104f11f05c。本轮合并28源码/协议路径（7D+21E）。删除FetchLaterLoader协议及Child/Tracked工厂全部构造/克隆/转移/设置/创建入口、network/Blink请求is_fetch_later_api字段/转换/Mojo/恒false keepalive条件；保留document受限deferred-fetch策略工具、实际普通加载及keepalive。删除无人读写的LCPP导航hint/协议及OpenGraph仅import协议。Inspector issue payload全部无外部类型消费者，收为原生CSP转换仍引用的原值枚举，同时删除邮件验证结果协议/GN；安全策略正文未改。另删除帧可见性和VK overlay无订阅者观察链3文件及LocalFrame注册/分发/GC，保留FrameView真实读取的visibility状态、VK CSS env/几何归一化/rect。PerformanceMonitor受protected Document调用且仍有longtask订阅，本轮未动、明确待完整收口。保护交集零、移除运行符号残留零、GN语法/diff及原文/条件对照通过，尚无图/编译/运行/像素验收。证据out/cut-stage82-combined/、out/cut-stage82-protocols/、out/cut-stage82-performance-monitor/。全局约65%±10、源码约80%仍为粗估；待办仍为受限Route/core/Worker/TrustToken/ReportingOptions/Blob/Perfetto/ICU/CSS诊断、UKM/Crashpad/tracing/PerformanceMonitor、Blink交互/AX/probes/lifecycle、根目录与第三方骨架复核和最终Windows/六平台完整验证。


stage81：stage80 已提交 258f953c28a3。本轮合并34源码/协议路径（3D+31E）。网络15路径删除代理地址解析/字符串工具cc/h、无调用构造/getter/operator与URLRequest代理状态传递；ProxyChain/ProxyServer裁为旧HTTP磁盘缓存元数据的最小读写/校验，7段pickle/校验正文与3个HTTP响应/缓存文件不变，保留WasFetchedViaProxy对应旧缓存传输信息和NEL隐私判断，明确属于缓存格式而非代理运行链。另12路径解除TLS证书和HTTP挑战恒false的is_proxy字段/reset/生产端/比较/Mojo与认证重启guard，真实origin证书/凭据不变。Blink 7路径移除加载统计flag、UMA计时/后台支持枚举头/GN，保留全部真实clock conversion、完成时间clamp、FCP/ResourceTiming与client/字节回调，后台可用条件不变。保护交集零、完整移除符号残留零、GN语法/diff及pickle/加载正文对照通过；未图/编译/运行/像素验收。证据out/cut-stage81-combined/及proxy-cache/auth-origin/load-metrics三组。全局约65%±10、源码约80%仍为粗估；主要待办为受限Route/core/Worker/TrustToken/ReportingOptions/Blob/Perfetto/ICU/CSS诊断、UKM/Crashpad/tracing、Blink交互/AX/probes/lifecycle、根目录与第三方骨架复核、最终Windows和六平台完整验收。继续整组先删后集中编译。


stage80：stage79 已提交 d4eb3600e024。本轮合并44源码/协议路径（9D+35E）。整组删除无实际接收端的ResourceLoadInfoNotifier两层wrapper及Mojo接口、Frame/FetchContext/URLLoader/Shot/Sync/Background/Navigation/Sender参数和回调链、独占统计数据结构与RecordLoadHistograms cc/h、专属响应clone/storage和请求destination字段；保留真实ResourceType枚举、加载安全/完成/计时，Shot同步异步加载正文不变。由此删除URLResponseHead代理字段、network_param代理schema/traits/typemaps五路径，net磁盘缓存格式未改。另删除无任何生产者的BlobURLNullOriginMap及线程nonce缓存两文件、SecurityOrigin永不命中读取/friend/GN，保留原拒绝、nonce序列化及同源/访问策略。受限local_dom_window仍include的blob_url头尾巴明确待处理。移除符号源码残留零、保护交集零、GN语法/diff、Shot/加载正文与Blob origin对照通过；尚无图/编译/运行/像素验收。证据out/cut-stage80-combined/及load-notifier/proxy-mojo/blob-origin三组目录。全局约65%±10、源码约80%仍为粗估；剩余net缓存ProxyChain/ProxyServer最小化、受限Route/core/Worker/TrustToken/ReportingOptions/Blob/Perfetto/ICU/CSS诊断、UKM/Crashpad/tracing、Blink交互/AX/probes/lifecycle、根目录复核和最终Windows/六平台验收。继续整组先删、少量合并commit、最终集中编译。


stage79：stage78 已提交 cad5b9211547。本轮合并40源码/协议路径（3D+37E）。网络29路径整组移除SocketPool代理map/构造/getter/SSL代理刷新、代理专属限额与随机化开关、HTTP两套Job/控制器/请求/回调的ProxyInfo传递和存储，删除ProxyInfo cc/h及GN与无生产者日志事件。连接池改为惰性单池，原直连256/6、容量随机化、排队/取消/释放/真实TLS origin刷新保持；响应仍保留ProxyChain磁盘缓存兼容字段与auth重启值，未宣称该值类型全删。Blink 11路径移除无脚本消费者的Blob URL注册/撤销链、URLRegistry头、UUID生成、登记表和专属Mojo方法，实际Resolve/token、Blob读取和文件元数据路径保持；NullOriginMap安全读取及受限Blob/File生成链待后续处理。移除符号完整源码残留零、GN语法/diff及16段原文对照通过，未生成图/编译/运行/像素验收。证据out/cut-stage79-combined/、out/cut-stage79-socket-pool/、out/cut-stage79-proxy-info/、out/cut-stage79-fileapi/。全局约65%±10、源码约80%仍为工作量粗估；剩余响应Mojo/缓存ProxyChain与ProxyServer、受限Route/core/Worker/TrustToken/ReportingOptions/Perfetto/ICU/CSS诊断、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/lifecycle、根目录复核及Windows与六平台最终验证。按用户新要求整组先删、合并提交、最终集中编译，不再为每个小分支单独提交。


stage78：stage77 已提交 8a153a75af75。本轮16源码/协议路径（7D+9E）。删除Annotation/Credential两组未接线协议3文件与专属GN目标/源列表；删除ReportingObserver入站schema及无用frame import，原frame其余协议不变。同步删除ReportingObserver cc/h/IDL、创建key friend、frame GN条目、LocalFrame无用生成include；ReportingContext解除注册/反注册、observer回调队列、每类型100条缓冲、入站receiver/Notify及dictionary wrapper。保留实际CSP/权限/干预等报告安全路径，CountReport/GetReportingService/SendToReportingAPI三段原文不变，QueueReport仅删除observer通知。Options IDL与生成cc/h仍受bindings/core/v8/build.gni约束未删，明确未完成尾巴；其他残留为计数历史映射和注释，不能算运行API。完整引用/备份/保护交集零、3GN语法、diff及正文对照通过；无图/编译/运行/像素验证。证据out/cut-stage78-combined/、out/cut-stage78-reporting/、out/cut-stage78-protocols/。后续仍需SocketPool/响应Mojo/ProxyChain值、受限Worker/Route/core/TrustToken/ReportingOptions/Perfetto/ICU与CSS诊断映射、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录复核及六平台集中验收。全局约65%±10、源码约80%仍是粗估，继续整组先删后统一编译。

stage77：stage76 已提交 6631a3528f81。本轮31源码/资源路径（16D+15E）。网络13E解除SpdySessionKey代理维度全构造/存储/比较/日志/别名重建与证书变更代理跳判断，保留origin认证/IP pooling/网络隔离/DNS/证书/socket tag/target network；ConnectJobFactory到唯一ClientSocketPool调用的代理参数与仅单值AlpnMode/恒true重协商参数同步删除，ALPN/ALPS/HTTP1.1覆写及TLS/DNS转换原行为对照保持。另完整移除17个无C++消费者的表单popup生产资源ID与GRIT/GN条目，删除16 JS/CSS文件约9400行；color_picker.js原文件仍受core testonly copy引用，保留文件但不再生产打包，受限尾巴未宣称完成。实际CSS/表单布局/验证气泡及其余GRIT条目保持；残留文件名仅html.css历史注释。备份/保护交集零、源码/资源ID残留、GN/XML语法及diff与关键正文对照通过；资源数字ID可能在最终生成时变化，尚无图/编译/运行/像素验收。证据out/cut-stage77-combined/、out/cut-stage77-spdy-key/、out/cut-stage77-popup-resources/。下一组可独立处理Annotation/Credential无接线协议；ReportingObserver可先解除receiver/回调，但options生成链仍在受限bindings/core/v8/build.gni，不能宣称全删；具体清单见out/cut-stage77-next-groups/report.md。仍需完成SocketPool/响应Mojo/ProxyChain值、受限Worker/Route/core/TrustToken/Perfetto/ICU、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录复核及六平台集中验收。全局约65%±10、源码约80%仍为粗估，继续整组先删后统一编译。

stage76：stage75 已提交 d31589ea01ea。本轮20源码路径（1D+19E），删除SessionUsage头与整个构造/字段/getter/比较/转发/include/GN链：所有实际入口原值均为kDestination。TLS配置和缓存键额外删除固定Direct代理链及零索引，保留host/dest IP/网络隔离/隐私模式；Spdy其他key维度不变，Alt-Svc仅解除不可达代理guard，原origin正文对照一致。另收齐ProxyChain无调用辅助（GET判断、SplitLast/Prefix/First/Last、遍历/统计/工厂等）及失去唯一写入口的opaque附加状态，真实Pickle/Mojo字段和有效性规则保持。源码残留/备份/保护交集零、GN语法和diff检查通过，未图/编译/运行/像素验证。证据out/cut-stage76-combined/、out/cut-stage76-session-usage/、out/cut-stage76-proxy-chain/。后续仍需解除ProxyChain/ProxyServer响应与Mojo/缓存值、Spdy/SocketPool代理key，处理受限Worker/Route/core/TrustToken/Perfetto/ICU、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录复核与六平台集中验收。全局约65%±10、源码约80%仍为粗估；继续整组先删后统一编译，不按本轮文件数虚增进度。

stage75：stage74 已提交 4c07dd8c71aa。本轮16E，收齐server-only HttpAuthController入口，删除代理目标参数/存储、tunnel flag/专属挑战失败与代理统计分支，保留server bucket原值2/3及stride4；缓存/身份/ResetAuth/挑战方法等价对照通过。删除无生产端错误-111/-115/-366及transaction对应fallback，余错误name/value/order不变，实际407拒绝保留。另删除Job/Factory代理SPDY key与reuse/ALPN/IP-pooling分支、IsGetToProxy和UsingHttpProxyWithoutTunnel，原destination key/TLS参数保持；进一步完整删除is_for_get_to_http_proxy在Job/PoolGroup/BasicStream/BasicState/HttpUtil的参数/字段/getter链及绝对URL请求行分支，普通PathForRequest请求行原文保持，缓存真实使用SpecForRequest不动。备份/保护交集零、移除符号残留、原false分支/关键正文对照和diff通过；无GN/图/编译/运行/像素验收。证据out/cut-stage75-combined/、out/cut-stage75-auth-controller/、out/cut-stage75-stream-job/。仍余ProxyChain/Server/SessionUsage及响应Mojo/值类型、受限Worker/Route/core/TrustToken/Perfetto/ICU、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录复核与六平台集中验收；全局约65%±10、源码约80%仍为工作量粗估，继续整组先删后集中编译，不能按提交数宣称完成。

stage74：stage73 已提交 0be41260b4c6。本轮5E完成HTTP transaction代理认证闭包：删除两步代理token状态/生成方法、Proxy-Authorization注入、代理AuthURL/无隧道判断、已无入口的隧道响应Read分支；双目标认证数组改为单个server控制器，pending目标改为server布尔，保持401认证、407拒绝、认证重启/HTTP1.1协商、request headers/cache和TLS客户端证书错误重试。直连TransportInfo与TLS endpoint选择收齐，ProxyInfo无人使用AnyProxy/IPProtection包装删除。另删除控制器代理统计及WebSocket URL转换、HappyEyeballs恒真direct guard，实际AltSvc/DNS/TLS调度保留。6段请求体/发送/读取函数原文一致、server token仅等价存储/URL替换、控制器StartJobs/AltSvc/Ready/Failed/CreateJobs静态对照通过，备份SHA/保护交集零/移除符号残留与diff通过；无GN改动、图/编译/运行/像素验证。证据out/cut-stage74-combined/、out/cut-stage74-http-auth/、out/cut-stage74-controller/。剩余ProxyChain/ProxyServer/HTTP与socket代理分支及响应值、受限Worker/Route/core/TrustToken/Perfetto/ICU、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录复核及六平台集中验收仍未完成。全局约65%±10、源码约80%仍是粗估，不以提交数抬高完成度，继续整组先删除后集中编译。

stage73：stage72 已提交 58805e5647c4。本轮15源码路径（2D+13E），删除ProxyList整套候选列表实现，收齐ProxyInfo的PAC/命名代理/绕过/多候选/方案过滤入口；实际唯一UseDirect入口改为单个ProxyChain，保持初始空/选择后直连及复制语义。代理解析计时无写入者，删除内部存储/API，HTTP输出仍显式为空TimeTicks。删除代理URI/PAC解析函数组，保留现有诊断格式函数3段原文不变；CommonConnectJobParams与session仅代理消费的HTTP认证缓存/工厂/User-Agent参数链同步删除，实际HTTP认证与请求User-Agent仍保留，SPDY池/DNS/TLS参数保持。另删除17个无消费者旧代理/SOCKS/PAC/WebSocket错误定义，原负数保留空位注释，现存name/value/order全不变，4个仍有实际使用错误待后续解除。完整符号/文件引用检查、备份SHA、保护交集零、GN语法及diff通过；未图/编译/运行/像素验证。证据out/cut-stage73-combined/、out/cut-stage73-direct-state/、out/cut-stage73-proxy-errors/。按要求继续整组先删、集中最后编译。尚余ProxyChain/ProxyServer及HTTP代理分支、受限Worker/Route/core/TrustToken/Perfetto/ICU、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录最终复核及六平台验收；全局约65%±10、源码约80%仍为粗估，后续批次需按大组重排，不能用本轮提交数抵扣完成度。

stage72：stage71 已提交 883898f6d9bb。本批31源码路径（5D+26E），删除无实例/设置入口的ProxyDelegate、代理fallback实现和ProxyRetryInfo；解除context/session/connect参数链、stream专属通知与计时、重试表/降级API及唯一日志事件，GN与遗留include同步收齐。当前直连普通错误直接返回，旧fallback仅对已删除SOCKS错误做重映射；HTTP/TLS/HTTP2实际流程保留。另删除无人调用的SchemelessEndpoint、bool/HostPortPair工厂重载和disabled ALPN分支，实际HTTP ALPN/DNS/证书参数静态对照通过。备份SHA、保护交集零、删除符号/文件引用复核、1GN语法及diff通过；未图生成/编译/运行/像素验证。证据out/cut-stage72-combined/、out/cut-stage72-proxy-policy/、out/cut-stage72-schemeless-connect/。按用户最新要求，后续扩大整组源码删除、合并提交，中途仅快速残留/语法检查，最后统一编译及运行验收。剩余代理值类型/策略、受限Worker URL-list/preload/WasFetched及时序、Route/core/TrustToken、Perfetto/ICU、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录复核与六平台验收仍未完成。全局约65%±10、源码约80%仍为粗估；原10–16批估计不随小项提交递减，后续按更大的功能组重新合并。

stage71：stage70 已提交 29adfa0694f6。本轮21E，完整解除上层代理隧道认证通知/重启API、HTTP stream/request/controller与socket handle/pool/preconnect回调和代理注解参数；ProxyInfo无读者注解存储/唯一写点同步删除。连接池专用于代理认证的BoundRequest容器、绑定/查找/延迟错误/计数及生命周期全部收齐，普通队列、连接所有权、取消/预连接与回调保留；12段普通HTTP/auth/SSL/读写/池操作函数原文对照、普通completion分支/尾部对照通过。另删除59个无生产端NetLog事件及9个source标签，保留标签顺序验证通过，隐式数字编号会随最终重建重排；仍使用的proxy fallback/resolve日志与SPDY检查feature不动。完整移除符号残留零、原始SHA/保护交集零、diff通过，无GN改动/图/编译/运行/像素验证。证据out/cut-stage71-combined/、out/cut-stage71-proxy-callbacks/、out/cut-stage71-dead-network-events/。Proxy值类型/HTTP代理选择与fallback、Worker受限URL-list/preload/WasFetched及时序、Route/core/TrustToken尾巴、Perfetto/ICU、UKM/Crashpad/tracing、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录最终复核及六平台验收仍未完成。全局仍约65%±10、源码约80%、余10–16大批次仅粗估；继续先整组删除，集中最终编译。

stage70：stage69 已提交 77463762df23。本轮45源码路径（14D+31E）。删除HTTP CONNECT/基础proxy/SPDY proxy/SOCKS4/5握手及ConnectJob共14实现/头；解除递归代理参数、variant分支、factory实例/参数/代理DNS key、TLS隧道/SOCKS状态与ConnectJob代理认证通知及pool转发，GN/include和独占调度feature同步。直连参数入口CHECK既有直连约束，SSLSocketParams强化为transport参数；12段ALPN/DNS/TCP/TLS/ECH代码对照通过（仅重试选择唯一直接状态与去冗余类型DCHECK），HTTP2/证书/网络隔离保持。另11E完整解除Worker响应source/cache-name字段、转发、FetchResponseSource枚举与CacheStorage-only deliveryType，EarlyHints改用原已有HasMatchingServiceWorkerUrl；ResponseUrl/URL-list/preload/SSL及普通cache_state对照保持。删除文件路径引用零、保护交集零、备份SHA/14实体删除、1GN语法及diff/残留检查通过；代理类名仅旧日志/说明注释。证据out/cut-stage70-combined/、out/cut-stage70-proxy-transport/、out/cut-stage70-worker-response/。无引擎图/编译/运行/像素验收。上层pool/HTTP代理认证callback和注解存储、代理值类型、Worker URL-list/preload/WasFetched及时序仍待收齐；其余受限Route/core、Perfetto/ICU、UKM/Crashpad/追踪、Blink交互/AX/fileapi/probes/observer/lifecycle、根目录复核及最终六平台验收未完成。全局仍约65%±10、源码约80%、余10–16大批次仅粗估，继续先整组删除后集中编译。

stage69：stage68 已提交 05d4be102cb6。本轮38源码路径（15D+23E），合并代理配置与ServiceWorker router响应两闭包。HTTP已固定直连，解除为读取默认注解而构造ProxyConfig的唯一真实入口，原注解逐字保留到调用处；删除PAC/WPAD/system/dynamic-routing配置数据、专属Mojo/traits与partial annotation共12文件，普通URLLoaderFactory流量注解单独保留最小目标，HTTP认证实际使用ProxyHostMatchingRules不动。另删除ServiceWorkerRouterInfo schema/Blink包装3文件，解除网络响应、WebURLResponse/ResourceResponse、ResourceTiming/性能IDL、fetchStart专属分支和7个路由统计字段；普通Create含SSL/证书/HTTP与fetchStart分支对照一致，其他worker响应元数据仍单列待办。5GN语法、备份SHA/15实体删除/保护交集零、diff及完整引用复核通过；残留是既有注解ID、Rust独立Windows绑定、UKM两条历史enum文字。证据out/cut-stage69-combined/、out/cut-stage69-proxy-config/、out/cut-stage69-worker-response/。未图生成/引擎编译/运行/像素验收，按用户要求集中最终编译。全局仍约65%±10、源码约80%、余10–16大批次是粗估；受限Route/core/TrustToken生成尾巴、Perfetto/ICU、UKM/Crashpad与追踪、代理transport/worker响应及递归prefetch、Blink编辑/AX/fileapi/probes/observer/lifecycle、最终根目录复核与全平台验收未完成。

stage68：stage67 已提交 e965a4d97ef5。本轮扩大为111源码/工具路径（91D+20E），两组合并提交：删除78个metrics上游维护/校验/测试脚本及Siso旧入口，保留实际内存白名单与UKM生成链；两生成器删除前后实际运行成功，5生成文件逐字节一致。删除9个无消费Web嵌入接口头及4专属实现，GN/exported/probe生成include与无用声明同步；真实FCP、打印布局、字体/GC与受限prefetch消费者保留。2GN、2JSON5及结构比较、备份SHA/保护交集零、91实体删除和diff/引用检查通过。证据out/cut-stage68-combined/、out/cut-stage68-metrics-tools/、out/cut-stage68-web-interfaces/。未引擎图生成/编译/运行/像素验收；按照用户要求继续整组先删、集中最后编译。全局仍约65%±10、源码约80%，余10–16大批次仅粗估，不按本轮文件数抬高进度。待办仍包括受限Route/core/TrustToken生成尾巴、Perfetto/ICU、UKM/Crashpad与追踪、网络策略/递归prefetch、Blink编辑/AX/fileapi/probes/observer/lifecycle、根目录最终复核以及统一编译/运行/像素/六平台验收。

stage67：stage66 已提交 58c5d81e0f96。本批15源码/工具路径（11D+4E）。解除Trust Token两个permissions-policy注册及命名枚举、六个NetLog事件、两个无生产端错误定义；权限72/110及错误-506/-507留注释保留空位，其他显式编号原文不变。使用仓库pyjson5解析确认其余权限配置完全相同；其余日志事件顺序保持，隐式NetLog数字ID可随最终重建变化，不宣称稳定。另删除Clang十一独立维护脚本：批量重构/编辑与测试链、额外clang-tidy工具构建、编译输入大小报表、上游dashboard；完整显式/动态入口复核无实际Shot或六平台消费者，build/update/package、Rust/Crubit、generate_compdb和资源字体生成保持。保护交集零、原始SHA/11实体删除、JSON5/枚举/残留与diff检查通过，无GN修改或图/编译/运行/像素验收。证据out/cut-stage67-combined/、out/cut-stage67-token-declarations/、out/cut-stage67-unused-tools/。Trust Token仅余受限Document生成头/schema与PrivateToken IDL/生成列表尾巴；历史WebFeature编号是兼容元数据而非启用功能。stage49受限core GN阻点及全局后续未解除，全局约65%±10、源码约80%、余10–16大批次仍为粗估。

stage66：stage65 已提交 de95404538ca。本批36源码路径（20D+16E）。解除Trust Token completion/error链：网络状态字段/schema/traits、WebURLError专属构造/getter、ResourceError拷贝/转换/比较/日志/特殊抑制及无调用key-commitments switch，五处空include收齐。普通CORS/响应阻断/DNS/证书/缓存错误继续保留；完整生成头引用只余受限document.cc和归属GN源条目，schema枚举原文不变且明确待删，未伪报完整功能完成。另删除macOS八组独立辅助及专属LaunchServices SPI/孤立test-helper BUILD、POSIX旧UnixDomainSocket共20文件，base GN同步；真实Mach签名/FD与Mojo通道/net socket/process launch/locale/font/error逻辑保留，残留同名Mojo helper为独立实现。保护交集零、原始SHA/实体删除、1GN语法、diff和残留核对通过；证据out/cut-stage66-combined/、out/cut-stage66-token-status/、out/cut-stage66-system-helpers/。无图生成/编译/运行/像素验证。Trust Token剩余受限schema/IDL生成边与policy/counter/历史event声明、stage49受限core GN阻点和全局后续仍待完成；全局约65%±10、源码约80%、余10–16大批次仅工作量粗估。

stage65：stage64 已提交 8f95ad10444e。本批51源码路径（26D+25E）。删除Trust Token请求闭包：iframe专用属性解析/state/IDL、frame-owner参数构造、脚本converter、Blink/WebURLRequest转换与网络请求/traits/Optional包装，FetchAPIRequest与导航字段同步；8专属实现文件移除、4GN列表收齐。schema只保留当前completion/error链使用的TrustTokenOperationStatus且枚举原文相同；安全/重定向尾部实现未变。整个Trust Token仍未完成：error/status、switch/policy/counter、受限bindings列表关联IDL和document.cc旧include单列待办。另删除9组无调用Windows辅助18文件（IAT patch、独立PE reader、Win11/TPM资格、Explorer/elevation、limited features、EnumVariant、ETW controller、CRT setup），base GN与tbs链接/延迟加载、唯一消费者消失的kDisableBreakpad同步；实际PEImage/COM ModifyCode/ETW provider/资源主题字体GDI保留。保护交集零、SHA/实体删除、5GN语法、diff与残留检查通过；残留明确区分NetLog历史注释、Rust独立TPM绑定及未完成状态链。证据out/cut-stage65-combined/、out/cut-stage65-trust-token/、out/cut-stage65-system-helpers/。无图生成/编译/运行/像素验收，stage49受限core GN阻点仍在，全局估计65%±10、源码约80%，余10–16大批次为粗估。

stage64：stage63 已提交 36aed49570a3。本批30源码路径（5D+25E），整链解除网络请求FetchRetryOptions专属类/schema/traits/GN及限速token、Fetch window ID、keepalive统计token、重试选项四组元数据；清理Blink存储/getset、WebURLRequest转发、网络转换与序列化，真实keepalive bool和网络连接重试保留，ResourceRequest从SendsCookies起的安全/重定向实现原文不变。另解除11个Core/ModulesInitializer空服务API及调用端，删除独占session_storage_namespace/storage_area两份协议；LocalFrame冻结/卸载仅去空缓存驱逐调用，CreateNewWindow仅去空storage clone。保留实际初始化/InitLocalFrame注册、PiP有返回值fallback；InstallSupplements调用端受限仍单列。保护交集零、备份SHA、5实体删除、4GN语法、diff和移除符号完整残留检查通过。证据out/cut-stage64-combined/、out/cut-stage64-network-fields/、out/cut-stage64-module-shells/。未图生成/编译/运行/像素验证，stage49受限core GN阻点及全局剩余未解除；全局工作量仍估计65%±10，源码约80%，余10–16大批次为粗估。

stage63：stage62 已提交 2a47ed80f433。本批36源码路径（17D+19E）。整组移除无调用HandleHooks、chrome崩溃/调试URL及其专属Win heap/CFG故障注入、ASan故障注入和四文件Rust人工崩溃夹具，同步GN并清理两空夹具目录；删除无实例网络变化日志观察器、只有写入没有读取的全局网络字节计数器及五个TCP/UDP计数调用点。五份socket文件仅计数调用/include变化，返回值、回调、错误和NetLog逻辑保持。并行解除DevTools四个Mojo接口及全部独占数据、空绑定API、Core/ModulesInitializer两空session初始化API和无用friend。GDI真实分配失败诊断、实际network notifier、CHECK/错误日志、CaptureStats/FCP与线程任务处理保留。保护交集零、删除存在性、备份SHA、6GN语法、diff和完整残留检查通过。证据out/cut-stage63-combined/、out/cut-stage63-debug-network/、out/cut-stage63-devtools-protocol/。未图生成/编译/运行/像素验收；stage49受限core GN阻点及全局后续闭包仍待处理，不能以静态通过代替完成。全局工作量估计仍约65%±10，源码约80%，余10–16大批次是粗估。

stage62：stage61 已提交 4e9aaba36663。本批37源码路径（21D+16E），合并遥测外围与四组浏览器协议：删除histogram增量序列化/快照、跨进程共享内存启动传输、落盘storage、single-sample工厂、旧debug profiler及runtime field-trial overrides；解除专属PrepareDeltas、无用include和无写入的override读取分支/参数。常规histogram/persistent allocator、注册trial枚举和锁、CaptureStats/FCP及错误日志未改。另完整解除无绑定FindInPage及专属AX结果通知、DraggableRegion和WindowFeatures Mojo数据、DeviceEmulationParams原生类型/schema/traits；真实文本查找/CSS拖动/窗口属性/Shot视口保持，DisplayCutout实际接收器保留待整体裁决。备份SHA、21实体删除、保护交集零、5GN语法、diff和精确残留检索通过；Rust Windows JsStartProfiling等同名子串为无关系统绑定。证据out/cut-stage62-combined/、out/cut-stage62-base-telemetry/、out/cut-stage62-blink-protocol/。未图生成/编译/运行/像素验证，stage49受限core GN阻点仍在。全局仍约65%±10，源码约80%，余10–16大批次为工作量粗估，最终验收尚未完成。

stage61：stage60 已提交 bbe067433a13。本批20源码路径（13D+7E），整组删除无调用的网络目录浏览器：DirectoryLister、JS目录HTML生成器及资源包、NetModule资源提供接口、BackoffEntrySerializer，并解除GN/资源ID入口。正常file加载、缓存及退避算法保留。并行整组删除PageBroadcast接口及独占PageRestoreParams、颜色映射Mojo序列化/traits；真实页面生命周期、预渲染参数及原生SVG颜色继承保留。4GN语法、diff及残留检查通过，保护清单交集零。没有本轮图生成/编译/运行/像素验收，stage49受限core GN阻点未解除。证据out/cut-stage61-combined/、out/cut-stage61-net-directory/、out/cut-stage61-page-protocol/。按用户最新要求，后续扩大删除组、集中最终编译；全局工作量仍估计65%±10，源码约80%，余10–16大批次是粗估而非小提交计数。

stage60 当前接续：stage59 已提交 06e6b2420c9c。本批22源码路径、14D+8E。删除无调用SystemMonitor设备通知、Linux桌面环境导出及专属GPU信息formatter、全部无消费者UI GLib wrappers（12D3E）；底层GLib、SysInfo、字体配置及实际display util/EDID保留，ui/base/glib与linux两空目录按边界与为空校验后清理。另仅解除WebPreferences无人消费的序列化/traits/Mojo struct/page更新方法（2D5E），Shot真实默认配置类/构造/应用逻辑继续保留；8个CSS/布局等枚举原文相同。类体只少一多余空行，声明/默认值不变，头文件显式包含V8CacheOptions shared枚举以消除旧传递依赖。22路径备份SHA/保护交集零、14实体删除、5GN语法、diff/残留核对通过；SystemMonitor同名Rust COM GUID非base调用，保留。证据out/cut-stage60-combined/、out/cut-stage60-system-ui/、out/cut-stage60-web-prefs/。无本轮图生成/编译/运行/像素验证，stage49受限core GN阻点及全局待办未解除，整体工作量估算仍约65%±10。

stage59 当前接续：stage58 已提交 33e64ee8cc7a。本批42源码路径、32D+10E。删除无实例/外部调用的平台PowerMonitorDeviceSource、电池provider/sampler、CPU频率估算、IOPM/温度/速度limit等26文件及GN生产/失效测试入口；网络/TCP/HTTP/定时器/线程仍引用的PowerMonitor/Source/Observer五文件原文保持，不把该基础层算作已删。另整链解除RendererPreferences类/构造/traits/schema/watcher、Page无消费者存储与get/set及page协议方法、专用WebRtcIpHandlingPolicy（6D9E）。原renderer_preferences.h只保留真实主题使用的四个默认选区颜色常量且数值原文不变，字体/layout theme实际行为代码与Shot runtime未改。42路径备份SHA、保护交集零、32实体删除、4GN语法与diff/残留检查通过；证据out/cut-stage59-combined/、out/cut-stage59-power-sources/、out/cut-stage59-renderer-prefs/。无本轮图生成/编译/运行/像素验证，stage49受限core GN阻点及原始全局剩余仍待完成；整体工作量估算约65%±10。

stage58 当前接续：stage57 已提交 ac73da72718a。本批20源码路径、2D+18E：删除无创建端WebURLRequestExtraData类/实现、WebURLRequest接口、请求存储/复制、GN与无实体测试项，连带originated_from_service_worker协议字段退出；跨站图片认证使用原null-extra分支，混合内容/嵌入身份限制及普通prefetch/cache保持。动态屏幕管理四操作、Delegate/单例注册、Mac覆盖方法和专属几何辅助整链移除；GetNewDisplayId、GetHeadlessScreenInfos、CreateDisplayList、GetDisplayNearestWindow、SetDisplayGeometry函数体保持。实际Shot worker/daemon/UA/资源加载、初始屏幕配置/DPI/字体不变。20路径SHA备份、保护交集零、3GN语法、diff/残留和保留函数核对通过，证据out/cut-stage58-combined/、out/cut-stage58-request-extra/、out/cut-stage58-headless-manager/。未编译/运行/像素验收，受限core GN图阻点仍在，整体估计约65%±10、原始全量任务未完成。

stage57 当前接续：stage56 已提交 a6efbc4d2408。本批21源码路径、3D+18E，源码4行增加/318行删除。整链移除浏览器Cookie注入/回传参数及响应cookie echo协议/traits（真实net Cookie解析保留）、previews_state和无调用prefetch_token、WebURLRequestExtraData页面跳转分类、FrameFetchContext到network的共享字典write-only标记。普通Cookie/重定向/referrer/加载安全尾部原文不变，实际共享字典解码保留。另删无调用CDP屏幕UpdateDisplay工具和HeadlessScreenInfo调试格式化/比较；七个几何/配置解析函数体与Mac屏幕实现保持，RE2实际配置解析保留。21路径备份SHA/保护交集零、2GN语法、diff及引用核对通过；同名Perfetto table row setter不是本批调用，未改。证据out/cut-stage57-combined/、out/cut-stage57-network-state/、out/cut-stage57-display-audit/。按整批修改后集中编译策略，本批未编译/运行/像素验证；stage49受限core GN阻点未解除，全局目标继续，整体仍估计65%±10。

stage56 当前接续：stage55 已提交 28102dfddc65。本批11E、源码4行增加/640行删除：整组解除无生产/消费端的六类network请求负载（浏览器EnabledClientHints缓存、合成响应producer/expected headers、DevTools解码白名单、两组NetLog参数），同步清掉专属NetLogSource Mojo/traits/typemap和无人实例化ScopedResourceRequestCrashKeys。实际net NetLog、反序列化失败诊断、Blink ClientHints和资源加载保留；SendsCookies至重定向/referrer/allowed-load-flags实现原文保持。媒体fragment去无调用时间/轨道/NPT API与RE2依赖，六个空间解析函数体和SVG视图代码不变。备份SHA、全部受限清单交集零、GN语法、diff与目标符号残留核对通过；证据out/cut-stage56-combined/、out/cut-stage56-network-fields/、out/cut-stage56-media-fragment/。未编译/运行/像素验收，stage49受限core GN阻点仍在；全局任务继续，整体工作量估计仍约65%±10，不把静态检查当最终验收。

stage55 当前接续：stage54 已提交 c4fa55014cfd。本批整组清理 UI 浏览器本地化工具（语言选择列表、文案/复数格式化、Cocoa菜单标签包装、窗口尺寸辅助），移除两份 l10n_font_util 文件；ResourceBundle locale解析、GetStringUTF16/RTL调整和Cocoa locale初始化原文保持，Windows字体代码未动。同步清理ipcz已无实体的测试/reference/fuzzer GN目标及Mojo测试依赖，真实Mojo/ipcz运行后端保留。同时移除libyuv完整7路径闭包：gitlink、unbundle shim、3条Blink GN边及DEPS/.gitmodules/unbundle同步入口；原独立仓库完整保存于out/cut-stage55-libyuv/vendor-backup/libyuv，不是活动源码。合计20源码路径、3普通文件删除+1gitlink删除+16E；7GN语法、Python AST、gitmodules和diff静态检查通过，清单与证据out/cut-stage55-l10n/、out/cut-stage55-ipcz-build/，第三方具体用途复核out/cut-stage55-library-audit/。按用户要求整组实施后集中验证，当前只完成静态检查，未编译或运行验收；受限core GN阻点未解除。全局仍约65%±10，完整待办和10–16大批次估计见全局报告。

stage54 当前接续：stage53 已提交 f8e410a354a1。本批252源码路径，164D+88E：删除无实际消费者的 auto_image_annotation_strings 与 ui_strings 两套 GRD/162翻译文件，解除GN、Shot无用语言pak输入、ID分配与12项startup排序记录；保留实际Blink和locale资源。AX消息242→14，81语言译文19440→1134，保留日期/时间字段实际引用的符号、原消息和原译文，官方Grit消息指纹核对一致。数字资源ID可重新分配，最终必须一起重新生成header/pak和编译消费者，不宣称数字ID不变。备份SHA、精确252路径、全部受限清单交集零、删除存在性、2GN语法、XML/指纹/引用与diff检查通过。旧IPC五Native类型复核确认源码迁移已完成，当前ipc跟踪文件及mojom Native声明为零，普通Mojo安全traits保留；该项移出实现待办。证据out/cut-stage54-combined/verification.json、out/cut-stage54-ui-strings/report.md、out/cut-stage54-ax-strings/report.md、out/cut-stage54-ipc-audit/report.md。没有当前图生成/编译/运行/像素验收，stage49受限core GN阻点未解除。整体仍估计65%±10、源码约80%、预计余10–16大批次，完整待办见全局报告。

stage53 更新：ui/resources 全部 90 文件、12 空目录已清理，配套打包/ID/路径依赖完成；DevTools request-id 和 emitted-extra-info 通知链同时解除。本批 128 源码路径，SSL/导航/混合内容/重定向实际行为静态核对保持。完整编译和运行验证仍待完成。证据 out/cut-stage53-combined/。

stage52 更新：36 源码路径、3 文件删除，完成 NetLog 导出/专用摘要、DevTools stack id 和广告拍卖请求标记裁剪。普通 CORS 分支等价，Flow helper 不变；实际错误诊断及 base/Perfetto 运行 tracing 尚在。证据 out/cut-stage52-combined/；当前累计源码仍未验收，不机械提高整体估算。

stage51 更新：WebBundle 请求/响应/Fetcher 闭包已完成源码处理，31 路径、4 文件删除，静态检查通过，尚未编译。下表第 5 组剩余网络项继续推进；整体约 65% 和 10–16 大批次仍为估算范围，不因一次源码提交机械上调。证据 out/cut-stage51-combined/。

上一轮已完成上层 BidirectionalStream 和 AcceptCHFrameObserver 闭包：27 个源码路径、9 个文件删除、18 个配套编辑，源码增加 33 行、删除 1781 行。普通 HTTP 和底层 HTTP/2 CONNECT 保留。备份、引用、2 份 GN 语法及 diff 检查通过；本轮没有执行完整图生成、编译或运行验收。

## 当前进度与口径

- **本次收尾交付：Windows x64 构建、运行与像素验收已完成。** 上述通过结果覆盖累计裁剪后的当前源码；首次并行 demos 等待异常未定因，串行整套通过。简版性能结果见下方。
- **全局源码裁剪约 85%（工作量粗估，误差 ±10 个百分点）**，全局交付约 80%（同为粗估）。不是按文件数统计，不能视为六平台完成率。
- 本次约定的离线第三方/工具/测试及 Crashpad、UKM builder 清理已经落地。运行 tracing、UkmRecorder/SourceId 和深层性能订阅仍在，不能宣称全部诊断清空。
- 用户指定剩余两大方向留待下次：Blink 深层交互/扩展，以及浏览器网络/路由尾巴。本轮不继续扩大删除范围。

## 已完成

| 范围 | 当前结果 |
|---|---|
| 大组件与图形 | gpu/sql/sandbox/device/google_apis 等跟踪源码清空；CPU paint、字体、SVG、MathML 保留。Canvas 绘图链已去除，标签静态 fallback/尺寸相关仍在 |
| 第三方/工具/测试 | Perfetto 离线 processor 2039 文件、ICU 16 文件删除；Perfetto Python 离线客户端/配套 49 文件删除；第三方直接维护源码，无需补丁队列 |
| UKM/Crashpad | 6 处核心 UKM builder 统计、builder 生成链移除；Crashpad/components crash 354 文件和 UKM 工具 8 文件删除 |
| GN/编译 | 失效 network:test_support 已移除；生成模板、直接 include、Mojo traits 和 jumbo 冲突已修；7041 输入存在；EXE/DLL/addon 构建完成 |
| 运行 | serve、net、node、daemon、daemon-protocol、Bilibili 通过；demos 串行 62 PASS + 1 FUZZY + 21 SMOKE |
| 像素 | 183 原始基线哈希不变；新输出 183/183 像素完全一致；Chrome oracle 1.524% 差异与旧验收一致 |

## 全局待办（下次再做）

| 范围 | 尚未完成 | 粗估批次 |
|---|---|---|
| Blink 交互/扩展 | editing、拖放、AX、fileapi/blob、observer/probe/lifecycle 等实际调用闭包；不能按名称整目录删除 | 1–2 |
| 浏览器网络/路由尾巴 | Route/CSS/URLPattern、Worker、TrustToken/ReportingOptions 和剩余 policy/协议；Route 本轮恢复了最小原生依赖用于通过编译 | 1–2 |
| 诊断耦合余项 | base/trace_event、运行 Perfetto、UkmRecorder/SourceId、PerformanceMonitor/声明式性能订阅；保留 CaptureStats/FCP/CHECK 的实际功能 | 1–2 |
| 全局复核与平台验证 | 根目录/DEPS/生成工具最终复核、六平台真实构建和必要修复；本次 Windows 本机通过不代替其他平台 | 1–2 |

预计 **4–8 个大批次**，可按同一依赖闭包合并提交；这是未知耦合较多的估计，不是承诺。发版准备可先进行其他平台构建，无需先完成上述全部可选裁剪。

## 现有边界与异常

此前对 Perfetto 2039 删除、ICU 16 删除、core 失效 GN 行、核心 UKM 以及三处编译修复的具体授权均已落实，不再列为当前阻点。其余旧受限修改没有获整组授权，后续应重新审查当前源码，不能重放历史补丁。

首次 jobs=4 demos 在 gradient-hard-stop 测试等待；输出存在不等于进程完成。独立同一用例及 jobs=1 全部 84 项通过；旧测试父进程已终止，首次并行运行记为失败/未完成，不计通过。原因未确认，后续并行可靠性值得单独排查。

## shotium 迁入 apps 的成本

**中低成本，建议独立一个提交处理，本轮仅评估未移动。** 已发现 58 个候选引用文件（6 workflows、22 scripts、21 apps、8 shotium、root package.json），其中可能含注释，不等于必须改 58 个文件。主要是 native 的头文件/库相对路径、构建与测试脚本默认路径、workflow 工作目录/产物和示例引用；不需要改 Blink/Skia 引擎架构。预计一个路径迁移批次，加包构建/类型/加载/打包检查；若引擎 GN 输入不变无需重编完整 Chromium。

## 证据

- stage84 删除提交：985fa6b678ff；out/cut-stage84-approved/、out/cut-stage84-crash-ukm/、out/cut-stage84-perfetto-python/。
- stage85 构建：out/cut-stage85-build/build-04.log、dll.log、addon.log、missing-inputs-final.log、binaries.json。
- 运行：out/cut-stage85-build/ 下各套日志；demos-serial.log 与首次 demos.log 分开保存。
- 像素：out/cut-stage85-checks/pixel-comparison.json；原始基线完整性：out/cut-stage85-build/baseline-integrity.json。
- 后续接续以本节与顶部 stage85 为准；上面的 stage84 及更早记录为历史，不应重新当作当前未完成清单。

## npm 0.4.0 简版对比（Windows x64）

同机、同一 JS bundle，真实 npm 安装包对比当前重建 addon/DLL；实际加载库 SHA 已记录并与最终产物一致。仅选 6 个本地用例，AB/BA 配对、至少 20 对、99% 区间；未做 A/A 噪声校准，使用脚本默认 2% 容差。不涉及远程网络速度，也不代表其他平台。

| 项目 | npm 0.4.0 | 当前 | 变化 |
|---|---:|---:|---:|
| 引擎 DLL（未压缩） | 48,976,896 B / 46.71 MiB | 42,726,400 B / 40.75 MiB | -12.76% |
| Node addon（未压缩） | 122,880 B | 124,416 B | +1,536 B |

当前 EXE 为 42,728,448 B（40.75 MiB），npm 包不含 EXE，故不进行跨形态比较。当前平台运行文件 DLL + addon + 两个 pak 合计 42,889,104 B；这不是重新打包后的 npm tarball 大小。

| 用例（p50，ms） | npm | 当前 | 耗时变化 | 当前/npm 99% 区间 | 配对数 | 判断 |
|---|---:|---:|---:|---|---:|---|
| card-png | 5.125 | 4.906 | -4.27% | 0.938–0.979 | 1000 | 小幅更快 |
| corpus-png | 13.508 | 12.895 | -4.54% | 0.924–0.992 | 571 | 小幅更快 |
| card-jpeg | 3.144 | 2.986 | -5.03% | 0.931–0.972 | 971 | 小幅更快 |
| card-webp | 14.702 | 14.505 | -1.34% | 0.972–1.000 | 201 | 小幅更快 |
| standard-long-page-png | 33.764 | 32.956 | -2.39% | 0.932–1.030 | 232 | 收益未证实 |
| startup-png | 11.491 | 10.963 | -4.59% | 0.935–0.975 | 396 | 小幅更快 |
| 引擎初始化 | 29.028 | 27.455 | -5.42% | 0.908–0.975 | 396 | 小幅更快 |
| 模块导入 + 引擎初始化 | 38.242 | 36.403 | -4.81% | 0.908–0.996 | 396 | 小幅更快 |

启动用例的 wall 是新进程内第一次截图；引擎初始化与模块导入另列，不含 Node 进程启动。p95 尾延迟存在不确定性，不宣称全分位提速。6/6 配对图片校验通过，各截图失败资源计数为 0。

这是按用户要求缩小范围的诊断对比，**不是完整性能门通过**：result.json 明确 complete=false/status=not-passed；完整报告器因此返回 exit 1，另有长页面 unproven。所选 6 项执行完成，没有把失败截图当速度收益。原始结果：out/cut-stage85-perf/result.json，图片结论 result.pixels.json，完整报告器输出 report.md。
