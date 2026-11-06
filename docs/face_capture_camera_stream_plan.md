# 摄像头采集 · WebSocket 推流 · 前端控制录制 · UE 转码 方案

> 状态：方案已拍板（2026-11-05）。**采集引擎（M1/M2 引擎侧 + `write_png_sequence`）已实现并编译通过**，HTTP/WS 路由与转码作业层待实现（进度见附录 A）。
> 范围：**一期**。人脸检测整体推迟到二期，本文只在 §6 钉住它的插入点与约束，不实现。
> 部署前提：**后端跑在用户自己的电脑上，由打包好的前端 App 拉起**（不是常驻服务器）。因此没有多用户、没有多设备并发。
> 落地位置：后端 `E:\Doodle\src\doodle_lib\http_method\local`（新增文件），复用 `src/doodle_lib/core`、`src/doodle_lib/exe_warp`。
> UE 侧：**零改动**，只调用已存在的 `UMetaHumanFaceCommandlet`。
> 关联文档：[metahuman_face_capture_solution.md](metahuman_face_capture_solution.md)（UE 侧的脸捕管线、`.dhfc` 格式、退出码约定）。

---

## 0. 拍板条目（后文全部按此展开）

| # | 条目 | 结论 |
| --- | --- | --- |
| 1 | UE 的接入方式 | **只走 Commandlet**：独立进程 `UnrealEditor-Cmd.exe <Project>.uproject -run=MetaHumanFace`，无编辑器内桥、无常驻进程、无 IPC |
| 2 | UE 的输入 | **只读 PNG 序列**。mp4 只用于前端回放与人工验收，不进入 UE 链路 |
| 3 | 丢帧补位 | **策略 A：重复上一帧**。补位后的帧序列必须连续无空洞 |
| 4 | UE 进程排队 | 脸捕 Commandlet **与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`**（`awaitable_queue_limitation`，默认 limit=1），不新开闩 |
| 5 | `frames/` 的保留策略 | **转码作业成功后自动删除 PNG 序列、保留 `raw.mkv`；不可配置** |
| 6 | 会话与连接的绑定 | 会话由 `capture_session::instance()` 进程内单例持有，**不挂 WebSocket 连接**；WS 断开不影响录制 |
| 7 | 控制通道 | **控制一律走 HTTP**：`POST /recordings` 开始、`POST /recordings/{id}/stop` 结束、`POST /transcode` 转码。**WS 是纯下行**，不接受任何命令（§3.5） |
| 8 | UE 侧源码变更 | **0**。前提是 M0 真机验证通过（§11） |
| 9 | **推流只有一条** | 本机后端、单用户，**任何时刻最多一条视频推流**。设计上**不提供第二条**：不做多连接分发、不做环形缓冲-history、不做逐连接背压（§5.3） |
| 10 | **同一时刻最多一个会话** | 会话对象只有一个（`capture_session::instance()`），不用查找表、不做会话池。同一台电脑上摄像头只有一个消费者，第二条 WS 连上就**接管**（§3.3） |
| 11 | **帧率定死 25 fps** | 全链路（推流/`raw.mkv`/`take.mp4`/`frames/`/UE `-FPS=25/1`）只用 25 fps，`time_base = 1/25`。不协商、不可配、不做分数换算；采集侧 40 ms 一拍抽帧/补帧（§4.4） |
| 12 | **人脸检测推迟到二期** | 一期**不做**：不写检测代码、不引模型、不预留 `face_status` 事件、不做开录前的人脸校验。§6 只记二期插入点与约束 |
| 13 | **转码作业不登记任务表** | **不写 `server_task_info`、不进任务列表 UI**，只在内存注册表保活；进度只看 `GET /transcode/{job_id}` 与 WS `transcode_progress`（§7.2） |
| 14 | **`ue_project` 必填且必须是绝对路径** | 前端在 `POST /transcode` 的 body 里必须给 `.uproject` 的**绝对路径**；缺失/为空/非绝对路径/文件不存在 → `400 missing_ue_project`。**后端不做相对路径拼接、不猜默认工程**（§7.2、§7.3-8） |
| 15 | **`package_path` 可空** | 空字符串或不传 → 命令行里**不出现 `-PackagePath`**，落点由 Commandlet 自己决定；非空才校验 `/Game/...` 格式（§7.2、§7.3-9、§8.1） |

---

## 1. 端到端链路

```
控制面（前端 → 后端，全部 HTTP）
  POST /face_capture/recordings                    开始录制
  POST /face_capture/recordings/{id}/stop          结束录制
  POST /face_capture/transcode                     转码
  GET  /face_capture/recordings[/{id}]            状态查询（WS 没连时的唯一来源）

