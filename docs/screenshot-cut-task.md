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

stage53 当前接续：stage52 已提交 6775069775ac。本批128源码路径，实际删除ui/resources全部90跟踪文件并精确清理12空目录；关闭/禁用/favicon/文件夹/信号/拖放图标和Windows光标的48个ID无源码消费者，Shot打包、UI生产/测试GN、GRD ID分配、startup排序和无调用UI_TEST_PAK路径枚举/lookup同步清理。保留Blink样式/图片pak；shot_strings及之后runtime/GN原文不变。另27E解除devtools_request_id完整请求/序列化/probe/redirect/trace及mixed-content纯透传参数，连emitted_extra_info纯通知字段清理。导航全文/SSL处理原文保持、WebURLResponse::Create只去通知其余cert_status/SSL逻辑不变、mixed-content函数体不变、redirect仅去ID赋值；普通5次response创建保留原false分支而导航安全参数仍在。128路径SHA备份、保护交集零、90删除存在性、6GN语法、diff、资源/ID/构建引用零通过。证据out/cut-stage53-combined/verification.json、out/cut-stage53-ui-resources/report.md、out/cut-stage53-devtools-id/report.md。未图生成/编译/运行/像素验收，stage49 core GN受限阻点及其余全量待办未解除；ui根内真实几何/字体/DPI/native表单基础仍在，不能称整个ui已删。

stage52 当前接续：stage51 已提交 bba79582dbc5。本批36源码路径、3文件实际删除。NetLog移除无实例TraceNetLogObserver、无调用GetNetConstants/GetNetInfo/active-object导出及net_info常量表，继续解除唯一为导出服务的连接池/SPDY session/stream/AltSvc/HTTP stream pool摘要和两专属状态字符串函数；NetLogWithSourceToFlow原文保持，实际consistency/error诊断使用的Group/AttemptManager/stream-attempt摘要及连接/流控/TLS/缓存保持。另解除devtools_stack_id完整复制/序列化链及无true生产者的广告拍卖trusted-signals字段/CORS专属参数，cors.cc按原普通false分支忽略注释空白整文件等价。36路径备份SHA、保护交集零、删除存在性、GN语法、diff和移除API残留检查通过；无图生成/编译/运行/像素验证，stage49 core GN受阻项未变。devtools_request_id仍有redirect/response/security/probe实际传递须另审；recursive_prefetch_token闭包触及受保护DocumentLoader两处，未动。证据out/cut-stage52-combined/verification.json、out/cut-stage52-netlog/report.md、out/cut-stage52-network-fields/report.md。全量目标未完成。

stage51 当前接续：stage50 已提交 bdd5e583aae6。本批31源码路径、4文件实际删除：WebBundleHandle协议与Blink SubresourceWebBundle/list三文件；27配套编辑解除network/Blink请求token/clone/traits/schema、Fetcher列表/匹配/取消、WebURLRequest getters、request conversion及响应inner-response标记。RequestDestination专用值/映射/Accept header/traffic annotation case同步去除，数值19明确退休，其余枚举不重编号。CSP/CanRequest helper仅去bundle替代URL参数并选择原params.Url分支，整文件忽略注释空白等价核对；普通HTTP timing、MHTML/SVG/MemoryCache以及TLS/资源加载保持。31路径备份SHA、保护交集零、4删除存在性、2GN语法、diff和已删实际符号全tracked零残留通过；历史use_counter/RequestContext标签和受保护Fledge runtime条目不是这条已删执行链，未按词强删。证据out/cut-stage51-combined/verification.json及out/cut-stage51-webbundle-blink/report.md。无本轮图生成/编译/运行/像素验证；stage49受保护core GN图阻点未变，全量目标继续active。

全局进度与待办（stage50）：见 docs/screenshot-cut-global-progress-2026-09-08.md。工作量估计：源码约80%，整体约65%（约±10个百分点），预计剩余10–16个大批次；当前累计源码尚未最终验收。下一独立WebBundle闭包只读清单为out/cut-stage50-webbundle-audit/，4D+18E，未实施。

stage50 当前接续：stage49 已提交 39fae314f124。本批27源码路径，9文件实际删除：net上层BidirectionalStream API/实现/SPDY适配器8文件及network AcceptCHFrameObserver协议1文件。18配套编辑解除上层stream type、factory/job/controller分流、ready回调、请求构造参数及AcceptCH请求字段/Clone/traits/schema/include/GN；Client Hints枚举改为直接include。普通HTTP成功分支及SpdyHttpStream构造按去注释空白比较等价，底层SPDY_BIDIRECTIONAL_STREAM、HttpProxyConnectJob和实际HTTP2 CONNECT未改，TLS/缓存/AltSvc限制保持。27路径备份SHA、删除存在性、保护交集零、2份GN语法、diff及目标符号残留检查通过；仅历史netlog注释残留。证据out/cut-stage50-combined/verification.json、out/cut-stage50-bidirectional/report.md。当前完整图仍受stage49 core/BUILD.gn失效//services/network:test_support依赖阻挡，此前拒绝路径未修改；本批没有重复图生成、C++编译、运行或像素验收，全量任务未完成。

stage49 当前接续：stage48 已提交 22a791e339fa。本轮集中使用pnpm build:engine --gen-only检查累计构建图，没有完整编译。依次清理mojo/public孤立fuzzers目标（指向已删tools/fuzzers/BUILD.gn）和BoringSSL仅测试尾段（test模板不存在导致Unknown function，生产加密/TLS/Rust目标前缀原文不变）。第三次图生成停在受保护third_party/blink/renderer/core/BUILD.gn:1283的//services/network:test_support，services/network/BUILD.gn已不存在；该路径命中此前Route拒绝清单，本轮未修改，不能用补空目标或其他文件绕过。2源码GN语法/diff通过，但当前完整图仍失败，missing-inputs/C++/运行/像素门尚未执行。证据out/cut-stage49-graph/gen.log、gen-02.log、gen-03.log、verification.json和approval-review.md。下一组只读审计out/cut-stage49-bidirectional-audit/已给8D+11E，尚未实施；保留实际HTTP2 CONNECT使用的底层SPDY_BIDIRECTIONAL_STREAM。全量目标未完成，仍有独立可推进工作，不标记整体blocked。

stage48 当前接续：stage47 已提交 00bae83a0025。本批89源码路径、22文件实际删除，源码155行增加/4526行删除。network公共层解除DevTools、NetworkService、CookieAccess、TrustTokenAccess、SharedDictionaryAccess五条无接收端观察者链（请求字段/复制/traits/schema），删6协议、纯DevTools错误上报与2个未用日志字符串helper，保留SRI验证、digest解析、IP分类及实际cookie安全；其原文按仅扣除上报块核对。net完成WebSocket跨HTTP transaction/cache/shared_dictionary/factory/job/request、URLRequest、SPDY查询、socket pool构造全闭包：删16专用文件，去独立池/端点锁/握手userdata、专用ALPN和构造参数、GN/source/test项及最终enable_websockets开关。普通HTTP池256/6/128、kHttpAll/force_tunnel=false、HTTP2复用/IP与证书检查/TLS/缓存保持，HTTP2 extended CONNECT setting合法性校验保留并更名状态，URL/cookie WS规范化安全算法及Perfetto独立WebSocket实现未动。DNS/proxy调用签名同步，净53个net编辑。89路径备份SHA、删除存在性、保护交集零、5份GN/GNI语法、diff和Chromium目标符号残留检查通过；关键调用参数做源码核对，非C++编译证明。证据out/cut-stage48-combined/、out/cut-stage48-network-devtools/、out/cut-stage48-websocket/report.md。未生成当前图/编译/运行/像素验收；原始全量任务及统一验收未完成，三组拒绝范围未触碰。

stage47 当前接续：stage46 已提交 263f4eada3cf。本批35源码路径、29文件实际删除：components/content_settings全部27个现存文件，network WebSocket协议和mojom/shot_sources.gni。解除无消费者CookieManagerParams/专用CookieAccessDelegateType及2imports、6条现存策略GN边；CookiePriority及后续全部cookie类型原文不变，CookieStore/TLS/资源加载与Blink RendererContentSettings保留。继续追溯stage46保留的WebSocket握手类型，确认仅21条无调用的WebSocket/WebTransport/DirectSocket/P2P探针声明维系，连同前置声明/协议目标/依赖完整移除。网络aggregate GN直接使用原Shot的url_loader_factory及相同2个依赖、空typemap，删除被覆盖的失效源表。备份SHA、删除存在性、源码/构建闭包引用零、21探针引用零、3份GN语法和diff检查通过；源码3行增加/6207行删除，未生成当前图/编译/运行/像素验收。证据out/cut-stage47-network-closure/verification.json与out/cut-stage47-policy-audit/report.md。原始全量任务及统一验收未完成，三组拒绝范围未触碰。

stage46 当前接续：stage45 已提交 0dc3dfa178a0。本批22源码路径、18文件实际删除：6个无消费者网络IPC schema、8个专用traits、net/extras/shared_dictionary 3个无人消费数据库元数据文件及cpp/shot_sources.gni。4份GN同步收口：网络cpp直接保留原Shot的34个生产编译源，移除5个失效测试目标（99个输入此前已不存在）与Blink测试依赖；清掉协议typemap、网络监听feature、空Linux块和shared_dictionary_info目标。生产34源对HEAD按换行归一比较一致；实际请求/响应/Cookie/TLS/HTTP2/共享字典解码/隔离键保持。WebSocket因core_probes.pidl仍用握手类型保留，不把它算成已删。备份SHA、删除存在性、已删文件引用零、4份GN语法和diff检查通过；本批源码减少1541行，未做当前图/编译/运行/像素验收。证据out/cut-stage46-network-build/verification.json与out/cut-stage46-network-audit/report.md。原始全量任务及统一验收未完成，三组拒绝范围未触碰。

stage45 当前接续：stage44 已提交 55ff6f12dc86。本批实际删除build/modules/modularize和unified的独立单元测试/测试数据及仅测试BUILD共35文件；实际mac/libc++使用的generator、modulemap配置、模块GN保持原文。备份SHA、删除存在性、tracked残留零与diff检查通过，无当前图/编译/运行/像素验收。证据out/cut-stage45-module-tests/。根目录复核清单out/cut-stage45-root-review/groups.json按9组区分阻挡与独立待审计：Route、Perfetto离线、ICU、UKM、Crashpad、运行tracing、network policy、Blink交互、UI尾项；这是待解耦范围，不是整组可删证明。优先下一组network协议/content_settings无保护交集，需保留cookie安全/TLS/实际请求类型；tracing/交互/UI须按实际消费者裁剪。实时既有跟踪文件gpu/sql/sandbox/google_apis/device均为零；这些结果不能代替完整原始A/B/C验收和六平台/运行/像素验证。原始全量任务仍未完成，三组拒绝范围未触碰。

stage44 当前接续：stage43 已提交 94899f061ea4。本批6源码文件解除LCP/LongTask/交互响应三条UKM记录链：去builder/include、随机采样常量/状态和测试setter、long_task_counter及LCP viewport统计位；交互函数改名NotifyUserInteraction并移除仅上报参数，保留实际DidObserveUserInteraction回调/时间值/CHECK/UMA/tracing。LCP计算、候选通知与FCP路径保持；LCP OnLcpMetricsForReportingChanged原文逐字一致，Performance AddLongTaskTiming仅扣除UKM块后逐字一致，buffer/overflow/observer保留。备份SHA、diff和本批符号检查通过，无当前编译/运行/像素验证。证据out/cut-stage44-timing-ukm/、out/cut-stage44-performance-ukm/。全tracked Blink direct UKM builder剩6处，均在受保护document.cc/document_loader.cc；DocumentLoader未改。UKM库/协议/生成器尚在，原始全量裁剪与统一验收仍未完成，三组拒绝范围未触碰。

stage43 当前接续：stage42 已提交 d76029b53e20。本批12源码路径移除7处纯UKM记录：FrameLoader生命周期/提交2处、LinkHeader预加载1处、ClientHints Accept/Delegate2处、DataURL1处、Zstd subresource采样1处。完整收掉LinkHeader采样classifier/统计enum、ClientHints上下文UKM方法、ParseDataURL统计参数及3调用端，以及ResourceFetcher UkmRecorder接口/成员/懒创建Mojo获取链。保留真实加载/提交/滚动恢复/原点限制/可信URL/请求头设置/解码/MIME检查及UMA计时；DataURL解析与UMA代码段与原文逐字核对。备份SHA、diff与本批上报符号残留检查通过；没有编译/运行/像素验收。证据out/cut-stage43-network-ukm/和out/cut-stage43-loader-ukm/。UKM库/Document接入、DocumentLoader/LCP/performance/responsiveness其余消费者及受保护依赖仍待解除，全量任务未完成，三组拒绝范围未触碰。

stage42 当前接续：stage41 已提交 46e179c93f7e。本批实际删除build/rust/tests全部104文件、private_code_test全部6文件；清理base测试依赖及CFG测试目标、Rust windows_sys仅测试可见性例外与gnrt配置、bindgen失效示例、Siso两项例外共5配套。合计115源码路径、110文件实际删除；实际Rust编译模板/运行库、Shot测试未变。备份SHA、删除存在性、tracked源码/构建残留零、3份GN语法、TOML/Python-AST兼容Starlark语法及diff检查通过，无当前图/编译/运行/像素验收。证据out/cut-stage42-build/。UKM只读清单另完成41消费路径（保护5、非保护36），out/cut-stage42-document-ukm-audit/consumer-paths.json与independent-edit-plan.md；Document未编辑，下一组可处理frame_loader纯记录块及非保护builder消费者。原始全量任务仍未完成，三组拒绝范围未触碰。

stage41 当前接续：stage40 已提交 ecb62e916c0d。本批解除discardable memory、PaintController、Partitions三处组件CrashKey消费者：删除仅用于附加崩溃信息的计数回调/快照及mapped-size格式化，保留原内存计数/释放/锁、OOM分支、错误日志和NOTREACHED。同步service/WTF GN；删除无人消费的crash_keys与shared_memory_user_stream共7文件及common/utils/旧平台测试目标。当前组件CrashKey外部源码调用为零，但受保护core/BUILD.gn:1214仍依赖crash_key，库本身和Crashpad尚未删除。另删services/metrics孤立根BUILD，清理public/cpp两个无消费者测试目标；UKM真实Document调用与生成器继续待解耦。合计16源码路径、8文件实际删除；备份SHA、删除存在性、已删头引用、4份GN语法和diff检查通过，未生成当前图/编译/运行/像素验收。证据out/cut-stage41-crash-consumers/、out/cut-stage41-metrics/。原始全量清理仍待完成，三组拒绝范围未触碰。另发现build/private_code_test/private_code_test.gni保留已无components/metrics等路径例外，待独立构建骨架清理。