数据面（后端 → 前端，单向 WS，不接受任何命令）
  浏览器 ◄──WS── avc_init / avc_fragment / capture_state / transcode_progress / error
                       │
        ┌──────────────┴───────────────┐
        │  后端采集 strand (单 strand)    │
        │  40ms 定时器 → 抽帧/补帧(25fps)   │
        │  cv::VideoCapture 读最新帧        │
        │  ├─► raw_sink   ───────────────► raw.mkv     (FFV1 无损，唯一权威来源)
        │  ├─► h264_encoder(libx264) ────► 分片 ─► ws_outlet(≤8 片) ──► 浏览器 fMP4
        │  └─► align_recorder ───────────► align.json  (seq / host_pts / gaps)
        └───────────────────────────────┘
                       │ 前端 POST stop → 200 返回时 raw.mkv 已收尾
                       ▼ 前端再 POST /transcode（独立动作）
            转码作业 transcode_job（后台协程，输入输出统一 25fps）
              step 1  png_sequence : raw.mkv ──► png_tmp/ → frames/frame_000000.png …   (补位策略 A)
              step 2  mp4          : raw.mkv ──► take.mp4                                (前端回放)
              step 3  ue_commandlet: frames/  ──► curves/*.dhfc  (-FPS=25/1, 共用 ue 闩)
                                  └──────────► 成功即删除 frames/
```

关键点三条：

1. **控制面全在 HTTP，数据面全在 WS**。WS 只往下推帧与事件，不解析任何上行命令（§3.5）。
2. **WS 是可选通道**：不连 WS 也能开始/结束录制、也能查状态——只是看不到实时画面。
3. **录制与转码是两条独立的协程链**。录制在采集 strand 上逐帧串行推进；转码在 `g_io_context()` 上用另一条协程，两者只通过 `session_dir` 下的文件交互，不共享内存对象。
4. **帧率自始至终只有 25 fps**。采集侧用 40 ms 定时器把任何摄像头帧率整形成 25，后面每一环（编码时基、`align.json`、PNG 计数、UE 的 `-FPS`）都直接用这个常量（§4.4）。

一句话总括：`opencv 采集 → raw.mkv（FFV1 无损，唯一权威来源）→ ①png_sequence → frames/ → UE Commandlet → face_curves.dhfc`，`②mp4 → 浏览器回放`，`③H.264 分片 → WebSocket → 实时预览`。

---

## 2. 模块与文件划分

### 2.1 目录布局

`src/doodle_lib/http_method/local` 是 HTTP/WS 入口层，依赖必须下沉到 `core`/`exe_warp`。因此：

```text
src/doodle_lib/core/face_capture/          ← 新增：与 HTTP 无关的采集/编码引擎（已实现）
    face_capture_types.h                    公共结构体、enums、错误码、to_json/from_json
    media_fragment_pipeline.h/.cpp          ffmpeg 编码 + fMP4 分片产出 + raw_sink（无损）
    ws_outlet.h/.cpp                        单推流出口：单在途写、上限 8 片、丢非关键帧
    capture_session.h/.cpp                  会话状态机 + 采集循环 + 产物目录管理（本机唯一的会话）
    frame_align.h/.cpp                      align.json 记录与读取
    png_sequence_writer.h/.cpp              raw.mkv → frames/（含补位策略 A）
```

一期是 **6 组源文件**（`face_detector` 随人脸检测一起推到二期，§6）。`ws_outlet` 独立成组是因为它既要被 `capture_session` 用，又要被 HTTP 层的 WS 协程持有，放在 `capture_session` 里会让两层互相包含。
src/doodle_lib/exe_warp/face_capture/      ← 新增：转码作业与 UE 编排
    face_transcode_job.h/.cpp               三步作业调度、状态机、进度
    face_ue_commandlet.h/.cpp               拼 Commandlet 参数、调 async_run_ue、解析退出码与 face_capture_log.json

src/doodle_lib/http_method/local/          ← 新增：路由
    face_capture.h                          DOODLE_HTTP_FUN_* 声明（全部路由类集中在此，与 local.h 同构）
    face_capture_device.cpp                 设备枚举
    face_capture_stream.cpp                 WS 下行推流（纯输出，无命令处理）
    face_capture_record.cpp                 录制：POST 开始 / POST 停止 / GET 列表 / GET 查询 / 下载 / DELETE
    face_capture_transcode.cpp              转码提交/查询/取消
    CMakeLists.txt                          （改）追加上述 .cpp
```

**没有 `capture_session_registry`**：单用户本机、同一时刻只有一个会话，注册表那一层是多余抽象。会话对象由 `capture_session::instance()`（进程内单例，`static std::mutex` + `static std::shared_ptr<capture_session>`）持有，HTTP 路由与 WS 协程都只是从它取**同一个** `shared_ptr`（§3.2）。之所以用单例而不是"挂在路由对象上"，是因为采集引擎要能脱离 HTTP 层独立跑（M1/M2 就是这样验证的），路由层的 clone 语义也不该参与会话生命周期。

`local.h` 已经有 171 行、18 个路由类，再塞 10 个会失控；把新模块集中到 `face_capture.h` 与既有 `local.h` **分开**：`kitsu.cpp` 里加一行 `#include <doodle_lib/http_method/local/face_capture.h>` 即可。

改动的既有文件只有四个：`http_method/local/CMakeLists.txt`、`http_method/kitsu.cpp`（注册表）、`core/CMakeLists.txt`、`exe_warp/CMakeLists.txt`。**UE 插件与 `vcpkg.json` 都不动**。

### 2.2 命名空间

统一 `doodle::face_capture`（引擎层）与 `doodle::exe_warp::face_capture`（作业层），路由层仍是 `doodle::http::local`。

---

## 3. HTTP 与 WebSocket 路由

### 3.1 注册方式

沿用现有写法，在 `create_kitsu_local_route()`（[kitsu.cpp:307-337](E:\Doodle\src\doodle_lib\http_method\kitsu.cpp)）末尾追加：

```cpp
.reg_c<local::face_capture_devices>()            // GET 设备枚举
.reg_c<local::face_capture_stream>()             // GET + Upgrade → WS（纯下行）
.reg_c<local::face_capture_recordings>()         // POST 开始录制 / GET 列表
.reg_c<local::face_capture_recording>()          // GET / DELETE，{session_id_}
.reg_c<local::face_capture_recording_stop>()     // POST 结束录制
.reg_c<local::face_capture_recording_video>()    // GET take.mp4
.reg_c<local::face_capture_recording_curves>()   // GET .dhfc
.reg_c<local::face_capture_transcode>()          // POST 提交
.reg_c<local::face_capture_transcode_instance>() // GET / DELETE，{job_id_}
```

**持有会话对象的是 `capture_session::instance()` 这个进程内单例**（不是某个路由类）。`local::face_capture_recordings` 只是"开始/列表"两个 handler 的容器，它和其他路由一样通过 `capture_session::instance()` 拿会话，**路由对象上不存任何会话状态**（clone 之后是副本，存了也不可信）。

稳定性检查：**9 个路由类、11 个端点**，没有再增加。（早先还考虑过把 `/stream` 拆成 open/close 两条，已否决——WS 的一生由 TCP 决定，不需要额外端点。）

路由模板必须**一个类一条 URL**（`DOODLE_HTTP_ROUTE` 只接受一个路径字面量），所以 `/recordings/{id}` 与 `/recordings/{id}/stop` 是两个类。`kitsu.cpp:312-333` 里每个排队项都是这个形态，照抄即可。

未注册的路由运行期不可达，这是新增 HTTP 功能唯一容易漏的一步。

### 3.2 接口清单

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/api/doodle/local/face_capture/devices` | 枚举摄像头：`{devices:[{index, name, backend, width, height, fps}]}` |
| GET | `/api/doodle/local/face_capture/stream/{session_id_}` | **WS 升级点**（`has_websocket() = true`，GET + `Upgrade` 才升级）。路径里的 `session_id_` 只用于前端日志与路由占位，**不参与查找**：服务端始终用那唯一一个会话，两者对不上就以会话内真实的 `session_id` 为准并在 `hello` 里回传。**这条连上只下发视频与状态，不接受任何命令**（§3.3） |
| POST | `/api/doodle/local/face_capture/recordings` | **开始录制**。body 见 §3.3「控制面」；`201` 返回新会话 `{session_id, state, width, height, fps}` |
| GET | `/api/doodle/local/face_capture/recordings` | 历史会话列表（磁盘扫描 `session_dir`） |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}` | 单会话状态、帧数、丢帧区间、产物清单 |
| POST | `/api/doodle/local/face_capture/recordings/{session_id}/stop` | **结束录制**。同步等 `raw.mkv` 收尾（≤ 2 秒）后返回 `{state:"stopped", frames, duration_ms}` |
| DELETE | `/api/doodle/local/face_capture/recordings/{session_id}` | 删除会话目录与 `/Game/.../<id>` 备用包；进行中返回 `409 recording` |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}/video` | `take.mp4`（`http_body_file_t`，支持 Range） |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}/curves` | `face_curves.dhfc` 下载（`set_attachment()`） |
| POST | `/api/doodle/local/face_capture/transcode` | 提交转码作业 |
| GET | `/api/doodle/local/face_capture/transcode/{job_id}` | 查询作业（含三步子状态） |
| DELETE | `/api/doodle/local/face_capture/transcode/{job_id}` | 取消作业（正在跑 Commandlet 时置取消标志） |

**控制面全部是 HTTP，WS 是纯下行**。这样前端不需要维护"命令发出去没回应"的重试逻辑：开始/结束录制都是一次 POST，成败直接看状态码与 body；WS 断了只影响画面，不影响任何控制动作。

路由类一律 `DOODLE_HTTP_FUN_C(xxx, local_http_fun)`（要拿登录 token）或 `DOODLE_HTTP_FUN_C(xxx, ::doodle::http::http_function)`（纯本机操作）。**注意 clone 语义**：`http_function.cpp:74` 每个请求都会 `clone()` 一次路由对象（`std::make_shared<Self>(*this)`），所以路由类的成员只能是可拷贝的值或 `shared_ptr`，**不能放 `cv::VideoCapture`、`FaceDetectorYN`、`std::unique_ptr`**。

> **会话对象的归属**：会话由 `capture_session::instance()` 单例持有，所有 handler 与 WS 协程都只是 `auto l_session = face_capture::capture_session::instance();` 取到**同一个** `shared_ptr`。**本机只有一个会话，所以这个 shared_ptr 只有一份，不存在查找表**；路径里的 `{session_id_}` 只用于前端日志与路由占位，不参与查找。

### 3.3 WS 协议

WS 升级线的关键事实（`http_session_data.cpp:157-181`）：只有 `GET` + `websocket::is_upgrade()` + `callback_->has_websocket()` 三者同时成立才升级；`websocket_init(session_data_ptr)` 在 `async_accept` **之前**同步调用，`websocket_callback(stream, session_data_ptr)` 在 accept **之后**调用且拿不到错误码——连接生命周期只能靠第一次 `async_read` 失败感知。

`face_capture_stream` 的约定：

- `websocket_init`：从 `in_handle->url_` 取 `token`（复用 `local_http_fun::parse_header` 的 cookie/Bearer 逻辑）；用 `capture_session::instance()` 拿那**唯一一个** `shared_ptr<capture_session>`，**不要依赖自身成员**（clone 后是副本）。
- 若此时还没有会话（前端没调 `POST /recordings` 就已经连了 WS），就把 `ws_outlet` 先挂到一个**待用槽**上：`hello` 回 `{state:"idle"}`，只下发事件、不下发视频；等 `POST /recordings` 建成会话后，会话把 outlet 接过去，自动开始推流。**前端不需要为了拿画面去重连。**
- 若已有会话且它在 `recording`，**新连接接管推流**：旧连接的 `ws_outlet` 被关掉并发 `error: superseded`，旧连接只读不写（或由服务端直接关闭），新连接成为唯一出口。**同一时刻只有一条推流**，不做多播。
- `websocket_callback`：`co_spawn(g_io_context(), run(std::move(stream)), detached)`，协程内部自己 `async_read`/`async_write`。
- 写端必须**单在途**：`atomic_bool writing_`，与 `socket_io_websocket_core::async_write_websocket`（`websocket_impl.cpp:124-158`）同构。控制消息是 text JSON，视频分片是 binary，切换用 `stream.binary(true/false)` 的 RAII guard（同 `binary_data_guard`）。**一条 WS 上不能并发写**，这是 beast 的硬约束。
- 读端**只做保活与断连检测**：收到任何帧都忽略，靠 `async_read` 的失败感知连接已经断掉（从而清掉 `ws_outlet`）。**没有命令解析、没有 `post` 到采集 strand 的分派逻辑**——控制面已经全在 HTTP（§3.5）。
- 降级：若客户端协商不到 `Sec-WebSocket-Protocol`，仍按普通 WS 走；诊断信息统一用 JSON 事件下发。

#### 分片装载格式

一个帧分片 = 一条 text 事件 + 紧随的一条 binary 消息（两帧，前端按到达顺序配对）。text 里的 `seq` 用于配对：

1. 前端收到 `avc_init` → `SourceBuffer.appendBuffer(init)`；
2. 前端收到 `avc_fragment` → 等下一帧 binary → `appendBuffer(moof+mdat)`；
3. 服务端**可能跳号**（§5.3：WS 写不动时丢分片），但**只会在关键帧边界重新同步**：丢掉的分片之后，下一个 `keyframe=true` 的分片一定完整送到。前端不需要实现跳片逻辑，只需在收到 `capture_state: recording` 时清空 buffer。

服务端保证同一连接上 `seq` 严格递增（可能不连续）；`seq` 与 `raw.mkv` 里的帧号通过 `align.json` 的 `seq` 字段一一对应（§4.4），这也是"录制到第几帧"的唯一口径。

#### 下行事件（text JSON）

| event | 时机 | 负载要点 |
| --- | --- | --- |
| `hello` | 连接建立 | `{session_id, state, width, height, fps:25}` |
| `avc_init` | 连接建立时补发、以及每次新会话开始录制 | `{bytes, seq_begin, seq_end}`（fMP4 的 `ftyp+moov`），紧随一个 binary 帧装 init 段 |
| `avc_fragment` | 每个分片 | `{seq, keyframe, size}`，紧随一个 binary 帧装 `moof+mdat` |
| `capture_state` | 状态变化 | `{state:"idle"/"recording"/"stopping"/"stopped"}` |
| `transcode_progress` | 转码阶段推进（≥1 Hz 节流） | `{job_id, step:"png_sequence"/"mp4"/"ue_commandlet", state, progress, detail}` |
| `error` | 任何失败 | `{code, message}`，`code` 取自统一错误码表（`to_string(error_code_t)`） |

一期就这 6 个事件。**没有 `face_status`**：人脸检测在二期（§6），一期前端拿不到、也不需要拿人脸框。

所有事件都是**纯下行**。`transcode_progress` 由后台作业广播到当前唯一的 WS；**WS 没连时事件直接丢弃**，前端恢复连接后用 `GET /transcode/{job_id}` 补齐当前状态（轮询 `GET /recordings/{id}` 同理）——即"WS 是可选的低延迟通道，不是唯一数据源"。

#### 上行：什么都没有

WS 上**不定义任何上行命令**。前端不需要发 `ping`（浏览器原生 WS 没有心跳 API，beast 侧也不做空闲超时：单用户本机连接不该被服务端主动断开）；连接是否活着由浏览器自己感知，重连就是重新 `GET /stream/{session_id}`。

#### 控制面（HTTP）

| 动作 | 请求 | 成功返回 | 失败 |
| --- | --- | --- | --- |
| 开始录制 | `POST /recordings`，body `{device:0, width:1280, height:720}` | `201 {ok, state:"recording", width, height, fps:25, frames:0, duration_ms:0}` + `session_id` | `409 already_recording`（`error_code:"already_recording"`）、`422 camera_open_failed` |
| 结束录制 | `POST /recordings/{session_id}/stop` | `200 {ok, state:"stopped", frames, duration_ms, truncated}` | `409 not_recording`、`404 unknown_session` |
| 查询状态 | `GET /recordings/{session_id}` | `{state, frames, gaps, artifacts, session_id, width, height, fps}` | `404` |
| 提交转码 | `POST /transcode`，body 见 §7.2 | `202 {job_id, state:"queued"}` | `409 job_running`、`400 missing_ue_project`（`ue_project` 缺失/为空/非绝对路径/文件不存在）、`400 bad_package_path`（非空但不是合法包路径） |

要点：

- **开始录制是同步的**：`POST /recordings` 在返回 `201` 之前已经完成了 `cv::VideoCapture` 打开、分辨率回读、编码器与 `raw.mkv` 的建立。所以前端拿到 `201` 就意味着"确实在录"。代价是这一个请求会阻塞几十到几百毫秒（打开 DSHOW 设备是慢操作），实现时 `co_await` 采集 strand 上跑的 `capture_session::start()`，路由本身返回 `awaitable`。
- **结束录制也是同步的**：等 `raw.mkv` 的 `av_write_trailer` 落完（上限 2 秒）再返回，前端拿到 `200` 就可以直接去拉 `/video` 或提交转码，不需要轮询等 `stopped`。
- 状态变化仍通过 WS 的 `capture_state` 推送一次，作为"另一条路也能看到"的冗余，但**前端应以 POST 的返回值为准**。
- 重复调用是幂等的：已在 `recording` 时再发 `POST /recordings` 返回 `409`（不悄悄重开录）；已 `stopped` 时再发 `stop` 返回 `409`。

### 3.4 为什么不用 socket.io

`/events` 命名空间的 socket.io 广播（`socket_io::broadcast`）适合低频状态广播，但它的一次写只装一段 text 和若干 binary，且广播语义是"发给所有连该命名空间的客户端"——**本机只有一个前端、只有一条推流，广播语义纯属多余**。视频分片还要自己控制"写不完时怎么办"（§5.3），在广播模板里做不了。所以推流走独立 WS；`/events` 仅用于列表页刷新这类可选的低频事件。

### 3.5 为什么控制不走 WS

WS 传命令的代价是**全双工状态的维护**：命令要有 id、要等 ack、超时要知道是"服务端没处理"还是"连接断了"、断线期间发出去的命令要不要重放。这些都是为了解决"网络不可靠"才需要的机制。而这里的现实是：**后端就在用户本机，被前端 App 拉起，HTTP 走 127.0.0.1**，请求-响应模型没有不确定性——POST 返回 `201` 就是真的开录了，返回 `409` 就是真的没开。

所以：

| 关注点 | WS 命令 | HTTP POST（采用） |
| --- | --- | --- |
| 知道动作成没成 | 等下一条 `capture_state` 事件，超时要自己定 | 看状态码与 body，浏览器原生支持 |
| 断线期间发命令 | 需要客户端队列与重放策略 | 请求本身就失败了，前端直接提示重试 |
| 调试 | 要开 WS 面板看帧 | `curl` 一行就能复现 |
| 服务端实现 | 读协程里解析 + `post` 到 strand + 回执状态机 | 一个 handler，返回 `awaitable` |

结论：**WS 只做单向的帧与事件下发**，是"监控画面的管道"，不是"控制通道"。控制全在 HTTP。

---

## 4. 采集引擎

### 4.1 线程与执行器

- **一条 `boost::asio::strand`（`make_strand(g_io_context())`，实现里是 `capture_session::impl::strand_`）承载全部采集步骤**：取帧、编码、写分片、推流分发。`cv::VideoCapture::read()` 与 `avcodec_send_frame` 都是阻塞调用，必须 `post`/`co_spawn` 到 strand 上跑，**绝不能直接在 io 线程里同步阻塞**，否则整个 HTTP 服务卡住。
  - 代价要说清：strand 跑在共享的 `g_io_context()` 上，一次 `read()` 阻塞仍会占住一个 io 线程。`g_io_context()` 是多线程跑的（`app_base` 起 `hardware_concurrency()-1` 条线程），单个阻塞只会降低并发而非死锁，**一期就按这个方案做**。若将来实测发现卡顿，再改成专属 `io_context` + 1 线程。
- 一帧一拍：`async_wait` 一个 `steady_timer`，固定 40 ms 一拍（`next_tick += 40ms` 的绝对时间累加，避免漂移）→ 取帧 → 分发 → 下一拍。详见 §4.4。
- 停止：**不用 `cancellation_signal`**。`stop()` 把 `state_` 改成 `stopping`，拍点循环在下一拍醒来时看到状态变了就 `break` 退出（`tick_loop` 的 `while (state_ == recording)`）；`stop()` 自己接着等推流排空、收尾编码器、写 `align.json`。这样清理路径只有一条，且幂等。
- 本轮**不采集音频**，`raw.mkv` 不带音轨。**注意**：UE 侧 `bSkipTongueSolve` 默认就是 `true`（`MetaHumanFaceTypes.h:51`），所以无音轨与 UE 的默认参数天然一致，不需要传任何额外开关。

### 4.2 会话状态机

```
idle ──POST /recordings──► recording ──POST /stop、错误──► stopping ──收尾完成──► stopped
                                │                                            ▲
                                └── 析构 / 下一个 start() ────────────────────┘
```

- `stopping` 是**短暂中间态**：`stop()` 把状态置成 `stopping` 并下发 `capture_state` 事件，然后**在同一个协程里**等推流排空（≤ 2 秒）→ 收尾三个编码器 → 写 `align.json` → 置 `stopped` 再发一次事件。所以前端看到 `stopping` 之后必定能等到 `stopped`，`POST .../stop` 返回时状态已经是 `stopped`。
- 新一轮 `start()` 进来时若仍是 `recording`/`stopping` → `409 already_recording`（`error_code_t::already_recording`）；否则先 `close_resources()` 收拾上一个会话的残留，再建新的。
- **本机只有一个会话对象**：`capture_session::instance()` 单例（§3.2）。所以"设备互斥"退化成"状态机不许重入"——没有第二个会话能来抢摄像头。
- `session_id` 在 `POST /recordings` 成功时生成（`uuid`），`session_dir = core_set::get_set().get_cache_root("face_capture") / session_id_str`。
- `stopped` 之后再 `POST /recordings`：**不开新会话对象，只换 `session_id` 与 `session_dir`**。上一个会话的 `raw.mkv`/`align.json` 已经被落在旧目录里，不迁移、不覆盖。
- WS 断开**不改变状态**；`recording` 期间重连是接管（§3.3），不是新建会话。

### 4.3 取帧与像素约定

- `cv::VideoCapture(index, cv::CAP_DSHOW)`（Windows 首选 DSHOW，失败回退 MSMF；DirectShow 更容易设置分辨率/帧率并在某些设备上更稳）。
- 请求 `CAP_PROP_FRAME_WIDTH/HEIGHT` 后**必须回读实际值**；帧率**不回读也不采用**——一律按 §4.4 固定 25 fps。设备若不支持 25 会给出它自己最接近的帧率，由我们的定时器抽帧/补帧吃掉差异。
- OpenCV 输出 BGR。**三条色彩口径，都要写死在实现里**：
  - **推流 / `raw.mkv` / `take.mp4`（编码链）**：`cv::Mat` 是 BGR24，**直接**用 `av::VideoFrame::wrap(..., AV_PIX_FMT_BGR24, ...)` 交给 `av::VideoRescaler` → `AV_PIX_FMT_YUV420P`。**不做 `cvtColor(BGR→RGB)` 中转**——多一次全帧转换只是多花 CPU，`swscale` 本身就有 BGR24 输入。
  - **`frames/`（step 1）**：ffmpeg 解码 `raw.mkv` → `av::VideoRescaler` 到 `RGB24` → `cv::Mat`（`CV_8UC3`，**不要**再 `cvtColor`）→ `cv::imencode(".png", ...)`（OpenCV 把 `CV_8UC3` 当 RGB 编码）。
  - **颜色来源**：swscale 默认按 `SWS_BICUBIC` 做 BGR↔YUV，`VideoRescaler` 用默认 flags（`SwsFlagAuto`）。**因为两条链路同源（同一份 `cv::Mat` BGR24、同一个 swscale），口径天然一致**，`take.mp4` 与 `frames/` 的逐像素比对（M4 验收）不应有系统偏差；若实测平均色差 > 2，先查是不是有人在中途插了 `cvtColor`，再考虑统一 `colorspace/color_primaries/color_trc`（都设成 `bt709` + `tv` range）。
- 正立与 sRGB 是 UE 求解链的硬要求（`FHyprsenseRealtimeNode` 只在正立画面上工作），摄像头驱动一般不做旋转，**画面翻转只允许在前端做，不进后端**。
- 像素格式统一到 `AV_PIX_FMT_YUV420P`（H.264 通用选择）；奇数分辨率会被 libx264 拒绝，`POST /recordings` 时把宽高向下取偶。

### 4.4 帧率：固定 25 fps（定死，不协商）

**全链路只有一个帧率：25 fps，`AVRational{25, 1}`，`time_base = 1/25`。** 推流、`raw.mkv`、`take.mp4`、`frames/`、UE 的 `-FPS=25/1` 全部用它，**没有任何一处需要按 num/den 分数推理**（25 是整数，不会出现 `30000/1001` 那种情况）。

**采集节奏**：采集 strand（`capture_session::impl::strand_`）上一条 `steady_timer`，按绝对时间累加 `40ms` 一拍（`next_tick += 40ms`，不是 `sleep(40ms)`，避免累积漂移）。每拍**产出一帧**，来源按下面的优先级：

| 拍点到来时 | 做法 |
| --- | --- |
| 有比上一拍更新的摄像头帧 | 用新帧（中间被硬件丢掉的旧帧直接忽略） |
| 没有新帧（摄像头更慢，例如它只出 15/20 fps） | **重复上一帧**（与 §10 的补位策略 A 同一原则，录制时长与墙钟保持一致） |
| 摄像头更快（30/60 fps） | 每拍取一帧，其余丢弃（**不做运动模糊、不做降采样平均**） |

- 这样 `pts = seq`、`seq` 连续，**录制链本身不产生空洞**：一拍一定落一帧进 `raw.mkv`，除非摄像头完全卡死（下一条）。
- 摄像头打开时仍尝试设置 `CAP_PROP_FPS = 25` 让驱动走原生 25，但**不依赖它成功**：设置失败也照常按 40 ms 抽帧。
- `POST /recordings` 的 body 里**没有 `fps` 字段**：前端没有选择余地。`GET .../recordings/{id}` 与 `align.json` 里有 `"fps": 25`，那是给下游消费者读的常量，不是可配项。
- **摄像头完全卡死（连续 > 1 秒没有新帧）时不补帧**：这一段拍点不往 `raw.mkv` 写，只在 `gaps` 里记一条区间（`{from_seq, to_seq, missing}`），**`seq` 照常只按实到帧递增**。所以 `align.json` 的 `frames[]` 与 `raw.mkv` 的包序**严格一一对应**（第 i 个 `frames` 项 ↔ 第 i 个解码帧）——这是 `png_sequence_writer` 消费解码帧的唯一依据，**实现里绝不能因为记 gap 就推进 `seq`**（否则 PNG 会整体错位）。短暂掉帧（< 1 秒）仍然走"重复上一帧"，不进 gaps。
  - 空洞的"时长"体现在两处：`gaps[].missing`（该补多少帧），以及 `expect_frame_count = frames.size() + Σ gaps.missing`（`frames/` 应有多少张）。
  - `capture_result.frames` 与 `align.json` 的 `duration_ms` 都按 **`frames.size() + Σ gaps.missing` × 40ms** 计，即"成片时长"，与 `frames/` 张数一致，而不是 `raw.mkv` 的包数。
- 时长口径：`duration_ms = 总帧数（含 gaps）× 40`，与墙钟无关；`align.json` 里的 `host_pts_us` 只用于诊断卡顿，不参与任何换算。

---

## 5. 编码与推流

### 5.1 编码器

| 项 | 取值 | 理由 |
| --- | --- | --- |
| 编码库 | `libx264`（`av::findEncodingCodec("libx264")`，已有同款写法见 [ffmpeg_video.cpp:78-86](E:\Doodle\src\doodle_lib\core\ffmpeg_video.cpp)） | vcpkg 的 `ffmpeg[all-gpl]` 已带 |
| preset / tune | `veryfast` / `zerolatency` | 编码延迟 ≤ 1 帧 |
| profile / level | `main`；`level` 不显式设，由 libx264 按分辨率自选 | 浏览器兼容面最宽；硬写 `3.1` 会在高分辨率时被拒 |
| GOP | `keyint = 25`（1 秒）、`sc_threshold=0` | 重连等待 ≤ 1 秒；25 来自 §4.4 的固定帧率 |
| rate control | `crf 23`（或 `bitrate 4M` 上限） | 局域网够用且画质稳定 |
| 像素格式 | `yuv420p` | 见 §4.3 |
| 时基 / 帧率 | `AVRational{1, 25}`，`AVRational{25, 1}` | 见 §4.4，全链路常量 |
| B 帧 | `max_b_frames = 0` | 零延迟推流；无 B 帧时 `pts == dts`，分片切分不需要处理重排 |

实现取值：`preset=veryfast`、`tune=zerolatency`、`crf=23`（`AV_OPT_SEARCH_CHILDREN`）、`sc_threshold=0`、`profile=main`、`gop=25`。**`level` 不显式设**——libx264 按分辨率自动选，硬写 `3.1` 反而会在 1080p 时被拒。

编码器封装：`avcpp` 的 `av::VideoEncoderContext`，或直接 `libavcodec`（`ffmpeg_video.cpp` 两种都有先例）。**禁止**用 `cv::VideoWriter`——它无法产出分片容器。

> `ffmpeg_video` / `ffmpeg_video_resize` 是**文件到文件**的成品转码工具（输入 `FSys::path`、输出 `FSys::path`、`process()` 同步跑完），拿不到逐帧回调，**不能复用来做推流**；step 2 的 `take.mp4` 也建议新写一条轻量封装，而不是复用它的字幕/水印/片头片尾那一整套逻辑。

### 5.2 fMP4 分片

- 容器层用 `libavformat` 的 `mov`/`mp4` muxer，`movflags` 组合：`frag_keyframe+empty_moov+default_base_moof`，另加 `flush_packets=1`、`mov_write_trailer=0`；
- 实现方式：avcpp 的 `av::CustomIO` 子类（`memory_io`）承接 muxer 的输出字节，`format_context_.setFormat(av::OutputFormat{"mp4"})` + `openOutput(io_.get())` + `writeHeader(movflags...)`；
- **降级路径（M1 必须准备）**：若实测发现自定义 `AVIOContext` 无法稳定切出 `moof` 边界，改成**手工封装**——`libavcodec` 出 H.264 Annex-B packet，分片时把每个关键帧起始包转成 AVCC（`h264_mp4toannexb` 的反向），自己拼 `moov`（一次）与每片的 `moof`（`tfhd/tfdt/trun`）+ `mdat`。这条路完全可控、无 muxer 状态，代价是自己维护 `trun` 的 `sample_duration/sample_flags/composition_time_offset`。**实现时先按 muxer 方案做；仓库不写测试，M1 靠"浏览器里能播、切页回来能续"人工验收；一旦 `moof` 边界错乱（表现为 `MEDIA_ERR_SRC_NOT_SUPPORTED` 或画面花屏）立即切手工封装。**
- 分片边界取 `AV_PKT_FLAG_KEY`：每遇到关键帧 packet 就切一个片段（片段 = 一个关键帧 + 其后所有非关键帧），与 `frag_keyframe` 语义一致；
- **第一个片段的特殊身份**：`empty_moov` 使得 `writeHeader` 输出的 `ftyp+moov` 就是 **init segment**（浏览器 `SourceBuffer.appendBuffer` 的第一块）。实现里把它作为 `h264_fragmenter::init_segment()` 单独取出，`seq_begin = seq_end = 0, key = true`，其余片段才是 `moof+mdat`。**不要每帧重开 muxer**。
- 全部编码器的时基固定为 `{1, 25}`（§4.4），`pts` 就是 `seq`；**不要**从 `host_pts` 换算，也不要按摄像头真实出帧率设时基。

### 5.3 分发（单推流，无背压层）

**本机只有一条推流**，所以这里没有"分发器"，只有一个**出口**：

```text
capture_session::frames ─┬─► media_fragment_pipeline(推流编码器) ─► ws_outlet ─► 那一条 WS
                         └─► raw_sink(FFV1) ─► raw.mkv
```

- `ws_outlet` 就是当前绑定的那条连接的写接口（`shared_ptr`，可为空——没连 WS 时推流编码直接**不做**，省 CPU）。
- **写不动就丢**：`ws_outlet::send_fragment(fragment)` 是非阻塞返回值的调用。内部是一个**上限 8 片**的 `std::deque`，用一个 `atomic_bool` 保证**同时只有一条写协程在途**；`drain()` 依次取队首写出去。
- **丢非关键帧**：队列里还压着没发出去的关键帧分片时，新进来的**非关键帧分片直接丢弃**（它反正要靠后面的关键帧重同步）。所以异常时队列最多积到 8 片，随后被关键帧覆盖式清空。**没有任何环形缓冲、没有 history 补发**。
- 为什么不需要更复杂的背压：单用户本机 + 同机前端，WS 走 loopback，单帧分片 10–60 KB，写满窗口的概率极低；偶发丢片最多让画面卡一下，**录制链（`raw.mkv`）完全不受影响**，因为 `raw.mkv` 与推流是两条独立的编码器，各自从同一批 `cv::Mat` 取帧。
- 这也意味着**§3.3 里不需要 `gap` 标记**：丢片时前端只会停在上一帧，下一个关键帧到达自然恢复（GOP = 1 秒）。
- 只由采集 strand 触碰 `ws_outlet`；WS 协程只在写完成时回到 strand 复位"空闲"标志。**不要裸 `std::queue` 跨线程**。
- 页面切后台导致 WS 停读：beast 的 `async_write` 挂着，队列一直满，分片被丢 —— 这正是期望行为，**不超时、不主动断开**（录制还在跑，断开就没法控制停止）。

### 5.4 录制侧（raw.mkv）

- 与推流**同一批 `cv::Mat`**，走第二条编码器：`FFV1`（`AV_CODEC_ID_FFV1`，level 3），封装 `matroska`。**像素格式不强制转换**：直接用 `AV_PIX_FMT_BGR24`（FFV1 支持 bgr0/bgr0p16 等 RGB 布局），省掉一次 swscale；若实测该布局编码失败再退回 `yuv444p`。
- 为什么 FFV1 而不是 H.264：`raw.mkv` 是转码与 `frames/` 的唯一权威来源，必须**逐帧无损**，否则 PNG 序列里会有压缩痕迹，UE 求解精度受影响。
- 体积代价：1280×720@25 约 7–13 MB/s，1 分钟约 0.4–0.8 GB；1080p 再放大 4 倍。默认分辨率取 **1280×720@25**（帧率定死，见 §4.4；分辨率在 `POST /recordings` 时可调）；超时长（> 10 分钟，见 UE 侧"单段 ≤ 10 分钟"约束）在 `capture_state` 里给 `warning`，不强制中断。
- 落盘线程：`av_interleaved_write_frame` 在无损编码下是大块顺序写，放在采集 strand 上会和采集抢同一个 io 线程。用一条**专用写盘线程**（实现里是 `std::thread` + `mutex`/`condition_variable`）+ 有界队列（上限 300 帧，`k_max_raw_queue_frames`），队列满时**丢帧并返回 false**，**绝不阻塞采集主循环**。
  - 丢帧记在哪：`raw_writer::push()` 返回 false 时 `capture_session` 置 `truncated_ = true`，`align.json` 的 `truncated` 就是这个含义（`raw.mkv` 比 `frames[]` 少帧）。**注意**这会让 `frames[]` 与 `raw.mkv` 包序不再一一对应 —— 这是"写盘线程丢帧"的已知代价，一期不做额外补偿，只标记 `truncated`；转码作业看到 `truncated: true` 时应在 step 1 前后对帧数做一次一致性检查（解码出来的包数 vs `frames.size()`），不一致就判 `raw_frames_mismatch`。
- **坏帧与短尾**：`POST .../stop` 到达时可能还有几帧在队列里，收尾必须先 `drain` 队列再 `av_write_trailer`；`stopping` 状态最长等 2 秒，超时直接 trailer 并记 `align.json` 的 `truncated: true`。**这个等待发生在 HTTP 请求内**，所以 `stop` 的响应时间上限就是 2 秒，前端超时要设得比它大。

---

## 6. 人脸检测（**二期，本方案不实现**）

**拍板：人脸检测整体移出本方案，本方案不写任何检测代码、不引入任何模型、不新增 `face_status` 下行、不做开录前的人脸校验。** 一期交付的是"能录、能推、能转、UE 能出曲线"，画面里有没有正脸由人在前端自己看。

之所以留这一节，是因为下一期要在**同一个采集循环里**插入检测，位置与约束得先钉住，免得二期把采集节奏改乱：

### 6.1 插入点（二期需要遵守的边界）

- 检测必须挂在 §4.4 那一条 40 ms 的拍点上，**不能自带定时器**，也不能反过来拖慢拍点。做法是"这一拍跑不完就顺延到下一拍"，而不是"等检测跑完再出帧"。
- 检测失败、超时、模型缺失一律**不影响录制与推流**：`raw.mkv` 与 `frames/` 的产出与有没有检测结果完全无关。
- 一期**不预留** `face_status` 事件、不预留 `align.json` 的 `face` 字段、不预留 `kitsu_ctx_t::face_detector_model_path_`。二期加的时候是一次纯增量改动，不改一期任何已有字段的语义。

### 6.2 选型（二期的结论，供参考）

| 候选 | 结论 |
| --- | --- |
| **OpenCV `cv::FaceDetectorYN` + YuNet** | **推荐**。OpenCV 4.10+ 自带，接口稳定，输入 320×320，单帧 ~2 ms CPU，5 点 landmark 可直接用于前端对齐提示 |
| onnxruntime 自接 SCRFD | 需要自己写 anchor 解码与 NMS，收益有限 |
| Windows MediaFoundation 人脸 | 精度低、不可控 |
| 前端 `FaceDetector` API | 只有 Chrome，且拿不到服务端判定 |

**前置风险（二期开工第一步就要验）**：`vcpkg.json:76-85` 的 opencv 是 `default-features: false` + `contrib/freetype/openexr/tbb/ffmpeg`，`FaceDetectorYN` 属于 opencv 主模块的 `objdetect 4.x` 模块；`objdetect` 通常随 `default-features: false` 保留，但**没在真机编译验证过**。二期第一个动作就是写一行 `#include <opencv2/objdetect/face.hpp>` 编一次；编不过再考虑走 `contrib` 的 DNN 前向手写。

### 6.3 模型分发（二期）

沿用现有资产根（`kitsu_ctx_t::get_depth_model_path()` 的范式，[kitsu_ctx_t.h:32-38](E:\Doodle\src\doodle_core\metadata\kitsu_ctx_t.h)）：新增 `face_detector_model_path_{"models/face/face_detection_yunet_2023mar.onnx"}` + `get_face_detector_model_path()`，模型文件由部署侧放到 `<root>/models/face/`，**不打进二进制或 install 目录**。
二期的 `cv::FaceDetectorYN` 走 OpenCV 自带的 DNN 前向，**不需要 `Ort::Env`/`Ort::Session`**，`get_ort_env()` 与本模块无关。

### 6.4 运行策略（二期）

- 检测**降频**：固定每 2 帧一次 = **12.5 Hz**（§4.4 帧率定死 25），跑在采集 strand 上；单次检测耗时 > 80 ms（两帧间隔）就顺延到下一拍，把实际频率如实上报。
- 检测分辨率：把帧 `resize` 到长边 640 再送检，边界框按比例映射回原尺寸。
- `status` 取 `none` / `ok` / `multi`，**任何取值都不影响录制**。是否在开录前拦下"没检到脸"的场景留到二期再定（一期没有这道闸，所以一期也没有 `422 no_face`）。
- 线程模型：`FaceDetectorYN` 对象**不可拷贝**，放进 `std::shared_ptr<face_detector>`，只在采集 strand 上调用 `detect()`，销毁也在 strand 上完成。

---

## 7. 转码作业

### 7.1 三步流水（顺序固定）

| step | 输入 | 输出 | 实现 | 完成后 |
| --- | --- | --- | --- | --- |
| 1 `png_sequence` | `raw.mkv`（25 fps） | `frames/frame_{n}.png`，`n` 从 0 连续 | ffmpeg 解码（`-vsync 0`，逐帧不插值）+ 补位策略 A | 通知 step 3 可启动 |
| 2 `mp4` | `raw.mkv` | `take.mp4`（H.264，25 fps，无音轨） | 与 step 1 并行，同一份 raw 只读 | 前端回放 |
| 3 `ue_commandlet` | `frames/` | `curves/` | `async_run_ue(...)`，`-FPS=25/1`（§7.3） | 成功后删除 `frames/` |

> 前端的"结束录制"（`POST .../stop`）只负责关掉采集并落好 `raw.mkv`；转码是**另一次显式动作**（`POST /transcode`）。两者不自动串联，避免用户在录完之前就被迫等待一次几分钟的 UE 求解。

- step 1 失败 → 整作业失败（UE 不可能跑）；step 2 失败 → 作业 `done_with_warning`（曲线已产出，只是没有 mp4）；step 3 失败 → 整作业失败，**保留 `frames/` 供人工重跑**。
- 步骤串并行：step 1 与 step 2 同时跑；step 3 等 step 1。**同一 `session_id` 同时只允许一个作业**；不同 `session_id` 在理论上可以并发（前端换过会话后再提交上一次的转码），step 3 会在 `ue_ctx` 闩上自然串行——**这一点不额外加锁，靠闩即可**。
- **不做** target 二选一（早期草案的 `target:"ue"|"ffmpeg"` 已取消），请求体固定。

### 7.2 请求与状态

`POST /api/doodle/local/face_capture/transcode`

```json
{
  "session_id": "…",
  "ue_project": "E:/Doodle/UnrealEngine/MyProject.uproject",
  "package_path": "",
  "decode_max_width": 1280
}
```

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `session_id` | **是** | 要转码的会话 |
| `ue_project` | **是** | `.uproject` 的**绝对路径**。**不是绝对路径、为空、缺失、文件不存在，一律 `400 missing_ue_project`**；后端不做相对路径拼接（不拼 `core_set::get_set().ue4_path`），也不回退到任何默认工程——脸捕是"在某台机器上用某个工程跑"，路径只能由调用方给出 |
| `package_path` | 否 | UE 包路径（不是磁盘路径）。**空字符串或不传 → 命令行里根本不出现 `-PackagePath`**，落点由 Commandlet 自己决定，后端不替它填默认值。传了则必须 `/Game/...` 且末段不含 `.`，否则 `400 bad_package_path` |
| `decode_max_width` | 否 | `raw.mkv → frames/` 的目标长边，默认 1280，用于控制 PNG 体积 |

- **请求体里没有 `fps`**：帧率固定 25（§4.4），转码也一律按 25 解 25 出，前端无从选择、也不需要选择。
- **没有 `target` 字段**，也**没有"失败时保留 frames"开关**：保留策略固定（§7.5），失败天然保留、成功必删。
- 两个 400 都在**提交时**返回（不是等到 UE 进程启动），前端拿到 body 直接展示即可。

`GET .../transcode/{job_id}`

```json
{
  "job_id": "…",
  "session_id": "…",
  "state": "running",
  "steps": [
    {"name":"png_sequence","state":"succeeded","progress":100,"detail":"3684 frames"},
    {"name":"mp4","state":"succeeded","progress":100,"detail":"take.mp4 122.8s"},
    {"name":"ue_commandlet","state":"running","progress":35,"detail":"solving"}
  ],
  "ue": {"exit_code":0,"log_path":"…/face_capture_log.json","curves_path":"…/face_curves.dhfc"},
  "error": null
}
```

- 进度：`png_sequence` 0–40%，`mp4` 0–20%，`ue_commandlet` 40–100%。**明确标注为粗粒度**，前端不得用于精确 ETA。
- **作业状态不进 `server_task_info`（拍板）**：转码作业是一次本机短作业，**不写数据库、不登记任务表、不出现在任务列表 UI 里**，只在内存注册表里保活；服务重启即丢失，产物文件仍在磁盘上可被下次扫描发现。要查进度只能走 `GET /transcode/{job_id}` 或 WS 的 `transcode_progress`。
- `run_time_info_t` 仍会传给 `async_run_ue`（它要这个入参收集起止时间），但**该对象只用于进程耗时统计，不落库**。

### 7.3 调用 Commandlet 的固定写法

```cpp
co_await async_run_ue(
    {l_uproject_path.generic_string(),
     "-Unattended", "-log", "-AllowStdOutLogVerbosity", "-ForceLogFlush",
     "-run=MetaHumanFace",
     fmt::format(R"(-FramesDir="{}")", l_frames_dir.generic_string()),
     fmt::format(R"(-OutDir="{}")",    l_curves_dir.generic_string()),
     fmt::format("-FPS={}",            l_fps),
     /* package_path 非空时才追加：*/ fmt::format("-PackagePath={}", l_package_path)},
    logger_ptr_, kitsu_client_, /*create_lock=*/true, l_run_time_info
);
```

**`-PackagePath` 是条件项**：请求里的 `package_path` 为空字符串（或不传）时，这一项**根本不拼进 `in_arg`**，由 Commandlet 用它自己的默认值；后端不写死、不猜、不补。拼参函数做成"传空就跳过"的形式，避免出现 `-PackagePath=` 这种空值参数被 Commandlet 当成非法包路径。

要点：

1. **不新写进程封装**。`async_run_ue`（[ue_exe.cpp:160-241](E:\Doodle\src\doodle_lib\exe_warp\ue_exe.cpp)）已经做了：UE 版本/插件安装 → 进程启动（`boost::process::v2`，读 stdout/stderr 管道）→ `timeout` 计时 → 结束后回捞 `<Project>/Saved/Logs` 最新日志 → 非 0 退出码抛 `doodle_error`。调用范例见 [import_and_render_ue.cpp:242-248](E:\Doodle\src\doodle_lib\exe_warp\import_and_render_ue.cpp)。
2. **`create_lock=true`**——与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`（`ue_ctx` 定义见 [ue_exe.h:26-31](E:\Doodle\src\doodle_lib\exe_warp\ue_exe.h)，闩本身是 [co_queue.h](E:\Doodle\src\doodle_lib\core\co_queue.h) 的 `awaitable_queue_limitation`，`limit` 默认 1）。**同一台机器上不允许两个 UE 进程并发**，脸捕排队在渲染任务后面是可接受的代价，换来零新增跨模块改动。
3. **`in_arg.front()` 必须是 `.uproject` 路径**：`async_run_ue` 用 `ue_exe_ns::find_ue_project_file(in_arg.front())` 回捞日志（`ue_exe.cpp:206`），顺序错了日志就丢。
4. **不能用 `-nullrhi`**：Commandlet 里求解走 `NNERuntimeORTDml`，但引擎资产加载要正常初始化（同 [metahuman_face_capture_solution.md §7.2](E:\Doodle\docs\metahuman_face_capture_solution.md) 的结论）。
5. **超时**：`async_run_ue` 用的是全局 `core_set::get_set().timeout`，脸捕耗时与帧数线性相关，可能不够。**本轮不改 `async_run_ue` 签名**；M0 实测出单帧耗时后，若 10 分钟素材会超时，再单独提"给 `async_run_ue` 加可选 timeout 参数"的改动（只加一个带默认值的形参，不改现有调用点）。
6. **`kitsu_client_` 用来做插件版本检查与安装**；脸捕作业没有 kitsu 上下文时传 `nullptr` 会在 `installUePath` 里解引用。因此作业在构造时从路由层拿到 `kitsu_client`（同 `local` 其它路由的做法：`std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip)` + `set_token(token_)`）。
7. **`-Unattended` / `-log` / `-AllowStdOutLogVerbosity` / `-ForceLogFlush`**：`-Unattended` 防 `FMessageDialog` 在无 UI 下把进程挂死（同一坑见 [metahuman_face_capture_solution.md §3.2](E:\Doodle\docs\metahuman_face_capture_solution.md)）；后三个让 `async_run_ue` 的管道读取能实时拿到日志，否则脸捕过程在作业日志里不可见。
8. **`.uproject` 由前端必填且必须是绝对路径**：脸捕是"某个人在某台机器上用某个工程跑"，工程路径不能从代码里猜、也不能从配置里猜。前端在 `POST /transcode` 的 body 里传 `ue_project`（**必填、绝对路径**），后端**只做两个校验**：`path::is_absolute()` 为真、该文件存在。**不拼 `ue4_path`、不做相对路径解析、不回退 `find_ue_project_file` 或任何默认工程**；任一校验不过，一律在提交时返回 `400 missing_ue_project`——猜错工程比报错更糟（用户会拿到别的工程的曲线）。
9. **`package_path` 由 Commandlet 决定**：请求里为空就不传 `-PackagePath`（见上面的条件项）。后端只做"非空时必须形如 `/Game/...` 且末段不含 `.`"的校验，不提供默认值。