stage40 当前接续：stage39 已提交 841e5c753419。本批实际删除gfx桌面菜单/窗口控制、屏幕更新抑制、提权图标、物理屏幕尺寸探测、GPU子窗口管理、热键观察及native view helper与CrashIdHelper共19文件，3配套移除WindowImpl调试ID/CrashId调用和gfx crash依赖；通知窗口、DPI/字体/颜色及错误检查保持。ui/base删除identifier/metadata/unowned骨架、无消费者时间/字节格式化、startup ResourceBundle helper、collator及4个Mojo协议共18文件，3配套仅收口GN和无用include；保留实际ResourceBundle/l10n字体/attributed-string/menu/windowstate等。合计43源码路径、37文件实际删除。备份SHA、已删头/协议引用、3份GN语法及diff检查通过，关键保留实现与HEAD逐字一致；未生成当前图、编译或运行/像素验收。证据out/cut-stage40-gfx/、out/cut-stage40-base/、out/cut-stage40-base-audit/。Crashpad仍有内存管理/paint/allocator三个活跃诊断消费者与受保护coreGN，UKM及其他原始全量审计和统一验收仍待办；三组拒绝范围未触碰。

stage39 当前接续：stage38 已提交 1cfffcd291bb。本批实际删除无人使用的委托墨迹数据/Point/Renderer Mojo及序列化，移除ChromeClient空SetDelegatedInkMetadata接口；删除favicon/桌面monogram/nine-image/spinner/扩展集合/旧插值及顺序ID辅助。桌面Animation调度/container/runner/subclass及duration scale共24文件退出，Windows字体通知中仅删除更新无人读取动画偏好缓存的调用，字体参数和缓存失效逻辑不变；保留Tween/keyframe及Blink CSS prefers-reduced-motion设置链。另删14个原生Mac/Win/XKB键码转换与旧事件feature文件，保留实际KeycodeConverter、DOM扫描码表、KeyboardCode枚举和时间戳/latency。合计71源码路径、63文件实际删除，8配套含5个无消费者switch/feature收口。备份SHA、删除存在性、4份GN语法、diff及已删头引用检查通过，关键保留文件与HEAD逐字一致；尚未生成当前图/编译/运行/像素验证。证据out/cut-stage39-gfx/、out/cut-stage39-events/、out/cut-stage39-events-audit/。原始全量清理和统一验收仍待完成，三组拒绝范围未触碰。

stage38 当前接续：stage37 已提交 84b604ec07dc。本批实际删除 display 配置/颜色管理 Mojo、native delegate、独立显示旋转及辅助代码40文件，3份GN配套；移除其唯一消费者退出后的gfx overlay转换和Mojo5文件、2份GN配套及头清单。另删Khronos全14文件及Blink无人消费的graphics_types_3d.h，平台GN和头清单同步收口。合计67源码路径、60文件实际删除。Screen/DPI/字体主题/ScreenInfo/DisplayColorSpaces及CPU渲染保持；备份SHA、删除存在性、6份GN语法、diff与已删头引用检查通过。尚无当前构建图、编译或运行/像素验收；原始全量审计、UKM/Crashpad/其余第三方与最终统一验收仍待处理。证据out/cut-stage38-display-audit/、out/cut-stage38-display/、out/cut-stage38-display-deletions/、out/cut-stage38-khronos/。此前三组拒绝范围未触碰。

stage37 当前接续：stage36 已提交 2e30e70cfe3f。本批实际删除原生Event/dispatch/平台桥/devices/Ozone及无人使用event.mojom/serializer共105文件，4份GN解除过宽类型映射和依赖；真实Blink事件枚举、键码/时间戳/latency类型及range-check保留。另删GpuExtraInfo/ANGLE诊断数据及Mojo、VSync基类/export骨架8文件，收掉Screen空GPU信息接口、NativeEvent旧别名/前置声明、12个无消费者GPU/合成switch（7配套）；libsync源码checkout原本缺失，本批删4个wrapper/metadata文件并移除DEPS和.gitmodules同步入口，避免再拉回。合计130源码路径、117文件实际删除；6份GN语法、DEPS/.gitmodules语法、原文SHA/删除存在性/精确残留和diff检查通过，未生成当前图/编译/运行/像素验证。证据out/cut-stage37-events-audit/、out/cut-stage37-events/、out/cut-stage37-events-deletions/、out/cut-stage37-gpu-diagnostics/、out/cut-stage37-libsync/。保留NativeView/Window/Cursor、字体和主题；原始全量审计、UKM/Crashpad/其余平台/第三方及最终统一验收仍待处理，三组拒绝路径未触碰。

stage36 当前接续：stage35 已提交 464aa447afbd。本批实际删除ui/events/blink、gesture_detection、gestures、velocity_tracker及滚动曲线76文件，保留真实Event/键码/时间戳/滚动类型；2处配套解除过宽sources/deps和event.h无用include，Windows WebMouseEventBuilder→ScreenWin调用已随实现移除。另删除gfx GPU Fence/NativePixmap/GPU buffer/IOSurface/CALayer/Swap/Presentation及Mojo序列化66文件，4配套收口memory_buffer/native_handle目标及typemap；buffer_types.h、overlay_transform.h和转换实现4文件与HEAD逐字一致。合计148源码路径、142文件实际删除。备份SHA、删除存在性、已删头/协议引用、3份GN语法及diff检查通过，无当前图/编译/运行/像素验收。证据 out/cut-stage36-events-audit/、out/cut-stage36-events/、out/cut-stage36-events-deletions/、out/cut-stage36-gfx-gpu/。仍需Event serializer/native平台链、Screen GPU附加信息与VSync基类、UKM/Crashpad及原始其他根目录项和最终统一验收；受拒绝三组路径保持原状，不宣称全部已完成。

stage35 当前接续：stage34 已提交 25fe94e6fd39。本批实际删除31个无外部消费者的桌面UI helper、CursorFactory、pointer探测/TouchUiController、device_form_factor/view_prop文件，3配套收口；pointer_device.h仅保留原PointerType/HoverType两enum，枚举体逐字不变。另删除mac DisplayLink/VSync及独占screen_utils共14文件，移除2个开关、Metal链接和Jumbo例外（3配套）；删除浏览器崩溃处理/JS错误上报/WER/旧discardable客户端4个孤立BUILD。合计55源码路径、49文件实际删除。三个GN语法、备份SHA、已删头/符号/目标残留和diff检查通过；未生成当前图或编译/运行/像素验收。保留Blink真实Clipboard/Cursor/Dragdrop类型、Resource/l10n/原生主题、Shot实际共享内存管理器和现有crash annotation调用；UKM及Crashpad的活跃调用仍是待解耦任务。证据 out/cut-stage35-ui-audit/、out/cut-stage35-ui/、out/cut-stage35-ui-deletions/、out/cut-stage35-display-link/、out/cut-stage35-orphan-targets/。受拒绝三组路径未触碰，原始全量审计与最终统一验收仍未完成。

stage34 当前接续：stage33 已提交 d71c109d386a。本批删除 Windows/Cocoa 桌面窗口、光标、OLE、菜单、远程 layer 和无消费者 IME 控制器/候选窗等111文件，4处 GN 配套；保留 mac defaults_utils 的原生主题调用及 Blink 使用的 IME span/text/mojom 类型，blink_headers 依赖收窄至 :ime_types。另删除 PCI/语音播报独立加载器和元数据5文件，清理16处生成器、DEPS include规则、安装依赖及sysroot包清单。合计136源码路径、116文件实际删除。备份SHA、删除存在性、已删头/加载器引用、6份GN语法、Python/DEPS语法和diff检查通过；未生成当前图或编译/运行/像素验证。证据 out/cut-stage34-ui-audit/、out/cut-stage34-ui/、out/cut-stage34-ui-deletions/、out/cut-stage34-linux-loaders/。Route92、Perfetto2039、ICU16待授权范围未触碰；完整原始任务继续待办。

stage33 当前接续：stage32 已提交 757ba692c76e。本批实际删除ui/base桌面accelerators、interaction、models（保留menu_separator_types.h）、base_window、user_activity、WebUI/template及独占cocoa menu/xdg shortcut/themed icon/export配套、空idle骨架和ui/linux status icon，共95文件；ui/base/BUILD.gn与ui/linux/BUILD.gn同步收口并清空无操作分支，合计97源码路径。原生主题实际消费menu_separator_types.h，ResourceBundle/l10n/字体/表单和Linux平台支持保留。备份SHA、删除存在性、已删头/类型及GN标签残留、GN语法和diff检查通过；无生成当前图/编译/运行/像素验证。证据 out/cut-stage33-ui-deletions/、out/cut-stage33-ui/和out/cut-stage33-ui-audit/。与Route92、Perfetto2039、ICU16拒绝清单交集为零，这些待授权范围保持原状；完整原始任务仍有待办。

stage32 当前接续（部分完成）：stage31 已提交 7de589d51fb7。独立处理root build_overrides的Mesa/Meson/PDFium无消费者配置3文件，移除.gn内PDFium/DevTools失效参数及Siso对Dawn的7项例外，共5路径。ICU仅应用BUILD.gn/sources.gni两处配套（normalization_test、Wasm分支、msvcres头清单），保留实际cast→icu-repack→shot数据及native汇编嵌入链。ICU16个独立元数据/测试/Wasm文件删除被自动审批以blocked by policy拒绝，仍全部存在，未重试；与此前Perfetto2039文件分别属于待具体授权清单。不能把这两组写成物理已删。root配置语法/diff、ICU GN语法及备份检查为静态证据，无当前图/编译/运行验证。证据 out/cut-stage32-build-overrides/、out/cut-stage32-icu/；此前Perfetto精确清单 out/cut-stage31-trace-processor/approval-review.md。完整任务仍未完成。

stage31 当前接续（部分完成）：stage30 已提交 8b32de799b8c。已删除Perfetto独立Android/Bazel/Meson入口及生成/检查器18文件、UI构建/发布骨架2文件，移除旧pre-push安装；base/cppgc的TraceProcessor JSON导出分支和对应base buildflag/参数收口8文件。子代理已应用23个GN/普通protozero测试配套编辑，解除libperfetto无条件memory_graph依赖并移动普通测试描述符生成位置。拟删除的2039个processor/离线工具文件被自动审批以blocked by policy拒绝，实时核对2039/2039仍存在，未重试/拆分；绝不能记录为已删。具体清单、原文、已应用补丁及拒绝记录 out/cut-stage31-trace-processor/approval-review.md。其他证据 out/cut-stage31-perfetto-build/、out/cut-stage31-perfetto-ui/、out/cut-stage31-base-json/。当前仅静态GN/Python/diff检查，无图/编译/运行验证；Python/测试/站点及配置残留与原始全量任务继续待办。

stage30 当前接续：stage29 已提交 c9f2c8726ae1。本批删除独立工具/platform_tools/experimental工具109文件，以及bentleyottmann、skplaintexteditor、skparagraph、sksg、jsonreader、skresources、modules/svg、skshaper、skunicode九模块276文件，共385源码文件删除。真实截图SVG仍走Blink SVGImage绘制记录，字体走Blink HarfBuzz/Skia字体端口；保留modules/skcms、pathops源清单、src/svg writer，以及Blink/SkCodec实际调用的experimental/rust_ico五文件。精确调用审计未发现代码/GN外部消费者，余下引用仅历史RELEASE_NOTES。385备份哈希和删除存在性、16GNI语法及diff检查通过；未生成图/编译/运行/像素验收。证据 out/cut-stage30-tools/、out/cut-stage30-modules/、out/cut-stage30-modules-audit/。其他第三方/根目录清单及最终统一验收仍待完成，不据此宣称全部裁剪结束。

stage29 当前接续：stage28 已提交 bc0e517d2f8c。本批整体退役 Skia 独立 Bazel/GN 配置、工具链、构建/生成脚本、全部散布Bazel定义、上游CI/PRESUBMIT及测试、旧vendor roll和Go/Gerrit辅助工具；独立DEPS及ANGLE/Dawn/D3D12/SPIRV/window包装器也已删除。共451源码路径，其中438删除、13份GNI仅更新直接维护说明。主仓三个Skia入口实际导入的16份GNI完整保留，13个注释修改文件的非注释文本逐字一致；16份GN语法解析和diff检查通过。备份/清单 out/cut-stage29-standalone/、out/cut-stage29-gpu-wrappers/，调用审计 out/cut-stage29-standalone-audit/。未生成当前图、编译或运行；Skia其余工具/未用模块和原始其他根目录清单、最终六平台与像素验收仍待完成。

stage28 当前接续：stage27 已提交 df70ff97787b。本批删除 Skia 内置 Vulkan SDK 51文件及其rewrite_includes例外，清理独立DEPS的5个Vulkan条目；另删除21个GPU SkSL生成器/验证器及2份GN/Bazel源清单中的对应声明，覆盖GLSL/HLSL/Metal/SPIRV/WGSL/PipelineStage。共76源码路径、72文件删除。CPU core/core_module源列表逐字不变，4个RasterPipeline文件保留，SkSLGLSL.h的CPU编译器共享类型仍需保留。备份哈希、已删文件名引用、GN/Bazel/Python语法和diff检查通过；未生成当前图、编译/运行/像素验证。证据 out/cut-stage28-vulkan-headers/、out/cut-stage28-sksl/。独立Skia MODULE/Bazel/GN工具和Dawn/ANGLE/SPIRV依赖元数据仍有残留，后续需成组清理；原始根目录清单和最终统一验收仍未完成。

stage27 当前接续：stage26 已提交 e0e0551ed5e4。本批移除 SkMesh 及 Canvas/Device、record/capture/NWay/SVG/XPS 绘制转发和 GN/Bazel 声明；SkBitmapDevice 的该入口原本只有 TODO，实际 SkVertices CPU 绘制保留。同时清理 Skia/cc/Blink 的 texture-backed 查询、GPU 图像类型和加速图像 getter，保留 CPU 解码、缓存、lazy image、gainmap/tonemap、颜色与采样。去重后44源码路径，删除3文件。相关符号跟踪源码搜索无残留，GN语法1项、Bazel兼容语法2项及diff检查通过；未生成当前图、编译或运行/像素验证。证据 out/cut-stage27-mesh/、out/cut-stage27-texture-query/。其他根目录/第三方、生成器、tracing/AX/Worker等原始清单及最终统一验收继续待办。

stage26 当前接续：stage25 已提交 51aed1185794。GPU Slug 的SkCanvas/Device转换与绘制、SkPicture/SkRecord记录回放、序列化数组/标签/回调及capture/NWay转发已移除，删除Slug.h和SlugFromBuffer.cpp并同步GN/Bazel（26路径）；普通TextBlob/字形绘制保留，旧DRAW_SLUG位于枚举末端，删除未重排之前操作号。编码器JPEG/PNG/RustPNG/WebP的GPU context签名和调用方同步收口（15路径），8实现除参数和前置声明外文本一致，质量/颜色/压缩逻辑不变。合计41源码路径，静态调用/残留、GN/Bazel语法及diff检查通过；无C++编译/运行/像素验证。清单和原文 out/cut-stage26-slug/、out/cut-stage26-encoders/。仍需texture-backed跨cc/Blink类型链、SkMesh、生成器/第三方/其他原始根目录清单及最终统一验收。

stage25 当前接续：stage24 已提交 cd12b9220ca8。本批收掉Surface/Canvas/Device的空GPU recorder/context查询、纹理替换、等待/characterization、GPU类型枚举与前置声明（父侧8文件）；SkImage readPixels/getROPixels/onAsLegacyBitmap/makeRasterImage 去掉GPU context参数，CPU子类与缓存/缩放/编码/SVG等调用同步（子代理24文件），移除makeNonTextureImage及仅返回false的Lazy读像素GPU探针。保留正常CPU绘制与实际失败路径。当前仍未编译/运行，diff与调用残留检查为源码证据。清单 out/cut-stage25-surface/ 与 out/cut-stage25-skimage/。下一批必须处理GPU Slug记录/回放/序列化闭包（slug-references.txt），以及texture-backed跨cc/Blink查询和encoder外部GPU参数；不得当作GPU全部收尾完成。

stage24 当前接续：stage23 已提交 110e3281210f。Skia GPU/Ganesh/Graphite 的 src/gpu、include/gpu、include/private/gpu、src/text/gpu 1336 路径，加GPU独占私有头/两份GN源清单，共1346文件已删除；配套GN/Bazel和CPU桥接修改合计1362路径，314,084行删除。阴影源码与旧非Ganesh分支文本2/2一致，SkImageAndroid保留RasterFromBitmapNoCopy；SK_GRAPHITE_USE_LEGACY_RRECT_CLIP_SHADER原本也影响CPU SkRRect，已直接保留原启用公式，未改圆角判断。GN parse4/4、Bazel兼容语法7/7、diff与已删头引用检查通过，未生成当前图/编译/运行。原文备份与精确清单 out/cut-stage24-skia-gpu/、out/cut-stage24-gpu-cpu-companions/。SkSurface等头中无效GPU接口声明/空虚函数、其他shader/toolchain生成器及独立构建配置仍待继续收口，不能宣称全部GPU耦合已完成。

stage23 当前接续：stage22 已提交 e1417567d3f3。本批删除 Skia PDF 后端48文件、2个PDF公开头及pdf.gni，移除 //skia 关闭分支/空PDF实现和SetNodeIdOp、节点ID版文字绘制重载、Blink打印页眉页脚PDF标记；普通文字栅格调用、字体形状和DOM节点获取保留。71个源码/构建路径，51个文件删除，12,615行删除；GN解析3/3、Bazel兼容语法5/5、diff检查通过，当前C++仍未编译。清单/原文/证据 out/cut-stage23-pdf/。PDF特有绘制调用已无残余，其他printing/AX/DOM元数据及未使用独立GN配置仍待清理，不能据此宣称所有打印耦合已处理。子代理完成GPU只读审计：1336个候选及Android CPU边界在 out/cut-stage23-skia-gpu-audit/，尚未删除。

stage22 当前接续：stage21 已提交 4ace701b670c。CanvasKit（Emscripten JS/WASM）和 Skottie（Lottie）278 个文件及配套独立 GN、Bazel、presubmit 引用已移除；Skia 独立根 BUILD.gn/.gn、WASM/iOS示例入口和无用 fuzz.gni 同批删除。295 个源码/构建路径，约 74,699 行删除，原文 SHA 与备份在 out/cut-stage22-skia-{modules,standalone}/；保留主仓 //skia 的构建、原生 SkCanvas、SVG/字体/CPU绘制及共享实现。GN parse、限定 diff、Python/Bazel兼容语法检查通过，无当前图/编译/运行验证。历史 RELEASE_NOTES 中的模块说明保留为历史。GPU/Graphite、剩余独立构建元数据、其他无用模块及整个原始清单仍待继续，不能把本批当作 Skia 内部裁剪完成。

stage21 当前接续：stage20 已提交 d1e909e8c184，以下旧的“未提交”记录是历史。新增清理 base/net/DNS/services 的 FuzzTest 元数据、net_fuzztests/coep_fuzztest/base fuzztest_support，WTF/controller/CBOR 配套以及 WebP 8 个、rapidhash 1 个 fuzzing 目标；删除 libFuzzer proto/renderer 两个无实现 BUILD.gn。12 个源码路径，10 个现存 GN 文件语法解析通过，限定 diff 检查通过；尚未生成当前图、编译或运行。WebP 子仓仍含实际 fuzzing 源文件，通用模板和 Route core 依赖未闭合，均继续待办。证据在 out/cut-stage21-{network-fuzztests,library-fuzztests,proto-renderer}/。

stage20：并行完成 net/DNS/Mojo 9 个 fuzzing 支持目标、base fuzzing_buildflags 和 Linux fuzzing 覆盖率分支、Blink/Skia 49 个目标及 1 个模板清理。14 个源码路径，793 行删除；本批 GN 文件语法解析和限定 diff 检查通过，未生成当前构建图或编译。通用 FuzzTest/libFuzzer 和受 Route 范围约束的 core 包装器仍待闭合。

# 静态截图裁剪任务：完整目标与接续清单

## stage20 接续（未提交、未编译）

在 d2eb46ea6385 之后，已移除 net/DNS/Mojo 的 9 个 fuzzer 支持/协议目标和 Mojo FakeChannelDelegate/Environment 两个独占源文件，共 3 个 GN 修改、2 文件删除。普通网络、DNS、Cookie 和 Mojo 实现保留。三个 GN 文件语法解析通过，精确符号/目标残留搜索通过；清单和备份在 out/cut-stage20-fuzzer-support/。通用 libFuzzer/FuzzTest、base buildflags、Blink/skia 包装器及原始任务其他范围继续待办。本组与后续清理合并提交，最终统一编译。

## 最新用户调整：取消 patch 重放，直接维护源码

三库迁移现在全部落实：ICU/Skia/Perfetto 改由主仓跟踪，四个补丁、本地及三平台 CI 应用逻辑、三个检出项、三个 gitlink 和 Skia hash hook 已移除。13,300 余个后续文件与 ICU 保留文件逐一核对原文，补齐两个符号链接目标，保留 312 个可执行位和 8 个符号链接。原三个完整子仓保存在 out/cut-stage19-direct-source/vendor-backup/。这是源码管理方式迁移，不等于三库所有无用实现已经删完；新版本未编译。当前完整状态以 screenshot-cut-stage19-report.md 为准，以下迁移待办描述为历史。

ICU 迁移已落实：996 个原文件加 .gitignore/来源说明改由主仓跟踪，移除 ICU gitlink、DEPS 和 .gitmodules 入口及其 patch；三平台 ICU 数据时间戳改读主仓路径历史，Windows 不再打 ICU 补丁。原子仓完整保存在 out/cut-stage19-direct-source/vendor-backup/icu（含 .git 和全部本地改动）；保留文件哈希全部一致。历史图记录核查不是当前构建证明，尚未编译；Skia/Perfetto 仍待迁移。ICU 普通文件及 .gitmodules 已暂存以完成 gitlink 转换，其余本轮配置和 stage18 修改尚未提交。

用户要求移除 patches，改为直接修改相应源码。已查明四个补丁覆盖 ICU、Skia、Perfetto，这三者目前都是 mode 160000 Git 子仓和 DEPS 检出项；单改子仓文件不会进入主仓提交。接下来结合原裁剪目标确定实际保留源码，转成主仓普通文件，保护已有全部本地改动与 Git 元数据，再删除四个补丁、build-engine.ts 和三平台 CI 中的应用逻辑，同步更新 DEPS/.gitmodules、时间戳/ICU repack 和文档。避免全量导入约 515 MiB 的 vendor 测试与工具。预检及已修改源文件精确备份见 out/cut-stage19-direct-source/；当前尚未迁移，patch 尚未删除。原全部裁剪目标继续有效，stage18 未提交改动保留。

## 最新源码接续：Mojo fuzzing 闭包（未编译）

继续追加：Breakpad 的 minidump_fuzzer 目标、跨工具链复制入口、专用静默日志配置和 copy_exe 中的 fuzzer 分支已移除，保留原普通工具复制和 ERROR 日志。另在 base、net、DNS、Mojo、URL 等 9 个 GN 文件移除 83 个 fuzzer_test 目标及导入；这些目标的字面 sources 输入均已不在本地，生产实现保留。清单分别在 out/cut-stage18-breakpad-fuzzer/、out/cut-stage18-engine-fuzzers/；10 个 GN 文件语法解析通过，diff --check 通过，无生成/编译/运行，未提交。剩余范围进一步明确为 net/DNS 的 fuzzer 支持 target 和 proto、Mojo core_impl_for_fuzzers、base fuzzing buildflags、Blink/skia 包装器、test.gni/FuzzTest/sanitizer 引擎以及 vendor 测试源码。不能把入口删除当作这些支持链已完成。

追加：已删除 fuzzable_proto_library 模板、自测和包装组，metrics_proto 改为同样参数的普通 proto_library；清单见 out/cut-stage18-fuzzable-proto/。另在 13 个库的 BUILD.gn 中删除 17 处 fuzzer_test 声明（BoringSSL 的一处循环会生成多个目标）、对应 import 和独占种子配置，见 out/cut-stage18-library-fuzzers/。新增 15 个 GN 文件语法解析通过，未生成/编译。保留生产库源码；BoringSSL/RE2 的 gitlink 检出中仍含 vendor 测试源，尚未完成供应商源码切片。base/net/Blink/skia/Breakpad 测试入口及通用 runner、sanitizer fuzzing 引擎闭包继续待办。全部 stage18 修改当前未提交。

在 f4d07d2944a0 之后，已移除 MojoLPM 生成器、fuzzers 两个 GN 骨架及普通 Mojo 模板内的 fuzzing 分支、预编译目标、typemap 别名、protobuf 可见性和 Siso 旧对象引用。普通 C++/Java/Rust 生成器保留；默认生成语言由 c++,java,mojolpm 改为 c++,java。9 个源码路径（6 修改、3 删除），清单和原文件备份在 out/cut-stage18-mojolpm/。三个 GN 文件与两个 Python 文件仅完成语法解析，全仓跟踪源码残留搜索通过；没有生成构建图、编译或运行验收。该组当前未提交，将与后续测试骨架合并提交。Route 92 路径提案仍未应用，与本组没有路径重叠。

下一步按 out/cut-stage18-fuzzing/next-boundary.md 清理通用 libFuzzer/FuzzTest/LPM 测试入口及 sanitizers 中的 fuzzing 专用部分。该范围仍未完成，不能把 MojoLPM 清理算作整个 fuzzing 闭包完成。继续遵守先集中源码、最后统一构建。

**本批本地源码提交：62 删除、65 修改。** 精确路径及提交号记录在 out/cut-stage17/commit.json。仅保存已经应用的父任务改动；Route 92 路径补丁未获具体确认、未应用。部分对应调用方仍待该补丁收尾，本批未生成、编译或运行，不是可构建/验收通过的版本。不 push，不创建 PR。


## 恢复后的源码进度（2026-09-08）

用户已明确继续。当前新阶段以 64f61d2ef182 为基准，out/cut-stage17/progress.json 跟踪接续。父任务已落实网络 connection allowlist / SafeUrlPattern / liburlpattern、无实现的 Network Service GN 骨架、FileReader 与脚本 Object URL 尾巴、JsonCpp 空骨架，并移除不用的内存 HTTP 缓存后端，共删除 62 个普通跟踪文件；Route/CSS/URLPattern 完整提案被自动审批拒绝，等待具体确认，子代理已停止。保留 CSP/混合内容/TLS 等真实加载检查、普通 File/FileList/输入框外观、实际 MemoryCache 和 Shot 缓存 API。这里尚未生成、编译或运行验证；只读 GN path 暴露的一处旧 runtime action 无用赋值已修复。此段优先于下方暂停交接历史。


> 2026-09-08 用户已明确恢复任务（“算了 你继续吧。claude卡死了”）。从 64f61d2ef182 接续剩余裁剪；下面暂停交接记录为历史。本轮先完成 Route/URLPattern 与网络 allowlist 闭包，再集中验证，最多一名已获授权子代理。


> 2026-09-08 最新指令：本批源码收尾后暂停，等待用户接手。下文旧继续计划仅为历史记录；当前状态以 [交接报告](screenshot-cut-handoff-2026-09-08.md) 为准。

## 目标与授权

用户要求完成根目录审计报告中的全部状态：无用实现、仍有引用但截图不用的耦合残留，以及待进一步确认的候选，都必须有实际处理结果。不能只把构建开关关闭、删掉一层调用或把“仍被引用”写成永久保留理由。

依据：`docs/screenshot-unused-code-audit-2026-09-07.md` 是删除前完整快照（根目录及子目录附录）；`docs/screenshot-cut-execution-2026-09-07.md` 记录完成批次和验证证据。本文件是当前任务的接续入口，不替代这两份报告。

2026-09-07 用户确认：Canvas 删除绘图系统，保留无 JS 下影响截图的最小 HTML 兼容；继续全部剩余工作，分批 commit、尽量少量大提交、不创建 PR。用户随后要求完整记录目标，并先完成整个大批次再集中编译，避免频繁构建。本任务不包含发布新版本。

## 不可丢失的产品边界

- 保留 HTML/CSS/DOM、布局、SVG、MathML、文本塑形和字体、图片解码、CPU 栅格化、PNG/JPEG/WebP 输出、文件与 HTTP(S) 加载。
- 保留当前公共 API、CLI、C ABI、Node addon、worker/daemon 协议、缓存、分片截图、加载等待和 CaptureStats。
- V8、content、GPU 进程、DevTools 不恢复；不改变字体栅格化、不启用 `-Oz`。
- Canvas 的绘图上下文、GPU 资源、脚本绘图入口删除；标签备用内容、属性宽高比、已有静态布局行为保留。Skia 的 SkCanvas 是整页 CPU 绘制设施，不属于 Web Canvas 删除范围。
- CSS 动画取值、滚动布局、表单默认外观、lazy loading、XML/XSLT 等不能因“无 JS”一概删除；逐调用点确定实际声明式行为。
- 保护共享工作区的既有修改：Skia、Perfetto、Vulkan loader checkout 中已有局部改动；逐文件核对，不能全仓 stage、reset 或 clean。只暂存本任务明确拥有的路径。

## 完成定义

每一个 A/B 候选必须删除完整功能闭包，或提供新的实际截图调用证据并明确收窄后保留的最小部分。C 候选必须完成调查并落到“删除”或“保留哪些行为、为什么”。不得以“耦合复杂”“以后再处理”作为完成状态。

删除的闭包包括：公开入口、调用者、成员/生命周期、实现、协议/traits、生成器输入、GN target/import/deps/data_deps、DEPS/gitlink/.gitmodules、同步 hook 和磁盘文件。第三方包不能只删本地目录，否则 gclient 会重新检出。

保留目录必须说明其实际职责与剩余子目录，而非只说“底层”。最终按根目录回填状态，并附全仓残留搜索、构建闭包与验证证据。构建图通过、编译通过、真实运行检查通过严格分开；没有完成的六平台编译不能标绿。

## 构建与提交节奏

**2026-09-08 最新用户调整（优先于下方历史批次节奏）：剩余全部范围先连续完成一轮源码裁剪，再统一编译和验收。GPU/Viz、网络与旧 IPC、诊断遥测、第三方、资源/生成器/同步配置连续处理；中途只做必要的调用关系核查、精确备份与轻量静态检查，不再每个组件或每个大类完整构建/跑全套回归。最终集中 GN、缺失输入、编译错误修复、EXE/DLL/运行检查及像素对照，保留六平台验证要求；提交合并为少量大提交。先前已完成的 11 批结果仍有效，当前源码未构建不能冒充已验证。**