### 7.4 结果解析

- 优先读 `curves/face_capture_log.json`（Commandlet 自己写，`MetaHumanFaceCommandlet.cpp:420-447`），字段：`stage / code / detail / curves_path / summary_path / log_path / frame_count / curve_count / elapsed_seconds / dropped_frame_ranges / missing_model_assets`（`MakeResultJson`，`MetaHumanFaceCommandlet.cpp:164-201`）。
- 退出码映射（`ToExitCode`，`MetaHumanFaceCommandlet.cpp:449-469`）：`0` 成功；`2` 参数/素材/CaptureData 问题；`3` 模型或推理后端缺失；`4` 其它求解失败；`5` 导出或写盘失败。
- `async_run_ue` 在非 0 退出码时抛异常，作业里 `catch (const doodle_error&)` 后**仍然去读 `face_capture_log.json`**（文件已落盘），把 `code/detail` 转成前端的 `error.code`。
- **`exit_code == 0` 不等于产物齐全**：必须同时校验 `curves_path` 存在且非空、`frame_count > 0`。
- **超时与"退出码"的混淆**：`async_run_ue` 触发全局 timeout 时同样抛异常且**进程仍在跑**。作业必须先 `boost::process` 终止该进程（或等待其自行退出），再去读日志，否则会出现"作业已失败但 UE 还在占着 `ue_ctx` 闩"的死循环。