1. 先完成一个完整的大批次：调用方、接口、实现、构建配置、实体删除及静态残留检查集中处理。
2. 批内不为每个文件/目录反复启动完整构建。到批次边界集中执行 GN、缺失输入检查、必要语法检查、EXE/DLL 编译与运行验证。
3. 出现编译错误，先收集同批完整错误集合，再按失败 TU 做 syntax-only 修复；只有前端错误解决后才继续完整构建。
4. Windows 构建只能 `pnpm build:engine`、只能 `out/Shot`。已按用户最新指令实试 jobs 20：第五批通过，但第六批多个 Blink TU 再次 LLVM OOM；保留日志后改用 jobs 8，EXE/DLL 成功。用户本次表示已清理后台内存，重新要求 20 并发；第十一批已照做，但再次出现 LLVM 及 PowerShell 宿主 OOM，因此保存本轮日志后降为 8。该最新实测优先于先前的下一次 jobs 20 计划，不修改项目永久默认值。先前 jobs 31 也曾 OOM。
5. 每批完成后用显式拥有路径清单提交；尽量合并相邻低风险链，避免一个目录一个 commit。
6. 每次任务接续先读本文件、执行记录、git 状态和当前批次清单；不要重新应用旧 scratch 覆盖已修复源码。

## 已完成的提交

| 批次 | 提交 | 完成内容 |
|---|---|---|
| 1 | `71ebd6e65771` | SQL/SQLite/VFS/persistent_cache、脚本缓存主链、PFFFT、GL dummy 和 Vulkan data dependency |
| 2 | `f48e852f6cb8` | 设备绑定认证/不可导出密钥、脚本缓存传输尾巴、content/google_apis/extensions、Mojo JS/TS 工具和浏览器空壳 |
| 3 | `7bdc3cfe9b56` | media/device/printing/chrome、blockfile、cc 微基准、媒体公共接口、版本/plist 和 Closure 残留 |
| 4 | `329c2433b200` | Canvas 绘图链、sandbox/storage 服务空壳、栈/堆采样 profiler；176 张像素一致 |
| 5 | `7817855215f4` | PAC/WPAD/系统代理解析服务及生命周期、prefs；116 文件删除，20 并发 EXE/DLL 与完整运行验证通过 |
| 6 | `a9089e98db19` | 浏览器宿主/Widget、popup/plugin、PAC/ScrollingCoordinator 与主合成器入口；249 文件删除，177 张像素一致，Windows 完整运行验证通过 |

| 7 | `bea94b044ba3` | 输入路由/IME/EditContext、浏览器公共接口、Autofill/拼写建议/编辑命令/SystemClipboard；216 文件删除，179 张像素一致 |
| 8 | `13a93d449cfa` | AnimationWorklet、NativePaint、CSS paint()和延迟图片记录；74 文件删除，181 张像素一致 |

前三批 Windows EXE/DLL、84 demos、serve/net、Node/daemon/协议、Bilibili 和像素基线检查已完成，详见执行记录；六平台实际编译未完成。没有创建 PR 或发布。

## 最新状态：本批源码完成，按用户要求暂停

**用户最新要求：本批完成后暂停并交接。此指令优先于下面所有历史 active 目标和继续执行计划；未经用户再次要求，不再裁剪、构建或启动子代理。**

本批七组已明确授权并全部应用：Service Manager、ANGLE/SPIR-V/Vulkan、Blink 拖放、UKM、NQE/FileNetLog、Sanitizer/Skeleton、Origin Trial。加上本轮前半部分 GPU/Viz、旧 IPC、原生 UI、Skottie、第三方骨架、XSLT 脚本 API 与输入预测清理，相对最后验证提交 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb 共删除 1,404 个普通跟踪文件、5 个 gitlink，新增 3 个 Mojo traits 头文件。源码与文档合为一个本地提交，提交号及精确路径见 out/cut-stage16-final/commit.json；不创建 PR、不 push、不发布。

七组源码完成为 7/7；整个裁剪目标尚未完成。已删除 gpu/、ipc/、services/service_manager、ui/gl 及 ANGLE/SPIR-V/Vulkan checkout；五个第三方完整仓库和既有修改保存在 out/cut-stage16-angle-authorized/vendor-backup。仍保留截图所需 CPU 绘制、CSS/DOM/布局、SVG、图片、字体、表单主题、Canvas 备用布局、原生 XSLT、普通 Mojo 和实际网络加载。

静态证据 out/cut-stage16/static-proof-combined.json：15,905 个代码/构建输入、285 个改动 C/C++ 预处理配对、653 份新阶段原始 SHA 备份；扫描发现的一个 drag_state.h 旧 include 已补删，当前记录 0 问题。早期父阶段和 Skottie 备份另有原始证据。

**本批尚未 GN 生成、编译/链接、重建 addon、运行或像素验收，不能宣称可构建或截图无回归。** out/Shot 仍是第十一批旧二进制：183 张已验证像素，第 184 张 Unbounded 只有旧二进制基线；六平台实际编译仍未完成。以后由用户决定是否验证，建议用已成功的 jobs 8（20 已多次实际 OOM），不改永久默认值。

剩余网络/公共协议/FileReader/Blob/Worker、诊断/Perfetto/Crashpad/AX、UI/display/拖放辅助、Route/URLPattern、Skia 实际 checkout 及其他第三方/生成器/同步尾巴，连同接手顺序与回退位置，全部见 [交接报告](screenshot-cut-handoff-2026-09-08.md)。逐根目录状态见 [根目录报告](screenshot-root-status-2026-09-08.md)。子代理已停止。

## 第十一批已提交（当前验证基线）

本段优先于后面的历史进度。基准提交 cd916149eb10080a0c176826e9308f96ef5dd82d；第十一批使用 out/cut-stage15，共 229 个 owned 路径（225 源码/构建输入、4 文档）和 576 个实体删除，总计 805 个变更路径。提交 d9b409db334cb60b0f6b0c11549b7d5c4be7e7bb，全部 Windows 验证通过；stage15 清单冻结。

已移除 View transition 创建入口、Document/DocumentLoader/LocalFrameView/PageAnimator 生命周期、样式调度/显示锁、DOM 伪树、布局/绘制/属性树、跨文档协议/traits、事件/字典和 GN；ForeignLayerDisplayItem 及调用方、无调用的图层/图片缓存入口和 AddToLayerDebugInfo 等也已删除。原 EnqueuePageRevealEvent 的 RouteMap 导航初始化副作用迁为 InitializeRouteNavigationState，仍在原调用时点执行。

CC 的 LayerTreeHost/Impl、layers、scheduler、raster、GPU tiles、帧率/合成指标、动画宿主/时间线、mojo_embedder、ViewTransitionRequest 已实体删除，主目标收窄为 CPU 几何/scroll-snap/sticky 算法和当前公共值类型。动画目标仅保留两种 CPU 滚动曲线及头文件/export。Viz BindLayerContext/LayerContextSettings、layer/layer_context/tiling 及八组 CC 图层协议/traits、旧测试和 Android target 同时移除；基础/调试/资源/指标尾巴另删除 46 文件。保留 CPU paint、图片、滤镜、绘制容器和 StickyPositionConstraint::CanMerge；其余 Viz/GPU、公共帧元数据与浏览器控制参数仍需下一批收窄。

CanvasChildPaintRecord/State、CanvasDrawElement/CanvasForDrawing、ElementCanvasTransform、drawable 子树、专用合成原因及调用方全部移除。唯一隐私绘制标记 producer 已随 Canvas 绘图分支删除，继而清除 kPrivacyPreserving、tainted 属性树传播及不可达绘制分支；普通 CORS/资源来源检查、SVG 滤镜和 HTML/CSS 绘制保留。HTMLCanvasElement 仍保留标签备用内容、属性宽高比及原有布局；PaintLayerPainter 对 Canvas 子分层仍取原 feature-disabled 路径。HTMLMediaElement 空 cc::Layer 与 LayoutVideo 加速判断删除，video poster 保留。AnchorPositionScrollData 比较载荷迁为 Blink AnchorScrollSnapshot，字段、默认值与实际布局计算不变。

元素/区域捕获及追踪已贯通删除：Element/NodeRareData ID 与 subrect 存储、资格判断、强制 effect、密码/iframe 坐标 producer、布局强制 box、HTML/SVG painter、PaintController/Chunker、PaintChunk 成员和 GC/比较/调试输出、CC/Viz 帧字段、CopyOutput 元数据、Mojo/traits/typemap 和 GN。无外部调用的 VideoCaptureTarget 与 CopyOutputBitmapWithMetadata 一并移除；密码控件状态/显示、iframe 正常属性与 srcdoc、实际截图输出保留。浏览器 ImageReplacement 的工厂无调用方，专用远程 iframe、图片布局分支、生命周期/事件、Mojo、开关和八个文件已移除，普通图片加载、解码、object-fit 与失败替代文本不变。

过渡 CSS 尾巴再删除六文件：CSSOM 包装/StyleRuleViewTransition、IDL 和专用 transition.css UA 资源；同步移除规则创建/存储/GC/复制、navigation/types 私有 descriptor、GN/GRD。@view-transition 通过通用未知 at-rule 跳过；普通 view-transition-name/group/class/scope 属性、选择器识别与静态分组/@supports 保留。TwoPhaseViewTransition 尚被普通导航解析使用，本批未改该导航语义。同步分歧已写 docs/upstream-sync.md。

所有实体删除均先确认工作区内普通文件，写精确备份并校验 SHA256 后逐文件删除。清单与证据在 out/cut-stage15 的 manifest.json、deletions.json、deletion-proof.json。最终静态证明：229 owned / 576 删除、16935 个输入扫描、186 个 C++/头文件预处理配对，0 问题。首次备份与基准提交无实质内容差异（11 个仅换行格式差异）。构建图 GN 6839 targets / 852 files、7409 个输入全部存在，IDL 枚举/union dry-run 无缺失。验证后按精确路径非递归删除 10 个空目录，清单 out/cut-batch11/empty-directories.json。

原第十批 182 张基准加 out/cut-view-transition/baseline.png，共 183 张不可覆盖。新增过渡基准含 @view-transition 后普通规则、九组属性/层叠/scope/@supports/变换/裁剪/SVG，来自第十批已验证 EXE（SHA256 b54ec426000f4f9ace00a2d9efc9c13c342f9308a2439e405dbc7444bcb83c31），两次渲染一致并目视检查。原始基准哈希保持不变，当前新二进制已完成全部逐像素对照。

本批已照用户最新要求实际运行 jobs 20，但出现 20 次 LLVM OOM 和 PowerShell 宿主 OOM，保存 out/Shot/cut-batch11-cpp-oom.log 后改用 8。源码错误集中修复并完成当前图 79/79 单元语法检查；续编发现的过渡滚动角残留整链删除，相关 2/2 单元通过，普通滚动条/滚动角 CPU 绘制保留。随后完整编译成功，不改变永久构建默认。

Windows EXE/DLL 均以 jobs 8 编译链接成功，0 失败 edge；新 addon 已重建，旁置 DLL/资源与 out/Shot 哈希一致。serve、net、84 demos（62 exact / 1 fuzzy / 21 smoke）、Node、daemon、协议、Bilibili 离线长页与 accept 全部通过。183/183 张 PNG 解码像素完全一致，动态 clip-path 与等价静态中点完全一致；Chrome oracle 差异仍为 1.524%。EXE 45,542,912 字节，较第十批减少 790,016 字节；DLL 45,540,864 字节。

EXE SHA256：7aee9e13c9505a25a66a99e1f1040fc924434d35146af8e6a5bca4d06667a2d3；DLL SHA256：881a850e5a2a5b8a6a18211f3d0bdf69d29f8012b2936616d05979fe4176a2b5。证据、日志和像素比较位于 out/cut-batch11；最终 EXE 日志 cut-batch11-build-final.log，DLL 日志 cut-batch11-dll.log。

Linux probe 为 0 缺 BUILD、0 主仓库缺输入，仍有 3 个 Linux DEPS 检出项和 1 个宿主工具链输入缺失；jumbo 静态扫描列出 39 个候选，部分平台/生成输入未扫描。这是图与静态证据，六平台实际编译仍未完成。其余 Viz/GPU、协议、网络/诊断、第三方和根目录总复核继续处理，整个目标 active。

拖放 14 文件提案未应用，自动审批曾拒绝且尚未获用户确认；范围在 out/cut-stage11/drag-proposal-review/scope.json。当前独立裁剪不包含该提案，不得重试或拆分。不得创建 PR、push 或发布。

## 第十批验证完成（历史基线）

本段优先于下面历史进度。第九批提交88516c1ec1ad，stage13冻结。第十批使用stage14：79个首次触碰路径（含3个文档、3个新CPU变换快照文件）和22个实体删除，共101个变更路径。Windows EXE/DLL、新Node addon与全部运行检查已完成，没有活动构建或测试。8并发，无OOM；原20并发OOM记录仍有效，永久默认未改。

第十批移除普通动画GPU状态、分组/回执、CompositorAnimation包装与delegate、eligibility/曲线桥接、合成时间线镜像、颜色/透明度/滤镜专用关键帧快照。七个GPU动画样式标记及属性树AnimationState参数/分支、锚点GPU动画状态传播、四个运行开关、遗留测试源项和无用时间范围检查也已删除。

CPU PendingAnimations排队、NotifyReady、未解析滚动时间线延迟、PaintClean后的时序延迟、原IsCurrent触发的on-demand更新保留。快照只保存transform/translate/rotate/scale的TransformOperations用于子像素及轴对齐；neutral keyframes、zoom/viewport刷新保持。SVG原点分离判定移入paint_property_tree_builder.cc，SMIL/资源祖先/zoom/vector-effect/额外SVG变换条件保持。ScrollOffsets和Timing枚举已为Blink本地数据，滚动16微秒/像素保持；ScrollAxis仅保留类内别名，避免遮蔽其他局部类型。

验证：GN6847 targets/856 files，7429输入全存在；IDL dry-run枚举54个/122引用、union42引用均0缺失。首轮14个失败TU已集中修复（缺少CSSProperty声明/PropertyHandle直接include、新快照谓词接口、全局ScrollAxis遮蔽、三个旧AnimationState空参数）；syntax首轮13/14，删无调用SupportedTimeValue后最后1/1 clean，EXE/DLL续编成功。serve/net、84 demos（62 exact/1 fuzzy/21 smoke）、新addon的Node/daemon/协议、Bilibili、accept全通过。182/182原始基准解码RGBA完全一致，动态clip-path与静态中点参考一致；Chrome oracle既有差异1.524%保持。

EXE46,332,928字节，较第九批减少62,464字节，SHA256 b54ec426000f4f9ace00a2d9efc9c13c342f9308a2439e405dbc7444bcb83c31；DLL46,330,880字节，同样减少62,464字节，SHA256 83a1dd6731c0a9746f81a07cb1b96c82812bae37857bcbd00d4276f1a04191e5。运行证据out/cut-batch10/validation.json，源码与22份删除备份SHA256证据out/cut-stage14。17510个源码/GN/生成输入（包括新文件）无删除路径引用，71个owned C++/头文件预处理配对通过，git diff --check通过。新二进制均晚于最后源码修改；Node加载相同SHA256的新DLL。

Linux probe为out/CutBatch10Linux：0缺BUILD/0主仓库缺输入，3项Linux DEPS和1项宿主工具链缺失；Jumbo仍列出40候选，部分文件未扫描，不计实际编译。六平台实际构建尚未完成。

下一批从out/cut-stage14/next-cc-boundary.md接续，必须新建stage15清单，不能重放stage14编辑。已确认PaintChunksToCcLayer::UpdateLayerProperties、ScrollbarDisplayItem::CreateOrReuseLayer、UnacceleratedStaticBitmapImage::CreateFromRaster仅有声明/定义，需拆除其CC层和图片缓存尾巴；实际CPU转换/滚动条绘制不删。ForeignLayerDisplayItem仍有三个view-transition产生方，需沿DocumentLoader的optional导航快照与CSS声明式边界一起裁决。StickyPositionConstraint有真实CPU合并消费者；AnchorPositionScrollData目前通过property_tree.h耦合，需拆纯数据，不能因此永久保留cc宿主。

拖放14文件提案未应用，自动审批拒绝仍未获用户确认；继续其他独立工作。cc宿主/调度/Viz/GPU、网络公共层、诊断与其余根目录闭包未完成，本批通过不代表全目标完成。

## 第九批验证完成（历史基线）

本段优先于下面历史进度。第八批提交13a93d449cfa，stage12冻结。第九批使用stage13：85个首次触碰路径、实体删除20文件，共105个变更路径；Windows EXE/DLL、新Node addon及全部运行检查已完成，没有活动构建或测试。8并发，无OOM；原20并发OOM记录仍有效，未修改永久默认。

本批删除FileReader/Sync脚本入口和返回union8文件、WorkerScheduler/页面代理/Web调度队列7文件、合成线程和专用scheduler5文件。同时删除无创建方的Worklet/ShadowRealm/V8/WebNN token、Mojom/traits/GN映射、Worklet请求destination、CSSOM暴露声明、V8三类队列和任务类别、Worker生命周期限流、后台输入子队列及无用音频实时参数。持久枚举原值不重编号，映射完整性表明确保留2/11/27编号空位。

保留字体/HTML预扫描实际使用的NonMainThread、默认/控制/idle队列、GC与任务清理顺序。MainThreadSchedulerImpl仍是Shot创建真实AgentGroupScheduler的入口，不可改成空SimpleMainThreadScheduler。FileReaderLoader/client/data仍被DataObject读取Blob使用；普通Worker公共token仍受网络/GPU协议耦合，后续继续处理。未改动未获批的拖放提案。

验证：GN6847 targets/856 files、7429输入全存在、IDL dry-run枚举与union检查通过。首轮2个失败TU（枚举保留编号表、ThreadScheduler直接include）已修复，2/2 syntax clean；续编EXE/DLL成功。serve/net、84 demos（62 exact/1 fuzzy/21 smoke）、Node/daemon/协议、Bilibili、accept全通过，181/181原始基准解码像素一致，动态clip-path与静态中点参考一致。Chrome oracle既有差异1.524%保持。

EXE46,395,392字节，SHA256 225b15334de211e7c1216bbfcbe78bf51b749d8e2b63acf164384a8317bca0d4；DLL46,393,344字节，SHA256 e4563b7c2a6fdbd4d8760c46881e7429d8a001f1515ce9017b50861a4879dd21。运行证据out/cut-batch9/validation.json，源码证据out/cut-stage13；20份删除备份SHA256通过，16,835个tracked源码/GN无删除路径残余，42个owned C++/头文件预处理配对通过。

Linux probe为out/CutBatch9Linux：0缺BUILD/0主仓库缺输入，3项Linux DEPS和1项宿主工具链缺失；Jumbo列出40候选，部分文件未扫描，不计为Linux实编译。六平台实际构建仍未完成。

下一批普通动画/cc闭包从out/cut-stage13/next-animation-boundary.md及next-animation-complete-callers.txt接续，使用新stage14清单，禁止重放stage13编辑。已有out/cut-animation-cpu/fixture.html与baseline.png：16组transform/paused/delay/fill/zoom/SVG行为，第九批新EXE渲染两次像素一致，出处与SHA256在provenance.json。下一批像素对照应增至182张。CPU PendingAnimations/NotifyReady、PreCommit的post-paint deferral和IsCurrent触发UpdateIfNecessary副作用、关键帧变换快照参与的子像素与轴对齐判断必须保留。不得按Compositor名称整块删掉。

拖放14文件提案未应用，自动审批拒绝仍未获用户确认；继续其他独立工作。普通动画GPU状态、cc/Viz/GPU运行链、网络公共层、诊断与其余根目录闭包仍未完成，本批通过不等于全目标完成。

## 剩余大批次清单

### 4. Canvas、服务空壳、采样 profiler（已提交 329c2433b200）

- [x] Canvas 专用 layout/painter、context host/factory、font/performance cache、资源 provider、GPU bitmap/drawing buffer/shared context 完整删除。
- [x] 保留最小 HTMLCanvasElement，移除仅脚本可用的 bitmap 尺寸状态、paint 请求队列等残留；保留属性宽高比和实际静态行为。ImageElementBase 的普通图片/SVG 职责保留或迁出 canvas 路径。
- [x] sandbox/storage 根目录、proxy_resolver/proxy_resolver_win、components/services/storage/quarantine 的调用/协议/GN 和实体清理。
- [x] 删除栈采样、heap sampling、线程池 profiler 钩子、企业管理/默认应用工具；保留线程池调度、allocator 基础设施和真实诊断消费者需要的 module cache。
- [x] 集中构建、测试、像素对照、更新记录、提交。

### 5. 浏览器嵌入层、交互、合成器和 GPU 完整闭包

- [ ] Blink WebView/WebFrameWidget/WebPagePopup、ChromeClientImpl/LocalFrameClientImpl 等浏览器嵌入层；Shot 直接 Page/Frame/EmptyClients，保留布局所需最小接口。
- [ ] platform/widget、components/input、输入预测/OneEuroFilter、手势/fling/overscroll、IME、clipboard、dragdrop、鼠标锁定、popup/fullscreen、editing 交互命令/撤销。
- [ ] 保留表单文本/default value 绘制、CSS 焦点/状态、布局选择计算中确实需要的机制。
- [ ] cc LayerTreeHost/Impl、代理、scheduler、tile 调度和 GPU raster provider、帧率/scroll-jank 统计；保留 paint record、CPU 图片与必要 property tree 数据。
- [ ] Viz client/提交、GPU IPC/command buffer/SharedImage、GL context、GPU transfer cache；必要颜色/像素格式/几何类型拆出，不保留整个运行目标。
- [ ] GPU/GL 解耦后同步清理 ANGLE、Vulkan loader/headers、SPIR-V、libdrm/libsync/khronos/DX headers 的无用部分、DEPS、.gn、overrides、gpu_lists_version hook。
- [ ] 六平台窗口/屏幕/Ozone/headless 系统适配逐项收窄，不能破坏字体/颜色/scale。
- [ ] 批次收尾构建、运行验证、像素对照、提交。

### 6. 网络公共层、旧 IPC、浏览器服务与存储接口

- [ ] services/network/public/cpp 拆资源加载实际类型，去掉浏览器 policy/helper 的无用消费者；prefs 48 文件及 GN 依赖已在第五批删除并验证。
- [ ] NetworkContext 全服务、代理/设备/存储等未连接协议与冗余 bindings、service_manager public 服务管理契约、独立 cert_verifier 服务残留。
- [x] net PAC/WPAD/DHCP/系统代理：解析服务及初始化删除，HTTP 建连直接选择 DIRECT；Windows EXE/DLL 和全部运行/176 张像素回归通过。
- [ ] Network Quality Estimator、NetLog 导出、持久 cookie/字典/报告存储空壳逐个确认并收窄。
- [ ] memory cache、First-Party Sets/隔离键/权限等 C 项逐项决策；保留已有缓存语义、安全检查、证书/TLS、HTTP2/HPACK。
- [ ] ipc 老 Channel/Pickle traits 等沿实际消费者拆除；与 Shot 自有 stdio/daemon 协议区分。
- [ ] FileReader/文件选择/Blob 服务通道、Worker/Worklet 线程与接口清理；保留 file URL、普通资源线程池。
- [ ] 批次验证并提交。

### 7. 浏览器诊断、遥测和剩余 Blink 扩展

- [ ] UKM/metrics 服务、metrics_proto、生成器、browser frame 指标和 performance_manager/scenario_api。
- [ ] base profiler 剩余 module cache/trace_event/tracing 与 Perfetto 记录/会话/导出后端，按消费者处理；不能删 CaptureStats、加载判断/FCP、CHECK 或实际日志。
- [ ] crashpad/crash key 接入、device_event_log、skia tracing/benchmark wrapper/跨进程图像适配收窄；保留需要的崩溃诊断。
- [ ] DevTools emulator/probe/异步跟踪残留；保留 Shot 输出的 ConsoleMessage。
- [ ] accessibility 协议/事件链；保留 HTML 属性与 CSS 状态。
- [ ] PerformanceObserver/User Timing、resize/intersection observer 区分脚本回调与内部 lazy loading/生命周期。
- [ ] route_matching/url_pattern/view_transition/origin_trials、XSLT/sanitizer/Trusted Types 明确静态用途；逐项删或保留最小行为，不按目录名判死。
- [ ] 批次验证并提交。

### 8. 第三方、资源、构建工具与根目录最终收口

- [ ] 按报告附录逐项回查 anonymous_tokens、leveldatabase、snappy、libpfm4、jsoncpp、win_virtual_display、inspector_protocol 等骨架。
- [ ] libyuv、re2、liburlpattern、ipcz 按剩余实际调用决策；普通图片转换/URL/Mojo 管道不能误删。
- [ ] 无消费者的上游测试 target/template、google_benchmark/fuzztest/ocmock 等，保留 Shot 测试资产和真实生成工具。
- [ ] Lottie、桌面图标/语言/AX resources、GRD/XTB 和未使用工具 hooks 同步收窄。
- [ ] base 电源事件/提权/系统工具、ui gfx/native/屏幕扩展逐项完成 C 类审查。
- [ ] shot 实验参数/加载 fallback 回调逐项确认；保留公开能力和有用途诊断。
- [ ] 根 .gn、BUILD.gn、DEPS、.gitmodules、prune-deps/trim-tree 白名单与同步行为检查；删除后不再检出同一无用包。
- [ ] 精确删除确认无文件的空目录；不能递归清理共享 checkout 或旧 out 实验目录。
- [ ] 全部根目录及附录逐项回填最终状态，列清保留基础设施/工具/测试，修正历史“全删完”表述。
- [ ] 最终 Windows EXE/DLL 和完整运行检查、Linux/macOS/六平台实际构建边界记录；必须有真实证据，不把 probe 当编译。

## 当前接续信息与验证资产

- 当前分支原为 `release`，最近完成提交 `7817855215f4`。继续前以实时 git 状态为准。
- `out/cut-stage6/manifest.json`、`extra-edits.json`、`tail-edits.json` 记录 Canvas 和本批追加 owned paths；源码已应用，原始 hash 不能用于重放。Canvas 删除清单已扩大到 118 个文件并已实体删除，包括绘图值类型/IDL/生成绑定。`ImageElementBase` 已迁到 core/html 公共位置，canvas 目录仅留标签三文件。
- 暂停时 editing_utilities 的 Canvas image URL 分支已删除。恢复后集中构建仅先发现 thread_pool_impl 缺 FilePath 的直接 include；已修复，失败 TU syntax-only 1/1 通过。
- `out/cut-stage7/manifest.json` 和 `out/cut-stage8/manifest.json` 的服务/采样 profiler 修改已核对 hash 并应用，后续另有收尾修复，不能重新覆盖。临时文件直接位于对应 stage 路径下，没有 scratch 子目录。
- 当前整批已实体删除 225 个普通文件，证明在 `out/cut-batch4/deletion-proof.json`；已去掉 34 个核对过的空目录，sandbox/storage 等根目录消失。GN 通过、7,459 个输入全部存在，IDL 131 个枚举引用/43 个 union 名称引用均无缺失。
- 本批 Windows EXE/DLL 与完整运行回归已通过，176/176 像素对照一致，已提交 329c2433b200。用户要求 jobs 31 后已实试，但多个 Blink animation TU 出现 LLVM OOM，记录 `out/Shot/cut-batch4-j31-oom.log`；已停掉明确属于本任务的 31 并发进程树并确认无遗留编译子进程，自动回退 jobs 8。当前日志 `out/Shot/cut-batch4-build.log`，不要与 OOM 日志混淆。
- 当前第五批已应用：out/cut-stage9/manifest.json 为 owned edits（含修复后的实际源码，scratch 不可重放），deletions.json / deletion-proof.json 记录 116 个实体删除。PAC/WPAD/系统配置/解析服务及 URLRequestContext/HttpNetworkSession 持有关系已删除，HTTP JobController 直接选直连，prefs 48 文件与 GN 依赖删除；已完成 Windows EXE/DLL（jobs 20）、完整运行检查和 176 张像素回归，第五批已提交 7817855215f4。后续继续 jobs 20。
- 旧 IPC 的网络 ParamTraits 仍被 Mojo [Native] ConnectionInfo / EffectiveConnectionType / URLRequestRedirectInfo 真正序列化使用，不能只删除 include；后续需迁为明确 Mojom 字段/枚举及验证 traits，再拆旧 IPC。相关调查位于 out/cut-stage9。
- 原始裁剪基线位于 `out/cut-baseline`，是本次任务从已验证 out/Shot 保存的对照资产，不是新编译验证对象。新产物一律用 `out/Shot`。
- Canvas 八组研究样本位于 `out/canvas-assessment`，保存真实 canvas 与普通元素差异。候选必须与 canvas.png 比，不能与 generic.png 比。
- 完成批次对照：169 demos + 4 render cases + corpus，174 张基线；补上 Canvas 专项 fixture。已有 Chrome oracle 差异 1.5245%，本次不得新增未解释差异。
- 不允许新起子代理（当前没有用户或项目授权）；按当前任务本地推进。

本文件各待办只有完成实际处理和对应验证后才能勾选。预算、运行时间或上下文切换不构成完成理由。

- 下一批统一收尾 Canvas 公共 property tree 子树状态、孤立 canvas_snapshot_info.h 和 ActiveScriptWrappableCreationKey 的旧 Canvas friend 声明；不保留无调用方的绘图 metadata。

- 第五批所有 exec session 已结束，无运行中的构建。下一整批优先拆 Blink 浏览器嵌入层/WidgetBase 与 GPU/合成器创建入口，out/cut-stage6/embedder-* 是早期只读定位，必须以当前源码为准；旧 IPC 的 5 个 Native 类型证据在 out/cut-stage9/native-mojom-refs.txt。不需要重编已验证第五批。