### 7.5 清理策略（拍板：不可配置）

- step 3 成功 → **立即 `FSys::remove_all(session_dir / "frames")`**，保留 `raw.mkv`、`take.mp4`、`align.json`、`curves/`。删除动作失败只记警告，不影响作业返回成功（产物已齐）。
- 提供重跑入口：再次 `POST /transcode` 时若 `frames/` 已不存在，自动从 `raw.mkv` 重跑 step 1（不需要重录）。
- `/Game/.../<session_id>` 下的中间资产由 Commandlet 写（包路径为空时落点由它自己决定）；删除会话（`DELETE .../recordings/{session_id}`）时一并清理（尽力而为，失败只记日志）。
- **`raw.mkv` 永不自动删除**；磁盘水位策略由部署侧运维负责，方案里不引入自动淘汰。
- 本方案**不改变** `DeleteSession` 对 `frames/`、`curves/` 的处理口径：它们都在 `session_dir` 下，删会话即删全部。

---

## 8. 交给 UE 的数据硬约束

全部来自 [MetaHumanFaceCaptureDataBuilder.cpp](E:\Doodle\script\uePlug\Doodle\Source\DoodleMetaHumanFace\Private\MetaHumanFaceCaptureDataBuilder.cpp) 的 `ScanSequence()` 与 `BuildWithTimecode()`，**这是整条链路最容易出错的地方**。