- 第六批正在实施，尚未构建或提交：out/cut-stage10/manifest.json 记录 17 个已应用调用方文件，deletions/deletion-proof.json 记录 widget_creation_observer.h 的删除。已拆无窗口媒体查询分支、widget 创建 observer、scheduler→widget 的任务完成通知（保留真正调度与队列统计）、FCP 的 widget 回调（当时保留 ChromeClient 通知；其并非实际截图等待入口，见后文核实更正）、DOM/HTML unbounded native window 分支与部分 DevTools/cursor 回调。当时尚未删除的主实现已在下述后续步骤删除；WidgetBase/FrameWidget 接口与下游调用仍待清理。需要把整个大批次完成后才集中编译，不能把当前状态当第六批已完成。
- out/cut-stage10/{creation-sites,external-refs,widget-callers,widget-lifecycle-refs}.txt 为当前批次切入证据，已修改调用方后内容是修改前快照；后续搜索当前源码刷新。不得重新应用 scratch 覆盖随后修复。

- 第六批继续：现已实体删除 37 文件（含前述 observer），移除 8 组 WebView/WebFrameWidget/WebPagePopup/WebLocalFrame/WebRemoteFrame/ChromeClientImpl/FrameClientImpl 主体及仅由它们创建的 DevToolsEmulator、ScreenMetricsEmulator、FullscreenController、WebSettingsImpl、FindInPage/TextFinder/FindTaskController、WebInputMethodController 和观察者。六个 GN source list 已同步。当前仍有下游旧引用，源码暂时不构建；必须继续完成依赖闭包后集中验证再提交。删除源码快照保存在 out/cut-stage10/deleted-source，便于迁出确实属于静态机制的实现；不要因为编译报错恢复整个浏览器层。当前 owned manifest 为 22 个路径，其中 WebFrameWidgetImpl.cc 后续已删除；使用去重路径并集提交。

- 下一接续直接处理 out/cut-stage10/remaining-browser-refs.txt 对应的当前下游调用。优先：删除 browser exported WebFrame 包装/序列化/public observer 尾巴、core Frame::Swap（仅 browser WebFrame 调用，保留 Frame::Detach）；拆 LocalFrame::GetWidgetForLocalRoot 剩余调用及 FrameWidget/WidgetBase；LocalFrameMojoHandler 中 WebFrame 查询和输入方法需连同未连接的 Mojom 方法删除或迁出实际核心机制，不能创建返回 nullptr 的替代 WebFrame。FCP 在 core/frame/local_frame.cc 的 ChromeClient::OnFirstContentfulPaint 保留。当前无运行中的构建/测试，不要基于旧二进制宣称第六批通过。

- 第六批后续进度：当前实体删除 62 文件，owned manifest 75 路径（另有本任务清单文档），累计 diff 约 3.34 万行删除；仍未编译/提交。已删除 Frame::Swap/SwapImpl、LocalFrame::SwapIn 和仅浏览器调用的页面交换辅助函数，保留 DetachDocument/XSLT 文档交接及正常 frame 树逻辑；移除 LocalFrameView 同文档导航屏幕呈现回调。
- 已删除 WebFrame 包装实现、HTML/MHTML writer FrameSerializer/WebFrameSerializerImpl 及 public API、WebFrameContentDumper、WebScopedPagePauser、InteractionEffectsMonitor/外部监视器（含 SoftNavigation 订阅、通知、Trace），未删除 MHTML 读取路径。FrameFetchContext/BaseFetchContext 无调用方 WebSocket 握手及其专属混合内容检查删除，普通 fetch 混合内容检查保留。
- 已删除 DOMViewport/viewport.idl 和 window.viewport 脚本属性（保留真实 VisualViewport/CSS 媒体查询），删除 Unbounded native bounds 缓存（HTMLElement/NodeRareData）和 PrePaint 发往原生窗口的分支；保留 active 状态及 CPU paint property 更新。删除 selection 的 Widget 输入作用域、image/text/layout-shift HUD 回调/接口；PaintTimingDetector 的无窗口坐标转换辅助已消除，调用者保留原始矩形语义。
- PointerLockController、DOM/TreeScope 查询、request/exit/事件 IDL、PointerLockOptions 及生成值类型、Page 持有关系、插件鼠标锁 listener/callback/API、输入事件锁定目标分支已删除；WidgetBase/FrameWidget 的 PointerLock Mojom 接口仍待与整层一起拆。删除列表及 hash 证明已同步 out/cut-stage10，均确认文件不存在，旧源码备份在 deleted-source。git diff --check 无空白错误，仅已有 CRLF 提示。
- 当前剩余引用快照：out/cut-stage10/widget-callers-current.txt、deleted-header-residuals.txt（清掉其中 8 个已无用 include 后需刷新）、remaining-browser-refs.txt（部分已过时）、closed-feature-residuals.txt；serialization-refs/pointer-lock-refs/dom-wrapper-refs 是修改前调查，不可作为当前状态。不要重放任何 scratch 或 manifest 原始内容。
- 下一步先处理 LocalFrameMojoHandler 未连接浏览器 RPC（UpdateOpener/WebFrame guard、DIP 坐标输入、浏览器控件等）、RemoteFrame/RemoteFrameOwner/RemoteFrameView 浏览器远端嵌入残留、LocalFrame::GetWidgetForLocalRoot/PrescientNetworking/SaveImageAt；然后 popup/plugin/context-menu/input 与 FrameWidget/WidgetBase 整层，最终 cc/Viz/GPU 创建链。当前 GetWidgetForLocalRoot 还在约 14 个文件出现，不能构建前只删 include 或新加空返回 WebFrame shim。
- PaintTiming 待处理调查：MarkPaintTimingInternal 在无 Widget 且无测试 callback_manager 时，先 Take 三类 image/text 回调并清 pending_paint_events，再直接返回；之后约 200 行是屏幕呈现和 Web Performance 队列，无 Shot 执行。后续需要连生产者/队列一起拆，不能只删 Widget guard 让原来不执行的回调运行。SetFirstContentfulPaint/LocalFrameView/LocalFrame 的绘制里程碑暂时保留；原称其为截图等待链不准确，见后文核实更正。RegisterNotifyPresentationTime 还与 FMP/BFCache 相连，需集中处理；本次尚未改它。
- 本次没有启动构建或测试进程。最新已验证二进制仍是第五批；后续完成第六批整个闭包后集中用 jobs 20 进行 GN/输入/EXE/DLL 和完整运行像素回归。

- **FCP 证据更正（以后接续按此项，不按早期摘要）：** 当前 shot/shot_renderer.cc:120 的 ShotRenderer::ChromeClient 只覆盖屏幕 scale 和 ConsoleMessage，并没有 OnFirstContentfulPaint/NotifyPresentationTime override。WaitForLoad:1113 的真正 loaded 判据是跑过生命周期、HasFinishedParsing、IsLoadCompleted、ActiveRequestCount()==0；FrameClient::DispatchDidHandleOnloadEvents:222 唤醒 CaptureContext。不得把此前摘要声称的“FCP 浏览器通知是截图等待入口”当事实。真实绘制里程碑与调度/字体/图片流程仍需按实际调用保留，不能误删 capture 等待、onload 唤醒、资源判据和 CaptureStats。
- 第六批最新进度：删除 79 文件、owned manifest 100 路径，约 3.91 万行删除，尚未构建或提交。LocalFrameMojoHandler 已移除全部入站 LocalFrame/LocalMainFrame/视频全屏/DevicePosture 接收器、registry 注册、ActiveURLMessageFilter 和约千行浏览器命令处理。frame.mojom 删除 LocalFrame、LocalMainFrame、无人调用的 LocalMainFrameHost 接口；device_posture_provider.mojom 仅保留 CSS 使用的姿态 enum，MediaValues 直接回答原有 kContinuous；Mac TextInputHost/FullscreenVideoElementHandler 协议文件删除。Handler 当前仅持有四个出站 host/reporting/BFCache remotes，后续仍需拆其调用方，并非最终保留结论。
- WebPrescientNetworking（一直因没有 WebLocalFrame 返回 null）工厂/字段、预连接/DNS 提示处理、HTML preload scanner 的 preconnect 请求种类、PreloadHelper 调用、hover DNS 提示已删除；保留真实图片/样式/字体 preload 及其过滤/安全检查。SavableResources 和测试导出接口、对应 Mojo 存储结构也已删除。LocalFrame 的 Save/CopyImageAt、媒体浏览器操作/视频帧 IPC 拷贝、Mac GetCharacterIndexAtPoint 和相应 MediaPlayerAction Mojom 已删除。
- AnchorElementMetrics/发送器/viewport tracker、navigation_predictor.mojom、Document/anchor/scroll/FCP 的遥测挂钩已整链删除；anchor 正常渲染与 render-blocking 处理保留。Mac SubstringUtil 无调用方后删除，HandleShadowDOMInSubstringUtil 运行开关删除。
- PaintTiming 的 MarkPaintTimingInternal 屏幕呈现/Web Performance 回调分支删除，改为 DiscardPresentationCallbacks，只保留原先无 Widget 路径的三个 TakePaintTimingCallback 和清 pending_paint_events 的真实收尾；删除 MarkPaintTiming/last_rendering_update_end_time_、测试 CallbackManager。FMP/BFCache 的 RegisterNotifyPresentationTime 仍留着；image/text/LCP/element timing 生产者的深层遥测裁剪仍待整链处理，不能把这个过渡状态当全裁完。
- 当前 GetWidgetForLocalRoot 仅剩 3 个实际调用：remote_frame_view.cc:286（OOPIF 合成缩放，LocalFrameView:948 会遍历调用 UpdateCompositingScaleFactor）、keyboard_event_manager.cc:274（PWA 对键盘 JS 分发例外）、view_transition_style_tracker.cc:2036（虚拟键盘 resize height）。LocalFrame 的 getter/声明还保留，不能引入返回 nullptr 的新替代实现；接着拆这三处及 RemoteFrame/RemoteFrameOwner/FrameWidget/WidgetBase 全层。element_timing_utils.cc 的 WebLocalFrameImpl 转换并不经过这个 getter，仍需同处理。
- out/cut-stage10/receiver-preconnect-residuals.txt、latest-closed-feature-residuals.txt、removed-receiver-type-refs.txt、remote-frame-view-refs.txt 是本轮调查；早期快照含已删除内容。before-receiver-cut 保存 handler 重写前源码用于迁出真正需要机制，不要整体恢复。预处理条件配对对 80 个当前 owned C++/头文件检查通过，git diff --check 通过；这些不等于语法/编译通过。所有构建/测试仍未启动，第五批产物仍是最近验证版本。下一轮完成大批次后统一 jobs 20。

- 第六批最新接续：已实体删除 183 文件，owned manifest 135 路径，累计 diff 约 6.59 万行删除；尚未编译或提交。本轮删除 RemoteFrame/RemoteFrameView/RemoteDOMWindow/RemoteFrameOwner/RemoteFrameClient/RemoteSecurityContext 和 ChildFrameCompositingHelper/Compositor 共 14 文件，移除 frame token 的远端查找、跨进程 focus/zoom/owner 属性复制、LocalFrameView 的远端遍历及远端合成提交。Frame::ResolveFrame 对非本地 token 返回未找到；真正 LocalFrame 布局、focus、普通 frame owner 与 iframe 标签保留。
- 已删除 GetWidgetForLocalRoot 的最后调用及接口本身，没有新增空返回适配器。删除 platform/widget 全部 60 个 tracked 文件，含 FrameWidget/WidgetBase、LayerTreeView、输入队列/预测/overscroll、Mojo 输入处理与 compositor 桥接；GN source/jumbo exclusion/旧测试和 Android WebView 目标条目一并清除。8 个外部旧 include、Mojo direct_receiver 的 widget friend 与 main_thread 的 worker-pool-delegate friend 已去掉。目录下若还有空目录，最终根目录收尾精确删除，不能递归碰其他路径。
- 外部/内部下拉弹窗、颜色/日期选择器 UI、ChooserResourceLoader、ColorPagePopupController 共 17 文件删除。随后删除 PagePopup/Client/Controller 和 ValidationMessageClient/Impl/OverlayDelegate 共 11 文件；Page 持有、生命周期 layout/prepaint/paint/animation 和 Document 焦点通知已断开。ListedElement 保留真正 validity 缓存与 CSS :valid/:invalid/:user-valid/:user-invalid 更新；ShowValidationMessage 改名 FocusValidationAnchor，保留原有滚动与焦点机制，删除提示气泡和其后台更新队列。普通控件的样式、默认值、校验计算保留。
- LocalFrame 的 PagePopupOwner setter/member/Trace/clipboard 借用和 StyleEngine 的 popup 字体分支、DocumentLoader 的 popup 专用测试 origin/agent-cluster 分支已删除；普通 CSSFontSelector 和正常 owner document 安全来源路径保留。PagePopup/PagePopupCopyPaste 运行开关已删除。动画帧监视器 AnimationFrameTimingMonitor 随 Widget 创建入口消失，现删除 cc/h 和 core_probes.json5 observer 注册；PerformanceMonitor 仍待后续审查。
- IME 的 Widget 输入标志保存/恢复、EditContext 通知 Widget 取消输入法、Unbounded 原生窗口跨 frame 命中测试结构/函数/两处调用删除。LoaderFactoryForFrame 的 WebFrame URLLoaderThrottleProvider 查找和包装 CreateThrottles 方法删除，给真正 URLLoader 构造传空 throttle vector，与原有 EmptyLocalFrameClient 路径一致；URLLoader/真实资源加载仍保留。
- 当前证据：out/cut-stage10/{remote-after-cut,widget-subtree-external,widget-input-external,widget-other-symbols,widget-after-cut,embedder-current,public-embedder-includes,page-popup-refs,validation-client-external,paint-compositor-external,remote-protocol-refs}.txt。带 external/refs 的多为修改前快照，请以当前源码刷新；widget-after-cut 与 popup-validation-after 均无实际残余结果。183 个删除文件均确认不存在，备份 hash 与 proof 一致；112 个 owned C++/头文件 #if 配对无错误，git diff --check 通过，仅已有 CRLF 提示。这些不等于编译通过。gn path out/Shot //shot:shot_core //third_party/blink/renderer/platform:platform 确认 public 依赖直达；没有启动新 GN 或构建。
- 下一步优先 PaintArtifactCompositor / LocalFrameView::PushPaintArtifactToCompositor 及约 33 处外部调用，连到 cc LayerTreeHost/Layer/合成器创建链。当前唯一 PaintArtifactCompositor 创建位于 LocalFrameView:2980，PushPaintArtifactToCompositor 先检查 acceleratedCompositingEnabled；settings.json5:198 默认 false，全仓现存只有 serializer 显式设置 false，没有 Shot 设为 true。必须保留 shot/shot_renderer.cc 使用的 paint_chunks_to_cc_layer（名字含 cc，但承担实际 paint record 转换）。不要盲删整个 graphics/compositing 或 cc/paint。另需继续插件/ContextMenu/FileChooser/ElementTiming 的 WebLocalFrameImpl 残留，以及 public WebFrame/WebView/Widget 与 remote_frame.mojom；这批未闭合前仍不启动大编译。jobs 20，最近已验证产物仍第五批，所有 exec session 已结束。

- 第六批最新接续（合成层调用收窄中）：owned manifest 155 路径、删除 187 文件，当前整批 diff 约 6.75 万行删除；仍未构建/提交。已删除 LinkHighlight/LinkHighlightImpl 4 文件及 Page 持有、所有 prepaint/paint/导航/合成 host 钩子，SetTapHighlight 在当前树没有外部创建调用；同时删除无调用方 Page::DidInitializeCompositing/WillStopCompositing。PrePaint/PropertyTreeBuilder 的点击高亮额外 fragment 条件消除，普通 inline fragment 和 CSS highlight/selection 绘制保留。
- 已清除 PaintPropertyTreeBuilder 的 DirectlyUpdateCcTransform/Opacity 和所有调用、滚动偏移/裁剪矩形直推合成树、VisualViewport 的直推 scale/scroll、FrameCaret 的直推 opacity、CaretDisplayItemClient/Scrollbar/DisplayLock 的合成更新通知。保留原有 CPU property node Update、OnUpdateTransform/Effect/ScrollTranslation、CullRectUpdater、IntersectionObservation、paint invalidation/重绘标志和 caret PaintCaret。DisplayLockContext::MarkNeedsRepaintAndPaintArtifactCompositorUpdate 改名 MarkNeedsRepaint，其真实重绘逻辑未删。Canvas transform/property-tree 子树标志尚待下一轮整链删除，不能认为本轮已闭合。
- 已删除 VisualViewport 中不创建的 cc scroll/scrollbar layers、创建/刷新/颜色更新/foreign layer Paint、相应 effect nodes/Trace/LayerFor* getter；LocalFrameView 的 viewport foreign-layer Paint 调用、Page 初始化滚动条层通知和 LayoutView 的层颜色通知同步删除。视觉视口的缩放/滚动偏移/真实 transform 与 scroll property nodes、TransformNodeForViewportScrollbars 和 UsedColorSchemeChanged 保留。RootFrameViewport/ScrollableArea 的 LayerFor* 与 HasLayerFor* 旧接口删除；ReplacedPainter 直接保留原本会执行的 CPU resizer 绘制，普通 ScrollableArea 重绘状态与 CPU ScrollableAreaPainter 保留。
- DropCompositorScrollDeltaNextCommit 全部 Blink 接口/调用已删除（三个 ScrollableArea 子类、基类、ScrollAnimatorCompositorCoordinator 中断回调、ViewTransition counter-scroll 提交通知）；保留现有 counter-scroll 本身。ScrollingCoordinator::UpdateCompositorScrollOffset 的所有调用/方法删除，但整个 ScrollingCoordinator 和 PaintArtifactCompositor 创建仍留着，后续须一起删除。LocalFrameView 的无外部调用 RunPaintBenchmark/MainThreadScrollingReasonsAsText/SetTracksRasterInvalidations 及 LocalFrame 的 GetLayerTreeAsTextForTesting/CompositedLayersAsJSON 调试入口删除；PaintController 删除两个对应不再调用的强制合成 benchmark enum，CaptureStats 与 Shot 自有性能测量不动。
- 最新 getter 残留在 out/cut-stage10/pac-current-callers.txt：除 LocalFrameView 自身，GetPaintArtifactCompositor 仅 PaintLayerScrollableArea::ShouldScrollOnMainThread 和 UsesCompositedScrolling 两处。SetPaintArtifactCompositorNeedsUpdate 仅 LocalFrameView::UpdatePaintDebugInfoEnabled 内部调用。但不能据此直接删整个 PAC：out/cut-stage10/paint-compositor-types-current.txt 记录 Animation/KeyframeEffect/CompositorAnimations/AnimationTimeline/Trigger/DocumentAnimations/PendingAnimations 的直接 PAC 参数传递，尚未清理。
- 下一具体步骤：先把 ScrollAnimator/ProgrammaticScrollAnimator/ScrollAnimatorCompositorCoordinator 的合成线程状态迁出/删除，仅保留真实主线程插值、offset/position（含 RTL）转换、pending/run/cleanup 状态、取消完成回调和 ScheduleAnimation。证据 out/cut-stage10/scroll-animator-coupling.txt。不能把 SendAnimationToCompositor/ShouldScrollOnMainThread 改成假返回常量；删发送分支及无实际路径的状态，再拆接口。ScrollAnimatorBase 会在禁用平滑滚动时直接实例化，它的即时滚动必须保留；cc::ScrollOffsetAnimationCurveFactory/Curve 用于真实主线程插值，不能随整个 cc/animation 删除。
- 然后收完 DocumentAnimations 的 PAC 传递与 PendingAnimations 主线程就绪。特别注意 Animation::PreCommit 的 Outdated + Playing + CurrentTime + PaintClean + ScriptForbidden 提前 defer 判据在原本无合成器的路径也可能执行；CSS 动画 pending/NotifyReady、滚动 timeline deferred start 和 ready 状态不能随着 GPU 创建盲删。LocalFrameView::RunPaintLifecyclePhase 在无 PAC 时 needed_full_update 原本恒 true，更新 CSS animations/Timeline 的 CPU 行为必须继续。最终移除 PAC/PendingLayer/PropertyTreeManager 的 cc layer 管线时保留 shot_renderer 使用的 PaintChunksToCcLayer CPU record 转换。
- 本轮 static 核验：187 个删除路径均不存在且备份 hash 对应；131 个 owned C++/头文件预处理 #if 配对通过；git diff --check 无空白错误，仅已有 CRLF 提示。这些不是语法/编译/运行通过。没有启动新构建，全部 exec session 已结束，后续完整批次 jobs 20，最近验证二进制仍第五批。

- 第六批最新接续（滚动 CPU 与合成器实删）：owned manifest 195 路径，实体删除 203 文件；整批 tracked diff 约 7.62 万行删除，另有新建 scroll_animation_state.cc/h。未构建、未提交；最近验证产物仍为第五批。新建路径在 manifest 的 sha 为 null，提交时应显式加入这两个新文件，不要遗漏，也不要重放旧 scratch。
- ScrollAnimatorCompositorCoordinator cc/h 已删除，真正 CPU 生命周期迁为 ScrollAnimationState cc/h，保留 Idle/WaitingToStart/RunningOnMainThread/PostAnimationCleanup、Detach 取消、curve、RTL/vertical-rl 的 offset-position 换算、clamp 和 ScrollOffsetChanged。ScrollAnimator/ProgrammaticScrollAnimator 删除 compositor animation 创建、ID/group/附着/提交/接管/中断回调与 impl-only 调整队列；保留同一 cc::ScrollOffsetAnimationCurve/Factory、时钟、曲线调整、立即滚动、开始/完成/取消回调和调度失败 fallback。UpdateCompositorScrollAnimations/UpdateCompositorAnimations 改为 UpdateScrollAnimationState/UpdateAnimationState，所有基类和 LocalFrameView/RootFrameViewport 调用已同步。调度失败后 ProgrammaticScrollAnimator 立即滚到目标并 return，避免原先 Reset 清空 curve 后又设为待启动；行为分歧已记录 docs/upstream-sync.md，尚未运行验证。
- ScrollableArea/VisualViewport/PaintLayerScrollableArea 的 compositor host/timeline getters、ShouldScrollOnMainThread、UsesCompositedScrolling 已删；滚动条保留 CPU 可见性、fade 定时器与 paint invalidation。MayCompositeScrollbar 及 ScrollbarDisplayItem 的 CPU Paint 路径仍存在（其 metadata/图层创建支路待后续拆除）；没有用返回 false 的新空壳冒充删除。
- LocalFrameView 的 PaintArtifactCompositor 持有/Trace/创建/Push/RootCcLayer/getter/更新通知、ScrollingCoordinator 全类两文件和 Page getter/所有权/销毁已删除。DidCompositorScroll 的 core 调用链、按 compositor ID 查找滚动区、仅用于提交的 scrollable_areas_with_scroll_node_ 注册集合已删除。真正 scrollable_areas_、锚定、sticky、CPU property nodes/重绘、PaintTree、NotifyPaintFinished 和 CSS 动画生命周期保留。ChromeClient/EmptyChromeClient 的 AttachRootLayer/GetCompositorAnimationHost/GetScrollAnimationTimeline、Document 的 timeline 挂接/解挂、Animation 的 timeline 挂接/解挂方法和调用已删除，没有补 null adapter。
- PageAnimator 向 cc host 上报 Canvas/inline style/SMIL/RAF/view-transition 统计的字段、setter、ReportFrameAnimations 及 Element/CSSOM/ViewTransition 调用已删除。SMIL ServiceSmilOnAnimationFrame 仍执行；next_frame_has_pending_raf_ 与 PostAnimate 的动画时钟控制保留。DocumentAnimations::GetAnimationsCount 只计 cc host 动画，已删除。
- AnimationTrigger/TimelineTrigger 的 cc trigger 持有、delegate、创建/销毁、异步激活/暂停时间同步、TimelineTriggerRange::ComputeCcBoundaries、Animation::StartTriggeredAnimationOnCompositor/NotifyAnimationStartedAsync/PausedAsync 已整链删除。TimelineTriggerRange 的 Idle/Primary/Inverse enum 改为自身 CPU 状态类型；CSS trigger 范围、play/pause/reverse 行为、pending ready 和真实 UpdateState/ComputeState 保留。DocumentAnimations 的仅用于 cc 的 trigger registry/更新/解挂删除；CompositorEventTrigger/CompositorTimelineTrigger flags 和 PlayInternal 的触发器 cc cancel 例外同步删净。
- PaintArtifactCompositor 和 PropertyTreeManager 四文件已实体删除；此前所有实例都不存在，CompositorPropertyAnimationsHaveNoEffect 的 null-PAC 路径原来返回 false，现删除这段不可执行的图层分析和所有 PAC 参数/前置声明。保留 missing_style_or_layout 判断和 pending 失效状态收尾。DocumentAnimations/AnimationTimeline/Animation/KeyframeEffect/CompositorAnimations/PendingAnimations 以及 SVGImage、ClipPathClipper 调用已同步。随后删除其无人创建的 PendingLayer/ContentLayerClientImpl/LayersAsJSON/AdjustMaskLayerGeometry 八文件和 GN/test source 项。合成目录现在仅留 chunk_to_layer_mapper 与 paint_chunks_to_cc_layer 四文件，它们仍有真实 CPU 使用；不能一并删掉。
- 证据：out/cut-stage10/{scroll-closure-all,scroll-closure-head,scrolling-coordinator-final-refs,animation-trigger-gpu-refs,trigger-callback-closure,trigger-async-callback-refs,page-animation-report-refs,pac-before-physical-cut,after-pac-dependent-refs,layer-client-head-refs}.txt 多为修改前快照；scroll-trigger-host-after/compositing-physical-after 为空表示对应旧入口/include 搜索无残余，不表示编译通过。gn refs out/Shot 对 PAC 文件返回 no matches，这是旧缓存图查询，不能当重新生成或实编译。203 个删除文件不存在且备份 hash 通过；166 个 owned C++/头文件 #if 配对无错，git diff --check 通过，仅已知 CRLF 警告。所有 exec session 已结束，没有构建进程。
- 下一步：动画内部仍保留 GPU PreCommit/group/pending/CompositorAnimation/NativePaintWorklet/KeyframeModel、Eligibility 和 PaintWorklet 子链，必须继续拆，不能把删除 PAC 等同全删完动画 GPU。PreCommit 的 Outdated + PaintClean + ScriptForbidden 延迟和 PendingAnimations 的 NotifyReady/scroll timeline deferred/start/postcommit 必须保留实际 CPU 语义，SVGImage 用 LayoutClean + false 的路径也要保留。Canvas property tree 标志、ScrollbarDisplayItem::CreateOrReuseLayer、foreign layer 的插件/view-transition/embedded/paint_layer 支路和 cc 主机/raster GPU 待处理。浏览器旧 include 的实时清单是 browser-headers-current.txt：插件、FileChooser、ContextMenu、ElementTiming、BlinkLeakDetector 等仍需闭包；LocalFrame/PreloadHelper/HTMLResourcePreloader/CreateWindow 还有早期旧 include 需按真实使用清掉。整批闭合后统一 jobs 20 构建和完整运行/176 像素验证，禁止复用第五批二进制冒充本批通过。