### 8.1 文件与目录

| 约束 | 出处 | 后端的做法 |
| --- | --- | --- |
| 文件名必须匹配 `^(([a-zA-Z0-9]*[_\s-])*)([0-9]+)[a-zA-Z_\s-]*\.[a-zA-Z]+$` | `:20` | 固定 `frame_000000.png`（前缀 `frame_` + 下划线 + 6 位数字） |
| 数字位数必须全序列一致 | `:212-222` | 固定 6 位；超过 999999 帧直接拒绝，**不自动加位** |
| 帧号必须从首帧起连续，不能有空洞 | `:233-258` | 补位策略 A（§10）保证连续 |
| 扩展名限 `bmp/dds/exr/jpg/jpeg/png` 且必须能被 ImgMedia 读 | `:287-298` | 只用 PNG |
| 目录里不能有别的图片文件 | `:148-156` 会扫描所有图片扩展名 | `frames/` 下**只放 PNG**，任何 meta 文件放 `curves/` 或 `session_dir` 根 |
| `-FramesDir` / `-OutDir` 必须是绝对路径 | `MetaHumanFaceCommandLine.cpp:234-246` | 传 `FSys::path::generic_string()` 的绝对路径并整体加引号 |
| `-PackagePath` 必须是 `/Game/...` 形式的包路径（不得含 `.`） | `MetaHumanFaceCommandLine.cpp:70-83` | **请求给了就校验**（非空必须是合法包路径，否则 `400 bad_package_path`）；**请求没给就不传这个参数**，落点由 Commandlet 决定（§7.2、§7.3） |
| ImgMedia 侧还要能解析出同一组帧 | `:487-512` `FTrackingPathUtils::GetTrackingFilePathAndInfo` 的 offset / 帧数必须与扫描结果一致 | M0 真机验证项（§11） |
| **不传 `-SequencePattern`** | `MetaHumanFaceCommandLine.cpp:189-193` 把它解析出 `MaxTokenLen`，`:256-261` 只校验"传了但不含 `{frame}`" | 省略；文件命名固定为 `frame_000000.png` |

### 8.2 帧率与时间基准

- **`-FPS=25/1`，写死。** Commandlet 要的是 `<num>/<den>` 分数形式（`MetaHumanFaceCommandLine.cpp:201-210`），但后端不需要做"帧率 → 分数"的换算：全链路只有 25 fps（§4.4），常量字符串直接拼进命令行。
- **必须在 `curves/`（即 `-OutDir`）写 `frames_meta.json`，字段 `first_frame`**（`ResolveStartTimecode`，`MetaHumanFaceCaptureDataBuilder.cpp:312-340`）。否则 Commandlet 退回用 `StartFrame`，再退回墙钟，时间码就是随机值。后端写入 `{"first_frame": n}`，`n` 默认为 0。
- 每帧对应的时长是常量 `40 ms`（`1/25`），`frames/` 里第 `n` 张对应 `n × 40 ms`；补位后的序列自然满足这个映射。
- `-Range=<first>:<count>`：全量转码**不传**（不传等价 `FrameCount == 0` = 全序列，`:398`）。

### 8.3 PNG 编码细节

- 输出 `RGB24` 且**必须正立**，色序口径见 §4.3（不要二次 `cvtColor`）。ffmpeg 侧固定 `-pix_fmt rgb24`，`png_sequence_writer` 侧不额外转换；**实现时把这一项列进 M4 验收**（与 `take.mp4` 同 seq 逐像素比对，平均差 > 2 即失败）。
- PNG 压缩级别取 3（OpenCV 侧 `cv::IMWRITE_PNG_COMPRESSION`，体积与速度平衡），不做 alpha，不做 16 位。