- 第六批最新接续（文件选择/菜单/插件/旧 WebFrame 头文件闭合）：owned manifest 260 路径、实体删除 249 文件；整批 tracked diff 在文档更新前为 500 文件、+331/-84517 行，另有新建 scroll_animation_state.cc/h。仍未提交、未有第六批新二进制。源码达到一次集中验证的批次边界，开始统一 GN/输入/编译；GPU 动画内部、输入、ContainerTiming/Performance 与 public WebFrame 等后续裁剪并未宣称完成。旧条目中“这批闭合前不构建”的切入工作已推进至全部已删头文件无直接残余；不必为后续全部 GPU/遥测项目继续无限扩大本次未验证 diff。
- FileChooser cc/h 与 file_chooser.mojom 三文件、ChromeClient/Empty 打开窗口接口、FileInputType 的选择/取消/目录遍历/Mojo 文件转换/拖入文件路径与 InputType/HTMLInputElement 包装已删除。FilePickerEventsFix、FileChooserOpened probe 和 GN 源项一并删除。保留文件输入的 shadow button/text、required/disabled/multiple/style/value/validity 和现有 FileList 状态；CanReceiveDroppedFiles 仍跟随 DragController 的悬停样式支路，后续输入/拖放整链继续拆，不是永久保留。
- ContextMenuController/Provider、ContextMenuAllowedScope、菜单数据/traits/builder/public structs/协议和无人使用 WebPopupMenuInfo 共 16 文件删除。Node 右键菜单默认处理、Page 创建/持有/Trace、frame.mojom 出站 ShowContextMenu 与 public WebLocalFrame/Client 方法删除；selection/IME/gesture 的 allowed scope 去掉。普通选择/焦点状态保留。ui/base 中 OmniboxContextMenuController 的旧 friend 是后续 ui 清理项，本轮没有宣称 ui 全删。
- WebPluginContainerImpl/public WebPlugin/Container/Document/Params、PluginDocument、PluginData/PluginRegistry、仅插件 sandbox 使用的 SinkDocument 共 14 文件删除；插件专用 BeforeUnloadEventListener 再删两文件，WebPluginScriptForbiddenScope wrapper 再删两文件。插件创建/焦点/输入/打印/合成层/Find/持久化/延迟释放/注册表 MIME 查询/外部处理、Document/Page/LocalFrame/View/Node/LayoutEmbeddedContent 持有及调用已整链拆除，GN/public 列表与 Mojo 同步。原先 Shot 和 SVG/serializer 显式 PluginsEnabled=false，默认也为 false；现在删除开关及 Empty CreatePlugin 空实现，没有添加假 null adapter。
- HTMLPlugInElement 仍是 object/embed 共享的布局/资源加载实现，不能按名称一起删除。保留图片加载、尺寸/属性样式、CSP/混合内容检查、真实子文档加载和 LayoutEmbeddedObject/备用内容、load-event delay 增减；RequestObject 对不支持内容直接失败，HTMLObjectElement 仍走原来的 RenderFallbackContent。删除 PluginParameters、JS 强制布局、插件实例持久化、Flash override 和远端 FrameView 恢复支路。嵌入内容可用状态字段改名 embedded_content_is_available_；仍承担实际 focus/CSP 拒绝状态。NeedsPluginUpdate/UpdatePlugins 等旧命名还承载 object/embed 布局后的加载，后续应按真实机制决定命名，不能误删其完成加载计数。
- 嵌入绘制中插件/远端 frame 的 SVG filter override 和对应 display item/hit-test enum 删除。原先 LocalFrame 路径仅统计并返回空 properties，现删对应计数接口和额外父节点遍历；真正 PaintPropertyTreeBuilder 的本地跨来源 SVG filter 安全处理保持。ViewTransition snapshot foreign layer 仍待后续闭包，不要把这次删插件等同全删 ForeignLayerDisplayItem。
- BlinkLeakDetector cc/h/Mojom 三文件及绑定注册、专用开关/PrepareForLeakDetection wrappers、内部测试 settings supplement、Worklet GC 空接口删除。正常 ResourceFetcher StopFetching、MemoryCache、CSSDefaultStyleSheets 初始化/Reset、真实 cppgc 不动。LocalFrame/HTMLResourcePreloader/PreloadHelper/CreateWindow 中已无用的旧 WebFrame/WebView includes 删除。
- ImageElementTiming/TextElementTiming/ElementTimingUtils 六文件删除，PaintTiming 的创建/持有/Trace/Get/废弃呈现 callback 生产者和 PaintTimingDetector image/background/remove 通知同步去掉。TextRecord 删除 element timing 专属 rect/bit/ctor 参数，TextPaintTimingDetector 删除相应 eligibility 和坐标转换；保留当前 LCP/SoftNavigation 检测、FCP/真实 painting/loading 逻辑。LCP 仍需要截断 data URL 到 100 字符，常量已迁入 largest_contentful_paint_calculator.cc。ContainerTiming/PerformanceElementTiming 和 Element/Node/layout/prepaint 标记仍在，下一个遥测子链须整链继续清理；调查 out/cut-stage10/container-timing-refs.txt。
- 当前静态证据：deleted-header-residuals-latest.json 遍历所有 tracked C++/h/mm 对 249 个已删路径的直接 include，结果为空；browser-closure-after.txt 对旧 WebFrame/WebView/ChromeClientImpl/ElementTiming/LeakDetector 入口检索为空。220 个 owned C++/h 的预处理配对通过，249 个删除文件不存在且备份 hash 通过，git diff --check 无空白错误。以上都不是编译通过。若 inline 修改出现 UNKNOWN 文件写入错误，应先核对哪些已经落盘再继续；本轮 page.h 一次写入错误已精确检查并完成，不能重放 *-edits-once.json 覆盖后续修改。
- 新增基准：out/cut-plugin/fixture.html、baseline.png（1200x1060，16 个 object/embed/PNG/fallback/noembed/file/select/date/color 情况）。使用经过 SHA256 核验的第五批 out/Shot/shotium.exe 生成，provenance.json 实际文件名为 baseline-provenance.json；仅旧版参考，不是第六批回归通过。PNG object/embed 能绘制，file 默认标签文字在旧二进制这张图中已经为空；noembed 是 raw-text。后续候选须对比这张基准，原来 176 张加本图共 177 张。原有 Canvas 应与 out/canvas-assessment/canvas.png 对比，不要用 generic.png。
- 2026-09-07 本轮已开始 pnpm build:engine --gen-only --jobs 20 --log out/Shot/cut-batch6-gen.log（统一验证，不是小修改频繁编译）。执行前 Get-Process ninja/clang-cl/rustc/lld-link 为空，CIM 在沙箱内拒绝访问，未杀任何进程。GN 完成后顺序 missing-inputs、EXE/DLL jobs 20；失败按 build:errors 分组、check:syntax 批量修复后续跑，不恢复整个浏览器层。执行状态以最新工具及本文件后续追加为准。已有 vendor Skia/Perfetto/Vulkan 改动仍须保护，不能 stage gitlink 或整个目录。第六批验证后再继续 GPU 动画/输入/public WebFrame/ContainerTiming 等剩余所有清单项，不能把本次验证当最终任务完成。

- 第六批统一验证进展：GN 第一次通过后 missing-inputs 发现 shot_sources.gni 遗漏的 media/fullscreen_video_element.mojom；已删该源项并重新 GN 成功（6868 targets / 857 files），missing-inputs 7448 个源码输入全部存在。EXE 构建已实际启动 pnpm build:engine --jobs 20 --log out/Shot/cut-batch6-build.log，工具 exec session 95327。本轮编译期间不继续修改 C++ 源码；先收集这批完整失败集合，再按 build:errors / check:syntax 修复。不要另开并行构建；接续先确认此 session/明确任务进程状态。

- 第六批 j20 完整编译已结束（exit 1）：26 个失败 edge，除 8 类普通诊断外还出现多个 LLVM out of memory。完整失败日志另存 out/Shot/cut-batch6-j20-oom.log；旧 session 95327 已结束且没有遗留编译进程。按当前实际内存证据，剩余重型编译改用 jobs 8，不重复让 20 并发 OOM。第一轮 syntax-only 检查 26 个失败 TU 为 22/26 通过，又发现 OOM 掩盖的四处诊断；已修复并运行第二轮 syntax（session 72129，out/cut-batch6/syntax.log）。第六批仍未有新二进制、未提交。
- 编译修复没有恢复浏览器实现：删除 CompositedAnimationRequiresProperties 无调用函数、LayoutEmbeddedContent 仅插件光标 override、PreloadHelper 仅旧 hints 使用的 console helper。LocalFrameView ForceCommitCriteria / RequestMainFrameOnCompositorAnimation 及 ChromeClient/Empty 接口整链删除，接收端原本为空；IntersectionObserver/resize/position anchors 的实际生命周期保持。Document RandDouble、WebDocument referrer policy、FrameFetchContext callback helper、VisualViewport rect conversion 补直接 include；EditContext 暂时直接包含现存 WebLocalFrameClient 中 SyncCondition 定义，后续输入闭包删除该浏览器 selection 同步链。所有修复路径已纳入 stage10 manifest，勿重放旧 scratch。

- 第六批统一验证最终结果：EXE 和 DLL jobs 8 编译链接成功；serve/net、84 demos（62 exact/1 fuzzy/21 smoke）、新 addon 的 Node/daemon/协议、Bilibili 离线长页和 accept 全部通过；177/177 PNG 解码像素完全相同，新增 object/embed/file fixture 也相同。EXE 46,838,784 字节，SHA256 9989fced172bb90ea3f24eaeb0cea9da865e24e7d7d5b7148a324b4fa5ef089f；DLL 46,836,736 字节，SHA256 b882dc9c7c42e00a4bc884db0a2eca7b39de6084258fb49d8f1a979e7ad0a2fa。Linux probe 0 缺 BUILD、0 主仓库缺输入，3 Linux DEPS 和 1 宿主工具链仍未具备，六平台实际编译未完成。全部验证 session 已结束，无并行构建。详细记录已写执行报告第六批，结果资产 out/cut-batch6。
- 本批 249 删除文件均已复核不在磁盘，按精确父路径移除 10 个空目录（含整个 platform/widget，列表 out/cut-batch6/empty-directories.json），未递归清理 checkout。提交前仅明确暂存 stage10 manifest/deletions、两份任务/执行文档和 upstream-sync；Skia/Perfetto/Vulkan 既有改动不动。提交后接着 GPU animation / input / public WebFrame / ContainerTiming 等全部剩余清单，不能将第六批验证通过等同整个目标完成。

- 第七批最新接续：当前 stage11 manifest 为 137 个首次触碰路径，实体删除 216 文件，尚未构建或提交。已在输入/IME/EditContext 和公共 WebFrame 头文件基础上，完成 PaintHolding/WebUI 提交延迟、Widget 输入协议、CSS selector watcher/推测规则收集、Autofill 事件和专用跨表单缓存、execCommand/编辑命令分发表、拼写检查/文字建议及无人调用的格式化/链接命令闭包。正常 CSS 匹配、form owner/validity/reference-target 遍历、表单默认显示保留。不要重放旧 scratch；以当前源码和 out/cut-stage11/manifest.json、deletions.json、deletion-proof.json 为准。
- 系统剪贴板 SystemClipboard、读写/缓存/监听/图片复制链、Frame 懒创建及持有、DataObject 的剪贴板导入和 DataObjectItem 的外部读取分支已经删除；ClipboardHost/Listener Mojom、ClipboardBuffer typemap 和 paste_mode.h 已同步删除。DataObject/DataTransfer/DragController 仍有拖放和编辑调用，下一步继续拆，不能把本批当全部输入闭包完成。
- 新增 out/cut-form-association/fixture.html/baseline.png（1100x950、8 组 form owner/外部控件/fieldset/radio/声明式 shadow DOM/nested form 条件）使用 SHA256 核验的第六批 out/Shot 生成并已目视检查；这是旧版基准，不是当前修改通过。加上 input fixture 后下一批需要比较共 179 张 PNG。最新可靠引擎仍为 a9089e98db19 的第六批；20 并发已实际 OOM，后续重型编译使用 jobs 8。

- 第七批拖放删除当前被自动审批拒绝，尚未应用：11 个调用方文件（MouseEventManager/GestureManager/EventHandler/Page/ChromeClient/EmptyChromeClient/GN）及 DragController cc/h、DragState.h 共 3 个删除。已补证 Shot/shotium 没有鼠标/拖放输入入口，唯一 StartDragging 实现为 EmptyChromeClient 空函数，SVG client 没有 override，截图滚动直接 SetScrollOffset；GN 存在 shot_core -> Blink core 路径。审批仍认为通用事件功能删除范围较大且未经编译，第二次拒绝。不得绕过重试或把提案当已应用。具体只读提案 out/cut-stage11/drag-proposal-review.patch，scope.json 在 drag-proposal-review/，before/after 仅 scratch 对照；stage11DragPlan 存储也未执行成功。需要用户对这条通用交互链的删除明确确认以解决审批阻碍，其他不受影响工作可继续。
- 最新实际静态核对：137 个 owned 路径、216 个实体删除；98 个当前 owned C++/头文件预处理条件配对通过，216 删除文件均不存在且备份 SHA256 匹配，16,927 个 tracked C++/GN/Mojom 文件未发现已删 include/GN 源路径残余，git diff --check 通过。SystemClipboard/Mojom/旧剪贴板 flags 全仓精确搜索为空。仅静态核对，未编译、未提交，二进制仍为第六批。

- 第七批集中验证已启动（拖放提案仍未应用）：pnpm build:engine --gen-only --jobs 8 成功，6847 targets / 856 build files；pnpm missing-inputs out/Shot 的 7431 个源码输入全部存在。pnpm gen:idl enums -n --check 只读检查 54 个枚举、122 个引用值，0 缺失。当前唯一构建为 pnpm build:engine --jobs 8 --log out/Shot/cut-batch7-build.log，exec session 92951；接续先查询此真实会话，不另起并行构建。out/cut-batch7/validation.json 记录 running，179 张像素计划已准备，均未运行。编译期间不继续编辑源码，收集完整失败集合后批量 syntax 修复。

- 第七批首次构建已终止（session 92951 exit 1）：唯一失败是 make_computed_style_base.py 的 ALIGNMENT_ORDER 中 Vector<String> 已无字段使用。已同步删除该旧对齐项，加入 stage11 manifest（现138）；原始日志留存 out/Shot/cut-batch7-generator-failure.log。当前继续同一批构建，session 23479、jobs 8、out/Shot/cut-batch7-build.log；未启动运行检查。

- 第七批 C++ 收集轮 session 23479 已结束：唯一失败 TU 为 Clipboard Jumbo，DataObjectItem 保留的文件令牌克隆缺少 mojo/public/cpp/bindings/remote.h 的直接 include。已补齐，不恢复 SystemClipboard；pnpm check:syntax --from-log 对该 TU 1/1 clean。原始失败日志 out/Shot/cut-batch7-cpp-failure.log。当前 EXE 续编 session 45552、jobs 8、同 cut-batch7-build.log；尚未 DLL/运行/像素验证。

- 第七批提交已确认为 bea94b044ba3，344 files changed / +114 / -45599；提交后根仓库工作区干净，随后仅更新本接续文档。stage11 清单冻结，不再往已提交批次追加源码；下一次实际编辑需新建 stage12 清单。没有运行中的构建/测试，无 PR/推送/发布。
- GPU 动画下一切口已从当前源码重新核实：Platform::IsThreadedAnimationEnabled 默认 false，Shot 和 SVG 平台没有 override，CheckCanStartElementOnCompositor 明确因此加入 kAcceleratedAnimationsDisabled，CreateCompositorAnimation 也受同一门控；证据 out/cut-batch7/animation-platform-gates.txt、animation-compositor-gate.txt、next-animation-entries.txt。不能直接删除整个 PendingAnimations：Update() 默认 true 的 PreCommit 仍有 Playing+CurrentTime+Outdated+PaintClean+ScriptForbidden 的 CPU 延迟；TimerFired 调用 Update(false)，DocumentAnimations 生命周期调用 Update()，NotifyReady 与 scroll timeline 未解析时 deferred 均是实际动画语义。下一批先分离这部分 CPU 收尾，再拆 CompositorState/group/ack/CompositorAnimation/NPW 整链。当前没有应用 GPU 动画新修改。

- 第八批首轮3个失败TU修复后，EXE/DLL与全部运行/181张像素检查通过，所有构建会话已结束。RequiresPropertyNode仍被普通eligibility使用，留待下一动画状态闭包，不能为了删名字误改CPU动画判定。

### 当前具体阻断

Route/CSS/URLPattern 的 92 路径补丁（44 修改 / 48 删除）被自动审批拒绝，真实文件 SHA 92/92 未变，尚未应用。原因、完整补丁和精确清单见 out/cut-stage17/approval-review.md；等待用户具体确认后继续本组。父任务 58 文件删除仍在工作区，尚未提交/生成/编译，本轮不能标为可构建。不得重放或换工具绕过拒绝。

### 独立完成：内存 HTTP 缓存后端

新增删除 net/disk_cache/memory 四文件，以及 InMemory 工厂、builder 选项与两个统计条件的专用分支。Shot 只有禁用缓存或显式 DISK_SIMPLE，OnBackendCreated 失败不选择其他后端；缓存失败继续无缓存运行。内部 HttpCacheParams 默认改为 DISK_SIMPLE，保留既有后续枚举数值；Simple 索引、Blink MemoryCache、Cookie 与 Shot 缓存 API 不变。证据/备份在 out/cut-stage17-memory-http-cache。当前父任务累计 62 删除、66 唯一修改；备份校验通过，未生成/编译/运行。Route 92 路径仍未应用，不能把整个源码批次标完成。