### 8.4 目录里的"其它文件"

`ScanSequence`（`MetaHumanFaceCaptureDataBuilder.cpp:148-156`）会扫描 `frames/` 下**所有**图片扩展名，非本序列命名的图片会导致 `FramesFormatMismatch`。因此：

- `frames/` 里除 `frame_XXXXXX.png` 之外**不允许任何文件**（连 `.tmp`、`.DS_Store`、缩略图都不行）；
- 写 PNG 时先写 `<session_dir>/png_tmp/`，全部写完并自检通过后**一次性重命名整个目录为 `frames/`**，避免 UE 读到半成品或被扫描时撞见临时文件；
- 若 `frames/` 已存在（重跑），先删再建，不做增量覆盖。

---

## 9. 产物目录

```
<core_set cache root>/face_capture/<session_id>/
    raw.mkv                    # 录制结束即固定，唯一权威来源，永不自动删除
    align.json                 # seq / host_pts / 丢帧区间 / 实际宽高（帧率恒为 25）
    take.mp4                   # 前端回放（step 2）
    png_tmp/                   # step 1 的落盘暂存，自检通过后整体改名为 frames/
    frames/                    # step 1 产出 → UE 输入；step 3 成功后删除（§8.4）
        frame_000000.png …
    curves/                    # step 3 产出
        face_curves.dhfc       # 主交付（格式见 metahuman_face_capture_solution.md §5）
        face_curves.dhfc.json  # 人读摘要
        face_capture_log.json  # Commandlet 自写（§7.4）
        frames_meta.json       # 后端写（§8.2）
    logs/
        transcode.log          # 三步作业过程日志
```

`core_set::get_set().get_cache_root("face_capture")` 与 `movie.cpp` 用 `get_cache_root("http")` 同款。

体积预估（1280×720@25，1 分钟）：`raw.mkv` 约 0.4–0.8 GB、`frames/` 约 0.5–1.0 GB（PNG 逐帧，取决于画面细节）、`take.mp4` 约 12–21 MB。`frames/` 与 `raw.mkv` 同一量级，**所以"成功即删 `frames/`"不是优化而是必需**。

判读 `align.json` 的字段（实现时按此定结构体）：

```json
{
  "session_id": "…", "width": 1280, "height": 720, "fps": 25,
  "started_at": "2026-11-05T09:00:00+08:00", "duration_ms": 61200,
  "frames": [{"seq": 0, "host_pts_us": 0}],
  "gaps": [{"from_seq": 412, "to_seq": 419, "missing": 8}],
  "truncated": false
}
```

- **没有 `face` 字段**：人脸检测在二期（§6），一期不往权威记录里写任何检测结果。
- `gaps` 只在摄像头卡死（§4.4 的"连续 > 1 秒没有新帧"）时才会出现；正常情况下 `seq` 连续、`gaps` 为空。它是 step 1 补位的唯一依据。
- `frames` 只记 `{seq, host_pts_us}`；`pts` 不单独存——帧率固定为 25，`pts` 就是 `seq`。
- **`frames[].seq` 是连续递增的实际帧号**（两次 gap 之间没有洞），`gaps[].from_seq/to_seq` 标的是"这里本该有帧但没有"，`to_seq = from_seq + missing - 1`。所以 `frames[i].seq` 与 `raw.mkv` 第 i 个解码帧严格对应 —— 这是 `png_sequence_writer` 的消费依据（§10），**实现里绝不能因为记 gap 就推进 `seq`**。

---

## 10. 丢帧补位：策略 A（重复上一帧）

**拍板：只做 A，代码里不提供 B/C 分支。**

`missing` 的计算必须自洽，**不能依赖墙钟时长**（墙钟会引入假丢帧；帧率固定 25 之后连"帧率不稳"这个诱因都没了，`gaps` 只来自摄像头卡死）：

1. 用 `align.json` 的 `frames[].seq` 与 `gaps[].missing`：`expect_frames = len(frames) + Σ gaps.missing`（`frames` 里是**实际落进 `raw.mkv` 的帧**，`gaps` 是它们之间的空洞）。
2. 逐帧解码 `raw.mkv`（帧率 25，解码顺序即采集顺序），**第 i 个解码帧对应 `frames[i]`**（因为采集侧 gap 不推进 `seq`，见 §4.4，所以两者天然一一对应）；解码帧按顺序写 `frame_%06d.png`。**当上一张与下一张有空洞时**（即 `frames[i+1].seq > 写出的下一张编号`），就把**上一张 PNG 的字节直接复制**若干次，凑到 `frames[i+1].seq` 为止（不重新编码，省 CPU 与 IO）。
3. 若首帧就丢（理论上不该发生），复制第一个实到帧。
4. 写完后自检：`png_count == expect_frames`、编号从 000000 连续、位数为 6、目录内只有 `.png` —— 任一不满足即判 step 1 失败，**绝不把不合格的序列交给 UE**。

代价与理由：丢帧时刻画面静止，对 face solve 几乎无影响（那几帧的表情信息本来就丢了）；换来的是帧数、帧率、时间映射三者全对，Commandlet 不会因空洞失败。

---

## 11. 里程碑

| 里程碑 | 内容 | 验收 |
| --- | --- | --- |
| **M0（0.5 天，只验证不写产品代码）** | ①手搓一份 `frame_000000.png`（6 位数字）序列，真机跑 `-run=MetaHumanFace -FramesDir=<abs> -OutDir=<abs> -FPS=25/1`；②记录单帧求解耗时，判断 `core_set.timeout` 是否够；③确认 `curves/frames_meta.json` 的 `first_frame` 被采用；④确认不传 `-SequencePattern` 能通过；⑤确认 `FTrackingPathUtils` 接受该命名 | 五条结论写回本文 §8；任一条失败立即回到 §8 复核 |
| **M1（引擎部分已完成）** | `capture_session` + `media_fragment_pipeline`（仅推流）+ `ws_outlet`；**HTTP/WS 路由待做**：`face_capture_stream` WS + `POST/GET recordings` + 路由注册 + CMake | 引擎侧：编译通过 ✅；剩下浏览器里出现实时画面 + `curl -X POST .../recordings` 开录/收尾 |
| **M2（引擎部分已完成）** | `raw.mkv`（FFV1）+ `align.json` + 状态机 + `take.mp4`；**HTTP 层待做**：`POST/GET/stop` 端点 | 引擎侧：编译通过 ✅；剩下录完能得到 raw + align、可播放/下载 |
| M3 | 转码作业 step 1 + step 2（`png_sequence` 含策略 A、`mp4`）+ 作业查询/取消。**`write_png_sequence()` 已实现并编译通过**，等待最终作业调度层 | 不接 UE 也能产出合格 `frames/` 与 `take.mp4`，补位与命名校验生效 |
| M4 | 转码作业 step 3（Commandlet 编排 + 结果解析 + `frames/` 删除） | 端到端：录 → PNG → UE → `.dhfc` |
| M5 | 加固：状态机重入、`ws_outlet` 丢片调参、`ue_ctx` 排队观测、错误码统一 | 连续录 20 次无泄漏、无卡死 |

> 一期只有 M0–M5 六个阶段，没有人脸检测相关的里程碑。二期的检测工作另立方案，开工第一步是验 `objdetect` 能否编过（§6.2）。

---

## 12. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| **UE 侧命名/跟踪路径不匹配**（B 路线首风险） | `BuildWithTimecode` 直接 `CaptureDataBuildFailed`，表现为"素材错" | M0 先钉死；step 1 用 §10 的落盘前自检，不合格不提交 UE |
| `cv::VideoCapture` 阻塞 io 线程 | 整个 HTTP 服务卡死 | 只在采集 strand 上调用（§4.1） |
| WS 写并发 | beast 直接崩或数据错乱 | 单在途写标志 + `binary(true/false)` RAII（§3.3） |
| 页面挂起导致 WS 停读 | 写挂住、推流分片被丢 | 上限 8 片队列 + 丢非关键帧（§5.3）；**录制链不受影响**，丢的只是画面 |
| 摄像头型号差异（分辨率/帧率/MJPG 直出） | 请求的分辨率被静默改成别的 | 回读实际分辨率并上报（§4.3）；帧率不关心，由 25 fps 定时器抽帧/补帧（§4.4） |
| 摄像头卡死 / 驱动掉帧 | 录制里出现时长空洞 | 每拍补上一帧，超过 1 秒无新帧则记 `gaps` 交给转码补位（§4.4、§10） |
| 设备不支持 25 fps | 出帧节奏与 25 不匹配 | 不做任何协商：定时器 40 ms 一拍抽帧，静态补帧只影响流畅度不影响求解 |
| `raw.mkv` 体积巨大 | 磁盘撑爆 | 默认 720p、成功删 `frames/`、`raw.mkv` 不自动删但需要水位告警（运维） |
| `core_set.timeout` 不足以覆盖脸捕 | 长素材必失败 | M0 实测后决定是否为 `async_run_ue` 增加可选 timeout（§7.3-5） |
| 与渲染任务抢 `ue_ctx` 闩 | 脸捕要排队，前端进度长时间停在 40% | 进度里显式给出 `ue_commandlet: waiting_for_ue_lock` 子状态，别让前端以为卡死 |
| `async_run_ue` 超时后 UE 进程仍在跑 | 闩被占死，后续所有 UE 任务排队卡住 | 作业捕获超时异常后先终止该进程再返回（§7.4） |
| 路由类被 `clone()` | 成员生成两份，引擎状态错乱 | 会话放在 `capture_session::instance()` 单例上，路由对象不存会话状态（§3.2） |
| 记 `gaps` 时误推进 `seq` | `frames[].seq` 与 `raw.mkv` 包序错位，PNG 序列整体错位 | gap **只记录、不推进 `seq`**；`expect_frame_count` 负责把张数补齐（§4.4、§9） |

> 一期没有"模型缺失""opencv 不带 objdetect"这类风险——人脸检测整体在二期（§6）。

---

## 13. 验证方式

按仓库约束：**不写测试、不跑测试**，验证手段只有编译通过（`cmake_configure Ninja_debug` + `cmake_build debug_doodle`）。运行期验证只在 M0 里程碑里做一次真机冒烟，其余靠人工按 §3.2 的接口清单点一遍。

**当前状态**：采集引擎 6 组源文件已全部实现，`cmake_build(--preset debug_doodle)` **exit 0**（`doodle_lib.dll` / `test_main.exe` / `doodle_kitsu_supplement.exe` 全链接通过）。HTTP 与 WS 路由、转码作业层尚未开始。

**编译期踩过的坑**（写后续代码时照此避让）：
- `av::VideoFrame` **没有 `copyTo`**，取像素要用 `copyToBuffer(uint8_t*, size_t, align)`，且它是**非 const 成员** —— 所以取帧的函数参数要按值收 `av::VideoFrame`。
- `av::VideoRescaler::rescale()` 返回新帧，缩放这一步不能省（直接 `wrap` 原始帧会拿到未缩放的尺寸）。
- `boost/asio/make_strand.hpp` 不存在，`make_strand` 在 `boost/asio/strand.hpp`。
- beast 的流类型是 `boost::beast::tcp_stream`（头 `boost/beast/core/tcp_stream.hpp`），不是 `boost::asio::ip::tcp::tcp_stream`。
- beast websocket 写接口 `async_write_some(bool fin, ...)` 第一参必须传 `true`。

**编译期注意**：新增源文件必须同时登记到 `core/CMakeLists.txt`、`exe_warp/CMakeLists.txt`、`http_method/local/CMakeLists.txt` 三个 `target_sources(doodle_lib ...)` 里（现状见 [local/CMakeLists.txt](E:\Doodle\src\doodle_lib\http_method\local\CMakeLists.txt)：`PUBLIC` 只放 `.h`，`.cpp` 全在 `PRIVATE`）。漏登记表现为"链接时找不到符号"而不是语法错误。

---

## 14. 已拍板 / 仍未定

**已拍板（2026-11-05 / 11-06，不必再问）**：

1. 丢帧补位 = 策略 A（重复上一帧），代码里不提供 B/C 分支（§10）。
2. 脸捕 Commandlet 与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`（§7.3-2）。
3. `frames/` 在转码作业成功后**默认删除**、`raw.mkv` 保留，**不可配置**（§7.5）。
4. **推流只有一条**，不存在第二条，设计上也不为它留口子（§0 第 9/10 条、§5.3）。
5. **控制一律走 HTTP POST，WS 纯下行**（§0 第 7 条、§3.5）。
6. **人脸检测推迟到二期**，一期不实现、不预留字段（§0 第 12 条、§6）。
7. **转码作业不登记 `server_task_info`**：不写库、不进任务列表，只走 `GET /transcode/{job_id}` 与 WS 进度（§7.2）。
8. **`ue_project` 由前端在转码请求里必填，且必须是绝对路径**：缺失/为空/非绝对路径/文件不存在即 `400 missing_ue_project`；后端不拼相对路径、不猜默认工程（§7.2、§7.3-8）。
9. **`package_path` 可以为空，空则不传 `-PackagePath`**，由 Commandlet 自己决定落点；非空时后端只校验格式（§7.2、§7.3-9、§7.3 的条件拼参）。

**仍未定（不阻塞实现）**：

1. M0 真机跑一次 Commandlet 后确认：不传 `-PackagePath` 时它的实际默认落点是否稳定（决定前端要不要养成显式传的习惯），以及 `-PackagePath=` 空值是否会被它当非法参数（所以后端才做条件拼参，不拼空值）。

---

## 附录 A：改动面清单（实现时逐项勾掉）

| 文件 | 动作 | 状态 |
| --- | --- | --- |
| `src/doodle_lib/core/face_capture/*`（6 组 .h/.cpp：`face_capture_types.h`、`media_fragment_pipeline`、`ws_outlet`、`capture_session`、`frame_align`、`png_sequence_writer`） | 新增 | **已完成，编译通过** |
| `src/doodle_lib/core/CMakeLists.txt` | 登记新增源文件 | **已完成** |
| `src/doodle_lib/exe_warp/face_capture/*`（2 组 .h/.cpp） | 新增 | 未开始 |
| `src/doodle_lib/exe_warp/CMakeLists.txt` | 登记新增源文件 | 未开始 |
| `src/doodle_lib/http_method/local/face_capture.h` + 4 个 .cpp | 新增 | 未开始 |
| `src/doodle_lib/http_method/local/CMakeLists.txt` | 登记 `.h`(PUBLIC) 与 `.cpp`(PRIVATE) | 未开始 |
| `src/doodle_lib/http_method/kitsu.cpp` | `create_kitsu_local_route()` 追加 §3.1 的 10 条 `reg_c` | 未开始 |
| `src/doodle_core/metadata/kitsu_ctx_t.h` | **不改**（人脸检测在二期才加 `face_detector_model_path_`） | — |
| `script/uePlug/Doodle/**` | **不改** | — |
| `vcpkg.json` | **不改** | — |

## 附录 B：与既有模块的边界

| 既有模块 | 关系 |
| --- | --- |
| `doodle::ffmpeg_video` / `ffmpeg_video_resize` | **不复用**（文件到文件，无逐帧回调，自带字幕/水印/片头片尾逻辑） |
| `doodle::exe_warp::async_run_ue` | **直接复用**，新增作业只是它的调用方 |
| `core_set::get_set().ue_ctx_->queue_` | **共用**，不新开闩 |
| `socket_io::broadcast` / `/events` | 推流**不用**（单条推流不需要广播）；列表刷新等低频事件可选接入 |
| `server_task_info` / `run_long_task_local` | 转码作业**不登记**（不写库、不进任务列表）；`run_time_info_t` 仅作为 `async_run_ue` 的入参收集起止时间，不落库 |
| `doodle::ai::doodle_depth_estimation` / `get_ort_env()` | **不涉及**（一期没有任何推理；二期的 `cv::FaceDetectorYN` 也不走 `Ort::Env`） |
| `doodle::kitsu::kitsu_client` | 作业用它做 UE 插件版本检查（`async_run_ue` 内部所需），以及可选的产物上传 |

## 附录 C：删减记录（相对初稿）

| 初稿设计 | 现在 | 原因 |
| --- | --- | --- |
| `capture_session_registry`（`session_id` 查找表） | 删除，改为 `capture_session::instance()` 进程内单例 | 本机只有一个会话 |
| WS 按 `session_id` 分路径、多连接互斥 + `409 device_busy` | 路径保留占位但不参与查找；第二条连接接管，旧的 `superseded` | 摄像头只有一个消费者，不需要仲裁 |
| 多标签页：`control:"observer"` 观察者模式 | 删除 | 只有一条推流、一个控制者 |
| 每连接 `fragment_sink` + 环形缓冲 + 逐连接 FIFO(8 片) + `gap` 标记 | 上限 8 片队列 + 关键帧优先级覆盖 + 丢非关键帧，`gap` 字段删除 | 只有一个出口，不需要分发与回放补发 |
| `align.json` 里的 `dropped_by_sink` | 删除 | 推流丢片不影响录制，不写进权威记录 |
| `capture_start` 的 `fill_mode` 字段（只接受一个值） | 删除 | 策略 A 已拍板，协议里不留无效开关 |
| WS 上行命令（`capture_start` / `capture_stop` / `transcode_start` / `ping`） | 全部删除，改为 `POST /recordings`、`POST .../stop`、`POST /transcode` | 本机 loopback，请求-响应没有不确定性，没必要维护 ack/超时/重放（§3.5） |
| WS 读协程里的命令解析与 `post` 到 strand 的分派 | 删除，读端只用于断连检测 | 没有上行命令可解析 |
| WS 心跳 `ping/pong` | 删除，服务端不做空闲超时 | 单用户本机连接不该被服务端主动断开 |
| 单独的"兜底停止"REST 端点 | 合并为唯一的 `POST .../stop` | 控制面只有一条路，不存在"主路用完用兜底" |
| 帧率可配：`POST /recordings` 的 `fps`、转码的 `fps:"num/den"`、`align.json` 的 `fps_num/fps_den`、按实际出帧率设 `time_base` | 全部删除，全链路常量 25 fps；`align.json` 只留 `"fps": 25`，`raw_pts` 一列也删（`pts == seq`） | 帧率定死（§4.4），分子分母推理、实际帧率回读、`host_pts → pts` 换算全都不需要 |
| 丢帧判定的 `1.8 × 帧间隔` 自适应阈值 | 删除；采集成拍产出，`gaps` 只记录"连续 > 1 秒无新帧"的卡死区间 | 帧率固定后没有"帧率抖动"这个丢帧来源 |
| 编码链 `cvtColor(BGR→RGB)` 中转后交 `SwsConverter` | 直接 `AV_PIX_FMT_BGR24` → `av::VideoRescaler` → `YUV420P`；`frames/` 侧用 `cv::imencode` 而不是 `cv::imwrite`（便于先把 PNG 字节攒在内存里做补位复制） | 少一次全帧转换；两条链路同源同 swscale，色彩口径天然一致 |
| 人脸检测整块（`core/face_capture/face_detector.*`、`face_status` 事件、`hello` 的 `face_detector` 字段、`align.json` 的 `face`、`POST /recordings` 的 `422 no_face` 与 `force`、`kitsu_ctx_t::face_detector_model_path_`、M0' 预研与 M3 里程碑） | 全部推迟到二期，一期不做也不预留 | 一期目标是把"录 → 推 → PNG → UE 曲线"打通，检测是独立增量（§6） |
