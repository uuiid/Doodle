# 摄像头采集 · 人脸检测 · WebSocket 推流 · 前端控制录制 · UE 转码 方案

> 状态：方案已拍板（2026-11-05），待实现。本文只描述设计，不含实现代码。
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
| 6 | 会话与连接的绑定 | 会话挂在 `capture_session_registry`（按 `session_id`），**不挂 WebSocket 连接**；WS 断开不影响录制 |
| 7 | 控制通道 | 录制控制走 WS 命令；REST 只保留设备枚举、转码提交/查询、产物下载、兜底停止 |
| 8 | UE 侧源码变更 | **0**。前提是 M0 真机验证通过（§11） |

---

## 1. 端到端链路

```
浏览器 ──WS(wss)──► 上行: capture_start / capture_stop / transcode_start / ping
                    下行: avc_init / avc_fragment / face_status / capture_state / transcode_progress / error
                       │
        ┌──────────────┴───────────────┐
        │  后端 rt_loop_strand (单 strand) │
        │  cv::VideoCapture 取帧           │
        │  face_detector 检测(降频)         │
        │  ├─► raw_sink   ───────────────► raw.mkv     (FFV1 无损，唯一权威来源)
        │  ├─► h264_encoder(libx264) ────► 分片 writer ─► 浏览器 fMP4
        │  └─► align_recorder ───────────► align.json  (seq / pts / 丢帧区间 / 人脸统计)
        └───────────────────────────────┘
                       │ 前端点"结束录制" → 得到 raw.mkv
                       ▼ 前端再点"转码"（独立动作）
            转码作业 transcode_job（后台协程）
              step 1  png_sequence : raw.mkv ──► png_tmp/ → frames/frame_000000.png …   (补位策略 A)
              step 2  mp4          : raw.mkv ──► take.mp4                                (前端回放)
              step 3  ue_commandlet: frames/  ──► curves/*.dhfc  (async_run_ue, 共用 ue 闩)
                                  └──────────► 成功即删除 frames/
```

关键点：**录制与转码是两条独立的协程链**。录制在 `rt_loop_strand` 上逐帧串行推进；转码在 `g_io_context()` 上用另一条协程，两者只通过 `session_dir` 下的文件交互，不共享内存对象。

一句话总括：`opencv 采集 → raw.mkv（FFV1 无损，唯一权威来源）→ ①png_sequence → frames/ → UE Commandlet → face_curves.dhfc`，`②mp4 → 浏览器回放`，`③H.264 分片 → WebSocket → 实时预览`。

---

## 2. 模块与文件划分

### 2.1 目录布局

`src/doodle_lib/http_method/local` 是 HTTP/WS 入口层，依赖必须下沉到 `core`/`exe_warp`。因此：

```text
src/doodle_lib/core/face_capture/          ← 新增：与 HTTP 无关的采集/编码引擎
    face_capture_types.h                    公共结构体、enums、错误码、to_json/from_json
    media_fragment_pipeline.h/.cpp          ffmpeg 编码 + fMP4 分片产出 + raw_sink（无损）
    face_detector.h/.cpp                    YuNet 封装（cv::FaceDetectorYN，模型来自 kitsu_ctx 资产根）
    capture_session.h/.cpp                  会话状态机 + 采集循环 + 产物目录管理
    capture_session_registry.h/.cpp         session_id ↔ 会话，设备互斥锁
    frame_align.h/.cpp                      align.json 记录与读取
    png_sequence_writer.h/.cpp              raw.mkv → frames/（含补位策略 A）

src/doodle_lib/exe_warp/face_capture/      ← 新增：转码作业与 UE 编排
    face_transcode_job.h/.cpp               三步作业调度、状态机、进度
    face_ue_commandlet.h/.cpp               拼 Commandlet 参数、调 async_run_ue、解析退出码与 face_capture_log.json

src/doodle_lib/http_method/local/          ← 新增：路由
    face_capture.h                          DOODLE_HTTP_FUN_* 声明（全部路由类集中在此，与 local.h 同构）
    face_capture_device.cpp                 设备枚举
    face_capture_stream.cpp                 WS 推流与命令处理
    face_capture_record.cpp                 录制开始/停止/列表/查询/下载/删除
    face_capture_transcode.cpp              转码提交/查询/取消
    CMakeLists.txt                          （改）追加上述 .cpp
```

`face_capture.h` 与既有 `local.h` **分开**：`local.h` 已经有 171 行、18 个路由类，再塞 9 个会失控；`kitsu.cpp` 里加一行 `#include <doodle_lib/http_method/local/face_capture.h>` 即可。

改动的既有文件只有四个：`http_method/local/CMakeLists.txt`、`http_method/kitsu.cpp`（注册表）、`core/CMakeLists.txt`、`exe_warp/CMakeLists.txt`。**UE 插件与 `vcpkg.json` 都不动**。

### 2.2 命名空间

统一 `doodle::face_capture`（引擎层）与 `doodle::exe_warp::face_capture`（作业层），路由层仍是 `doodle::http::local`。

---

## 3. HTTP 与 WebSocket 路由

### 3.1 注册方式

沿用现有写法，在 `create_kitsu_local_route()`（[kitsu.cpp:307-337](E:\Doodle\src\doodle_lib\http_method\kitsu.cpp)）末尾追加：

```cpp
.reg_c<local::face_capture_devices>()
.reg_c<local::face_capture_stream>()             // GET + Upgrade → WS
.reg_c<local::face_capture_recordings>()         // GET 列表
.reg_c<local::face_capture_recording>()          // GET / DELETE，{session_id_}
.reg_c<local::face_capture_recording_stop>()     // POST 兜底停止
.reg_c<local::face_capture_recording_video>()    // GET take.mp4
.reg_c<local::face_capture_recording_curves>()   // GET .dhfc
.reg_c<local::face_capture_transcode>()          // POST 提交
.reg_c<local::face_capture_transcode_instance>() // GET / DELETE，{job_id_}
```

路由模板必须**一个类一条 URL**（`DOODLE_HTTP_ROUTE` 只接受一个路径字面量），所以 `/recordings/{id}` 与 `/recordings/{id}/stop` 是两个类。`kitsu.cpp:312-333` 里每个排队项都是这个形态，照抄即可。

未注册的路由运行期不可达，这是新增 HTTP 功能唯一容易漏的一步。

### 3.2 接口清单

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/api/doodle/local/face_capture/devices` | 枚举摄像头：`{devices:[{index, name, backend, width, height, fps, formats[]}]}` |
| GET | `/api/doodle/local/face_capture/stream/{session_id}` | **WS 升级点**（`has_websocket() = true`，GET + `Upgrade` 才升级） |
| GET | `/api/doodle/local/face_capture/recordings` | 会话列表（含进行中） |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}` | 单会话状态、帧数、丢帧区间、人脸检出统计、产物清单 |
| POST | `/api/doodle/local/face_capture/recordings/{session_id}/stop` | **兜底停止**（WS 不可用时的 REST 通道） |
| DELETE | `/api/doodle/local/face_capture/recordings/{session_id}` | 删除会话目录与 `/Game/Doodle/FaceCapture/<id>` 备用包；进行中拒绝 |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}/video` | `take.mp4`（`http_body_file_t`，支持 Range） |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}/curves` | `face_curves.dhfc` 下载（`set_attachment()`） |
| POST | `/api/doodle/local/face_capture/transcode` | 提交转码作业 |
| GET | `/api/doodle/local/face_capture/transcode/{job_id}` | 查询作业（含三步子状态） |
| DELETE | `/api/doodle/local/face_capture/transcode/{job_id}` | 取消作业（正在跑 Commandlet 时置取消标志，不杀进程） |

路由类一律 `DOODLE_HTTP_FUN_C(xxx, local_http_fun)`（要拿登录 token）或 `DOODLE_HTTP_FUN_C(xxx, ::doodle::http::http_function)`（纯本机操作）。**注意 clone 语义**：`http_function.cpp:74` 每个请求都会 `clone()` 一次路由对象（`std::make_shared<Self>(*this)`），所以路由类的成员只能是可拷贝的值或 `shared_ptr`，**不能放 `cv::VideoCapture`、`FaceDetectorYN`、`std::unique_ptr`**。采集引擎必须放在 `capture_session_registry` 里，路由类只持有 `shared_ptr`。

> 已知既有写法：`local::actions_local_ai_session_ue`（[local.h:162-169](E:\Doodle\src\doodle_lib\http_method\local\local.h)）及 `task_instance::cancel_manager_` 都是「单例路由 + `shared_ptr<impl>`」的形态，本方案的 registry 与它们同构——路由对象本身**不做请求状态**，所有状态在 registry 上。

### 3.3 WS 协议

WS 升级线的关键事实（`http_session_data.cpp:157-181`）：只有 `GET` + `websocket::is_upgrade()` + `callback_->has_websocket()` 三者同时成立才升级；`websocket_init(session_data_ptr)` 在 `async_accept` **之前**同步调用，`websocket_callback(stream, session_data_ptr)` 在 accept **之后**调用且拿不到错误码——连接生命周期只能靠第一次 `async_read` 失败感知。

`face_capture_stream` 的约定：

- `websocket_init`：从 `in_handle->url_` 取 `session_id`（query 或 path）与 `token`（复用 `local_http_fun::parse_header` 的 cookie/Bearer 逻辑）；用 `in_handle->route_ptr_->get_function<face_capture_stream>()` 拿注册时的原型对象再取 registry，**不要依赖自身成员**（clone 后是副本）。
- `websocket_callback`：`co_spawn(g_io_context(), run(std::move(stream)), detached)`，协程内部自己 `async_read`/`async_write`。
- 写端必须**单在途**：`atomic_bool writing_`，与 `socket_io_websocket_core::async_write_websocket`（`websocket_impl.cpp:124-158`）同构。控制消息是 text JSON，视频分片是 binary，切换用 `stream.binary(true/false)` 的 RAII guard（同 `binary_data_guard`）。**一条 WS 上不能并发写**，这是 beast 的硬约束。
- 读端只有 `websocket_callback` 里那一条 `async_read` 循环，读到 text 帧就解析命令、`post` 到 `rt_loop_strand` 处理；**不要在 read 协程里 `co_await` 采集动作**，否则读循环被阻塞、`ping` 与 `capture_stop` 全部延迟。
- 降级：若客户端协商不到 `Sec-WebSocket-Protocol`，仍按普通 WS 走；诊断信息统一用 JSON 事件下发。

#### 分片装载格式

一个帧分片 = 一条 text 事件 + 紧随的一条 binary 消息（两帧，前端按到达顺序配对）。text 里的 `seq` 用于配对与丢弃：

1. 前端收到 `avc_init` → `SourceBuffer.appendBuffer(init)`；
2. 前端收到 `avc_fragment` → 等下一帧 binary → `appendBuffer(moof+mdat)`；
3. 若 binary 到来时已知的最大 `seq` 已前进（网页被挂起后追尾），**跳过该 binary 不 append**，直接等下一个 `keyframe=true` 的分片重新同步（`gap=true` 时前端必须 `remove()` 历史 buffer 再 append）。

服务端保证同一连接上 `seq` 严格递增；`seq` 与 `raw.mkv` 里的帧号通过 `align.json` 的 `seq` 字段一一对应（§4.4），这也是"录制到第几帧"的唯一口径。

#### 下行事件（text JSON）

| event | 时机 | 负载要点 |
| --- | --- | --- |
| `hello` | 连接建立 | `{session_id, state, server_seq, fps, width, height}` |
| `avc_init` | 编码器首帧或重连补发 | `{codec:"avc1.42E01E", init: base64}`（fMP4 的 `ftyp+moov`） |
| `avc_fragment` | 每个分片 | `{seq, dur_ms, keyframe, align_seq}`，紧随一个 binary 帧装 `moof+mdat` |
| `face_status` | 每次检测（默认 5 Hz） | `{t_ms, faces:[{score, x, y, w, h, landmarks[5]}], status:"ok"/"none"/"multi"/"low_score"}` |
| `capture_state` | 状态变化 | `{state:"idle"/"recording"/"stopping"/"stopped"/"error", session_id, start_pts, frames}` |
| `transcode_progress` | 转码阶段推进（≥1 Hz 节流） | `{job_id, step:"png_sequence"/"mp4"/"ue_commandlet", state, progress, detail}` |
| `error` | 任何失败 | `{code, message, session_id}`，`code` 取自统一错误码表 |

`transcode_progress` 仅在 WS 先用 `transcode_start` 提交作业的连接上推送；用 REST 提交的作业只能轮询 `GET .../transcode/{job_id}`。**同一条 `session_id` 的多条 WS 连接都会收到**（例如开了两个标签页）。

#### 上行命令（text JSON）

| command | 说明 |
| --- | --- |
| `{"cmd":"capture_start","width":1280,"height":720,"fps":30,"fill_mode":"duplicate_prev"}` | 开始录制；`fill_mode` 只接受 `duplicate_prev`（策略 A），其它值直接 `error: unsupported_fill_mode` |
| `{"cmd":"capture_stop"}` | 结束录制；回 `capture_state=stopping`，落在 `raw.mkv` 收尾后回 `stopped` |
| `{"cmd":"transcode_start","package_path":"/Game/Game/FaceCapture","fps":"30000/1001","ue_project":"…"}` | 触发三步作业（等价 `POST .../transcode`） |
| `{"cmd":"ping","t":123}` | 心跳，回 `{"event":"pong","t":123}` |

前端接收顺序保证：`avc_init` 一定在第一个 `avc_fragment` 之前；断线重连后由服务端重发 `avc_init`，客户端丢弃 `align_seq` 小于自身已解码最大值的分片。

#### 幂等与并发

- 同一 `session_id` 上重复 `capture_start`（已在 `recording`）：回 `capture_state=recording` 并在 `error` 里给 `already_recording`，**不重新开录**。
- `capture_stop` 在 `idle`/`stopped` 上调用：回当前状态 + `error: not_recording`。
- 一个 `session_id` 允许多条 WS 连接（多标签页），但**同一时刻只有一条连接可以发送命令**：后连的请求在 `hello` 里拿到 `control:"observer"`，只收不发；首条连接断开时控制权移交给最早观察者。这是在 `capture_session` 上放一个"当前控制连接 id"，不是新建一致性机制。

### 3.4 为什么不用 socket.io

`/events` 命名空间的 socket.io 广播（`socket_io::broadcast`）适合低频状态广播，但它的一次写只装一段 text 和若干 binary，没有背压、没有落帧策略，且广播是"所有连该命名空间的客户端"。视频分片必须做**每连接一条 FIFO + 有界队列 + 落后即丢非关键分片**，要在广播模板外自己控制。所以推流走独立 WS，`face_status` 之外的通用状态（比如列表页刷新）才可选地走 `/events` 广播。

---

## 4. 采集引擎

### 4.1 线程与执行器

- **一条 `boost::asio::strand`（`rt_loop_strand`）承载全部采集步骤**：取帧、检测、编码、写分片。`cv::VideoCapture::read()` 与 `avcodec_send_frame` 都是阻塞调用，必须 `post`/`co_spawn` 到 strand 上跑，**绝不能直接在 io 线程里同步阻塞**，否则整个 HTTP 服务卡住。
  - 代价要说清：strand 跑在共享的 `g_io_context()` 上，一次 `read()` 阻塞仍会占住一个 io 线程。`g_io_context()` 是多线程跑的，单个阻塞只会降低并发而非死锁；但**如果实现时发现 `g_io_context()` 线程数过少**，应改为一条**专属 `boost::asio::io_context` + 1 线程**同时跑采集与分片，HTTP/WS 只做投递与写。—— M1 开工时先确认 `app_base` 的线程数，再定这一条。
- 一帧一拍：`async_wait` 一个 `steady_timer`（按目标帧间隔，用绝对时间累加避免漂移）→ 取帧 → 分发 → 下一拍。
- 停止时 `asio::cancellation_signal` 取消定时器，协程走清理路径；清理必须幂等。
- 本轮**不采集音频**，`raw.mkv` 不带音轨。**注意**：UE 侧 `bSkipTongueSolve` 默认就是 `true`（`MetaHumanFaceTypes.h:51`），所以无音轨与 UE 的默认参数天然一致，不需要传任何额外开关。

### 4.2 会话状态机

```
idle ──capture_start──► recording ──capture_stop/断流/错误──► stopping ──收尾完成──► stopped
                              │                                   │
                              └── cancel/析构 ────────────────────► stopped(partial)
```

- 一个进程同一时刻**只允许一条 `recording`**（摄像头物理互斥）。第二路 `capture_start` 返回 `409 device_busy`，附带占用它的 `session_id`。
- `session_id` 在 GET WS 时由 `capture_session_registry` 创建（`uuid`），`session_dir = core_set::get_set().get_cache_root("face_capture") / session_id_str`。
- WS 断开**不改变状态**；`recording` 期间允许重连（同 `session_id` 进入同一会话）。

### 4.3 取帧与像素约定

- `cv::VideoCapture(index, cv::CAP_DSHOW)`（Windows 首选 DSHOW，失败回退 MSMF；DirectShow 更容易设置分辨率/帧率并在某些设备上更稳）。
- 请求 `CAP_PROP_FRAME_WIDTH/HEIGHT/FPS` 后**必须回读实际值**，以实际值为准上报给前端和 UE 参数。
- OpenCV 输出 BGR。**三条色彩口径，都要写死在实现里**：
  - **推流 / `raw.mkv`（编码链）**：`cv::cvtColor(BGR→RGB)` → `SwsConverter` → `AV_PIX_FMT_YUV420P`。
  - **`frames/`（step 1）**：ffmpeg 解码 `raw.mkv` → `RGB24` `cv::Mat`（**不要**再 `cvtColor`）→ `cv::imwrite`（`imwrite` 自己会把它当 RGB 写 PNG）。
  - **颜色来源**：ffmpeg 的 swscale 默认按 `SWS_BICUBIC` 做 BGR↔YUV，与 OpenCV 的默认 `BGR2YUV`（BT.601 有限范围）可能产生 ±2 级别差。**M4 验收方式**：`take.mp4` 解码一帧与 `frames/` 同 **seq** 的 PNG 逐像素比较，平均色差 > 2 即视为口径错；修复方向是统一两端 swscale 的 `colorspace/color_primaries/color_trc`（都设成 `bt709` + `tv` range）。
- 正立与 sRGB 是 UE 求解链的硬要求（`FHyprsenseRealtimeNode` 只在正立画面上工作），摄像头驱动一般不做旋转，**画面翻转只允许在前端做，不进后端**。
- 像素格式统一到 `AV_PIX_FMT_YUV420P`（H.264 通用选择）；奇数分辨率会被 libx264 拒绝，`capture_start` 时把宽高向下取偶。

### 4.4 帧率与时间基准

- 摄像头出帧率不稳定：目标 30 fps 实际可能是 29.9/30.1。**编码器只接受 `time_base = 1/fps_num` 的单调 pts**，用 `pts = 第 n 个实际送出的帧`（帧序号），`time_base = {fps_num, fps_den}`（例：`25/1`、`30000/1001`）。
- `align.json` 记录两套时间：`seq`（帧序号）与 `host_pts`（`std::chrono::steady_clock` 采集时刻，微秒），用于换算真实时长与丢帧。
- 丢帧判定：相邻两帧 `host_pts` 差值 > `1.8 × 帧间隔` 就认为中间有丢帧，记录区间 `{from_seq, to_seq, missing}`。

---

## 5. 编码与推流

### 5.1 编码器

| 项 | 取值 | 理由 |
| --- | --- | --- |
| 编码库 | `libx264`（`av::findEncodingCodec("libx264")`，已有同款写法见 [ffmpeg_video.cpp:78-86](E:\Doodle\src\doodle_lib\core\ffmpeg_video.cpp)） | vcpkg 的 `ffmpeg[all-gpl]` 已带 |
| preset / tune | `veryfast` / `zerolatency` | 编码延迟 ≤ 1 帧 |
| profile / level | `baseline` 或 `main`，`3.1` | 浏览器兼容面最宽 |
| GOP | `keyint = fps`（1 秒）、`sc_threshold=0` | 重连等待 ≤ 1 秒 |
| rate control | `crf 23`（或 `bitrate 4M` 上限） | 局域网够用且画质稳定 |
| 像素格式 | `yuv420p` | 见 §4.3 |

编码器封装：`avcpp` 的 `av::VideoEncoderContext`，或直接 `libavcodec`（`ffmpeg_video.cpp` 两种都有先例）。**禁止**用 `cv::VideoWriter`——它无法产出分片容器。

> `ffmpeg_video` / `ffmpeg_video_resize` 是**文件到文件**的成品转码工具（输入 `FSys::path`、输出 `FSys::path`、`process()` 同步跑完），拿不到逐帧回调，**不能复用来做推流**；step 2 的 `take.mp4` 也建议新写一条轻量封装，而不是复用它的字幕/水印/片头片尾那一整套逻辑。

### 5.2 fMP4 分片

- 容器层用 `libavformat` 的 `mov`/`mp4` muxer，`AVFMT_FLAG_FLUSH_PACKETS` + `movflags` 组合：`frag_keyframe+empty_moov+default_base_moof`；
- **降级路径（M1 必须准备）**：若实测发现自定义 `AVIOContext` 无法稳定切出 `moof` 边界，改成**手工封装**——`libavcodec` 出 H.264 Annex-B packet，分片时把每个关键帧起始包转成 AVCC（`h264_mp4toannexb` 的反向），自己拼 `moov`（一次）与每片的 `moof`（`tfhd/tfdt/trun`）+ `mdat`。这条路完全可控、无 muxer 状态，代价是自己维护 `trun` 的 `sample_duration/sample_flags/composition_time_offset`。**实现时先按 muxer 方案做；仓库不写测试，M1 靠"浏览器里能播、切页回来能续"人工验收；一旦 `moof` 边界错乱（表现为 `MEDIA_ERR_SRC_NOT_SUPPORTED` 或画面花屏）立即切手工封装。**
- 分片边界取 `AV_PKT_FLAG_KEY`：每遇到关键帧 packet 就切一个片段（片段 = 一个关键帧 + 其后所有非关键帧），与 `frag_keyframe` 语义一致；
- 片段产出：`avio` 用**自定义写回调**（或内存 `AVIOContext`）截获 muxer 输出，每写完一个 fragment 就把 `(init_bytes_once, moof+mdat_bytes)` 交给分发器。**不要每帧重开 muxer**。
- `decode_pts` 换算成 `dts/pts` 的 `time_base` 与 §4.4 一致。

### 5.3 分发与背压

`media_fragment_pipeline` 内部持有一个**每连接**的 `fragment_sink`：

```text
fragment ─► [session 分片环形缓冲, 上限 N=120 片] ─► 每个 ws_sink 的 FIFO(上限 8 片)
```

- 新连接进入时先从环形缓冲里取最近一个**关键帧**开始补发（保证可解码），再进入实时流。
- 某个 sink 的 FIFO 满：丢**非关键帧分片**；连续丢到下一个关键帧若仍满，整段丢到下一个关键帧并把 `gap=true` 标进下一个 `avc_fragment`，让前端知道中间缺了。
- 环形缓冲与 sink 都只由 `rt_loop_strand` 写、WS 协程读，读写用 `boost::asio::experimental::concurrent_channel` 或"strand 内 mutex"；**不要裸 `std::queue` 跨线程**。
- 页面切后台导致 WS 停读：beast 的 `async_write` 会挂住，sink 的写入协程靠 timeout 与丢弃策略自救，禁止无界排队。

### 5.4 录制侧（raw.mkv）

- 与推流**同一批 `cv::Mat`**，走第二条编码器：`FFV1`（`AV_CODEC_ID_FFV1`，level 3，`yuv420p` 或 `bgr0`），封装 `matroska`。
- 为什么 FFV1 而不是 H.264：`raw.mkv` 是转码与 `frames/` 的唯一权威来源，必须**逐帧无损**，否则 PNG 序列里会有压缩痕迹，UE 求解精度受影响。
- 体积代价：1280×720@30 约 8–16 MB/s，1 分钟约 0.5–1 GB；1080p 再放大 4 倍。默认分辨率取 **1280×720@30**，`capture_start` 时可调；超时长（> 10 分钟，见 UE 侧"单段 ≤ 10 分钟"约束）在 `capture_state` 里给 `warning`，不强制中断。
- 落盘线程：`av_interleaved_write_frame` 在无损编码下是大块顺序写，放在 `rt_loop_strand` 上会和采集抢同一个 io 线程。用一条**专用写盘线程**（`boost::asio::thread_pool{1}`）+ 有界队列（上限 300 帧），队列满时**丢帧并记入 align.json**，绝不阻塞采集主循环。
- **坏帧与短尾**：`capture_stop` 到达时可能还有几帧在队列里，收尾必须先 `drain` 队列再 `av_write_trailer`；`stopping` 状态最长等 2 秒，超时直接 trailer 并记 `align.json` 的 `truncated: true`。

---

## 6. 人脸检测

### 6.1 选型

| 候选 | 结论 |
| --- | --- |
| **OpenCV `cv::FaceDetectorYN` + YuNet** | **采用**。OpenCV 4.10+ 自带，接口稳定，输入 320×320，单帧 ~2 ms CPU，5 点 landmark 可直接用于前端对齐提示 |
| onnxruntime 自接 SCRFD | 需要自己写 anchor 解码与 NMS，收益有限 |
| Windows MediaFoundation 人脸 | 精度低、不可控 |
| 前端 `FaceDetector` API | 只有 Chrome，且拿不到服务端判定 |

**前置风险（必须 M0' 先验）**：`vcpkg.json:76-85` 的 opencv 是 `default-features: false` + `contrib/freetype/openexr/tbb/ffmpeg`，`FaceDetectorYN` 属于 opencv 主模块的 `objdetect 4.x` 模块；`objdetect` 是 core 模块之一、通常随 `default-features: false` 保留，但**没在真机编译验证过**。M0' 的第一个动作就是写一行 `#include <opencv2/objdetect/face.hpp>` 编一次；编不过再考虑 `contrib` 里的 DNN 前向手写。**不要在没验证前把它写进主线里程碑**（故 M3 排在 M4/M5 之后）。

### 6.2 模型分发

- `vcpkg` 里没有任何 ONNX 模型文件，仓库内也没有（现有深度模型走 `kitsu_ctx_t::get_depth_model_path()`，即 `<root>/models/depth/da3mono_large.onnx`，见 [kitsu_ctx_t.h:32-38](E:\Doodle\src\doodle_core\metadata\kitsu_ctx_t.h)）。
- 因此**沿用同一套资产根**：新增 `kitsu_ctx_t::face_detector_model_path_{"models/face/face_detection_yunet_2023mar.onnx"}` + `get_face_detector_model_path()`，模型文件由部署侧放到 `<root>/models/face/`。**不要把模型打进二进制或 install 目录**。
- 模型缺失时：`/devices` 与 WS `hello` 里带 `face_detector:false`，采集照常进行，`face_status` 下发 `{status:"unavailable"}`。**人脸检测失败不阻塞录制**。

### 6.3 运行策略

- 检测**降频**：默认每 3 帧一次（10 fps @ 30），跑在 `rt_loop_strand` 上；若单次检测耗时 > 1/3 帧间隔，自动退化到每 6 帧并把 `face_status` 的频率如实上报。
- 检测分辨率：把帧 `resize` 到长边 640 再送检，边界框按比例映射回原尺寸。
- 多脸/无脸：`status` 取 `none` / `ok` / `multi`。**不因多脸中断录制**，只在 `capture_start` 时若首帧 `status != "ok"` 返回 `error: no_face`，需带 `{"force": true}` 才能继续（是否强制由前端策略决定，见 §14 待定项）。
- 线程模型：`cv::FaceDetectorYN` 内部走 DNN 前向，不需要 `Ort::Env`/`Ort::Session`（`get_ort_env()` 是给 `doodle_depth_estimation` 等另一批模型用的，**本方案不碰它**）。`FaceDetectorYN` 对象**不可拷贝**，放进 `std::shared_ptr<face_detector>`，只在 `rt_loop_strand` 上调用 `detect()`，销毁也在 strand 上完成。

---

## 7. 转码作业

### 7.1 三步流水（顺序固定）

| step | 输入 | 输出 | 实现 | 完成后 |
| --- | --- | --- | --- | --- |
| 1 `png_sequence` | `raw.mkv` | `frames/frame_{n}.png`，`n` 从 0 连续 | ffmpeg 解码（`-vsync 0`，逐帧不插值）+ 补位策略 A | 通知 step 3 可启动 |
| 2 `mp4` | `raw.mkv` | `take.mp4`（H.264 + AAC 若有音轨） | 与 step 1 并行，同一份 raw 只读 | 前端回放 |
| 3 `ue_commandlet` | `frames/` | `curves/` | `async_run_ue(...)`（§7.3） | 成功后删除 `frames/` |

> 前端的"结束录制"只负责关掉采集并落好 `raw.mkv`；转码是**另一次显式动作**（WS `transcode_start` 或 `POST .../transcode`）。两者不自动串联，避免用户在录完之前就被迫等待一次几分钟的 UE 求解。

- step 1 失败 → 整作业失败（UE 不可能跑）；step 2 失败 → 作业 `done_with_warning`（曲线已产出，只是没有 mp4）；step 3 失败 → 整作业失败，**保留 `frames/` 供人工重跑**。
- 步骤串并行：step 1 与 step 2 同时跑；step 3 等 step 1。**同一 `session_id` 同时只允许一个作业**；换 `session_id` 可以并发，但 step 3 会在 `ue_ctx` 闩上自然串行。
- **不做** target 二选一（早期草案的 `target:"ue"|"ffmpeg"` 已取消），请求体固定。

### 7.2 请求与状态

`POST /api/doodle/local/face_capture/transcode`

```json
{
  "session_id": "…",
  "fps": "30000/1001",
  "decode_max_width": 1280,
  "package_path": "/Game/Game/FaceCapture"
}
```

- `fps` 必须是 `num/den` 字符串（UE 侧只认这个形式，见 §8.2），省略时用采集时上报的帧率。
- `decode_max_width`：`from(raw.mkv) → scale` 的目标长边，默认 1280，用于控制 `frames/` 体积。
- `package_path` 是 **UE 包路径**，不是磁盘路径。默认留空则让 Commandlet 用它自己的默认值（`OutDir/Package`），但推荐显式传。
- **没有 `target` 字段**，也**没有"失败时保留 frames"开关**：保留策略固定（§7.5），失败天然保留、成功必删。

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
- 作业状态与 Doodle 现有长任务（`server_task_info`）**解耦**：它是一次本地短作业，不写数据库，只在内存注册表里保活；服务重启即丢失，产物文件仍在磁盘上可被下次扫描发现。

### 7.3 调用 Commandlet 的固定写法

```cpp
co_await async_run_ue(
    {l_uproject_path.generic_string(),
     "-Unattended", "-log", "-AllowStdOutLogVerbosity", "-ForceLogFlush",
     "-run=MetaHumanFace",
     fmt::format(R"(-FramesDir="{}")", l_frames_dir.generic_string()),
     fmt::format(R"(-OutDir="{}")",    l_curves_dir.generic_string()),
     fmt::format("-FPS={}/{}",         l_fps_num, l_fps_den),
     fmt::format("-PackagePath={}",    l_package_path)},
    logger_ptr_, kitsu_client_, /*create_lock=*/true, l_run_time_info
);
```

要点：

1. **不新写进程封装**。`async_run_ue`（[ue_exe.cpp:160-241](E:\Doodle\src\doodle_lib\exe_warp\ue_exe.cpp)）已经做了：UE 版本/插件安装 → 进程启动（`boost::process::v2`，读 stdout/stderr 管道）→ `timeout` 计时 → 结束后回捞 `<Project>/Saved/Logs` 最新日志 → 非 0 退出码抛 `doodle_error`。调用范例见 [import_and_render_ue.cpp:242-248](E:\Doodle\src\doodle_lib\exe_warp\import_and_render_ue.cpp)。
2. **`create_lock=true`**——与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`（`ue_ctx` 定义见 [ue_exe.h:26-31](E:\Doodle\src\doodle_lib\exe_warp\ue_exe.h)，闩本身是 [co_queue.h](E:\Doodle\src\doodle_lib\core\co_queue.h) 的 `awaitable_queue_limitation`，`limit` 默认 1）。**同一台机器上不允许两个 UE 进程并发**，脸捕排队在渲染任务后面是可接受的代价，换来零新增跨模块改动。
3. **`in_arg.front()` 必须是 `.uproject` 路径**：`async_run_ue` 用 `ue_exe_ns::find_ue_project_file(in_arg.front())` 回捞日志（`ue_exe.cpp:206`），顺序错了日志就丢。
4. **不能用 `-nullrhi`**：Commandlet 里求解走 `NNERuntimeORTDml`，但引擎资产加载要正常初始化（同 [metahuman_face_capture_solution.md §7.2](E:\Doodle\docs\metahuman_face_capture_solution.md) 的结论）。
5. **超时**：`async_run_ue` 用的是全局 `core_set::get_set().timeout`，脸捕耗时与帧数线性相关，可能不够。**本轮不改 `async_run_ue` 签名**；M0 实测出单帧耗时后，若 10 分钟素材会超时，再单独提"给 `async_run_ue` 加可选 timeout 参数"的改动（只加一个带默认值的形参，不改现有调用点）。
6. **`kitsu_client_` 用来做插件版本检查与安装**；脸捕作业没有 kitsu 上下文时传 `nullptr` 会在 `installUePath` 里解引用。因此作业在构造时从路由层拿到 `kitsu_client`（同 `local` 其它路由的做法：`std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip)` + `set_token(token_)`）。
7. **`-Unattended` / `-log` / `-AllowStdOutLogVerbosity` / `-ForceLogFlush`**：`-Unattended` 防 `FMessageDialog` 在无 UI 下把进程挂死（同一坑见 [metahuman_face_capture_solution.md §3.2](E:\Doodle\docs\metahuman_face_capture_solution.md)）；后三个让 `async_run_ue` 的管道读取能实时拿到日志，否则脸捕过程在作业日志里不可见。
8. **`.uproject` 的来源**：脸捕是"某个人在某台机器上用某个工程跑"，工程路径不能从代码里猜。取法与渲染任务一致——前端在 `transcode_start` 里传 `ue_project`（可选），后端按 `core_set::get_set().ue4_path` + 该工程的 `.uproject` 拼绝对路径；传空则用 `ue_exe_ns::find_ue_project_file(core_set::get_set().ue4_path)`。**若找不到 `.uproject`，作业在提交时就返回 `400 no_ue_project`，不要等到进程启动才炸。**

### 7.4 结果解析

- 优先读 `curves/face_capture_log.json`（Commandlet 自己写，`MetaHumanFaceCommandlet.cpp:420-447`），字段：`stage / code / detail / curves_path / summary_path / log_path / frame_count / curve_count / elapsed_seconds / dropped_frame_ranges / missing_model_assets`（`MakeResultJson`，`MetaHumanFaceCommandlet.cpp:164-201`）。
- 退出码映射（`ToExitCode`，`MetaHumanFaceCommandlet.cpp:449-469`）：`0` 成功；`2` 参数/素材/CaptureData 问题；`3` 模型或推理后端缺失；`4` 其它求解失败；`5` 导出或写盘失败。
- `async_run_ue` 在非 0 退出码时抛异常，作业里 `catch (const doodle_error&)` 后**仍然去读 `face_capture_log.json`**（文件已落盘），把 `code/detail` 转成前端的 `error.code`。
- **`exit_code == 0` 不等于产物齐全**：必须同时校验 `curves_path` 存在且非空、`frame_count > 0`。
- **超时与"退出码"的混淆**：`async_run_ue` 触发全局 timeout 时同样抛异常且**进程仍在跑**。作业必须先 `boost::process` 终止该进程（或等待其自行退出），再去读日志，否则会出现"作业已失败但 UE 还在占着 `ue_ctx` 闩"的死循环。

### 7.5 清理策略（拍板：不可配置）

- step 3 成功 → **立即 `FSys::remove_all(session_dir / "frames")`**，保留 `raw.mkv`、`take.mp4`、`align.json`、`curves/`。删除动作失败只记警告，不影响作业返回成功（产物已齐）。
- 提供重跑入口：再次 `POST /transcode` 时若 `frames/` 已不存在，自动从 `raw.mkv` 重跑 step 1（不需要重录）。
- `/Game/<package_path>/<session_id>` 下的中间资产由 Commandlet 写；删除会话（`DELETE .../recordings/{session_id}`）时一并清理（尽力而为，失败只记日志）。
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
| `-PackagePath` 必须是 `/Game/...` 形式的包路径（不得含 `.`） | `MetaHumanFaceCommandLine.cpp:70-83` | 由前端/配置给出，后端只校验 |
| ImgMedia 侧还要能解析出同一组帧 | `:487-512` `FTrackingPathUtils::GetTrackingFilePathAndInfo` 的 offset / 帧数必须与扫描结果一致 | M0 真机验证项（§11） |
| **不传 `-SequencePattern`** | `MetaHumanFaceCommandLine.cpp:189-193` 把它解析出 `MaxTokenLen`，`:256-261` 只校验"传了但不含 `{frame}`" | 省略；文件命名固定为 `frame_000000.png` |

### 8.2 帧率与时间基准

- `-FPS` **必须是 `<num>/<den>` 分数**（`MetaHumanFaceCommandLine.cpp:201-210`）：25 → `25/1`，29.97 → `30000/1001`，23.976 → `24000/1001`。**绝不能写 `29.97`**。
- **必须在 `curves/`（即 `-OutDir`）写 `frames_meta.json`，字段 `first_frame`**（`ResolveStartTimecode`，`MetaHumanFaceCaptureDataBuilder.cpp:312-340`）。否则 Commandlet 退回用 `StartFrame`，再退回墙钟，时间码就是随机值。后端写入 `{"first_frame": n}`，`n` 由采集起点换算（默认 0；若要接时间码，用 `round(start_seconds × fps)`）。
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
    align.json                 # seq / host_pts / 丢帧区间 / 人脸检出统计 / 实际宽高帧率
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

体积预估（1280×720@30，1 分钟）：`raw.mkv` 约 0.5–1 GB、`frames/` 约 0.6–1.2 GB（PNG 逐帧，取决于画面细节）、`take.mp4` 约 15–25 MB。`frames/` 与 `raw.mkv` 同一量级，**所以"成功即删 `frames/`"不是优化而是必需**。

判读 `align.json` 的字段（实现时按此定结构体）：

```json
{
  "session_id": "…", "width": 1280, "height": 720, "fps_num": 30, "fps_den": 1,
  "started_at": "2026-11-05T09:00:00+08:00", "duration_ms": 61234,
  "frames": [{"seq": 0, "host_pts_us": 0, "raw_pts": 0}, "…"],
  "gaps": [{"from_seq": 412, "to_seq": 419, "missing": 8}],
  "dropped_by_sink": 0,
  "truncated": false,
  "face": {"checked": 2048, "ok": 2010, "none": 30, "multi": 8}
}
```

`gaps` 是 step 1 补位的唯一依据；`raw_pts` 是 `raw.mkv` 里的真实帧序号，`seq` 是"应该是第几帧"，两者在对齐时等价、丢帧后开始分叉（`seq = raw_pts + 之前所有 missing`）。

---

## 10. 丢帧补位：策略 A（重复上一帧）

**拍板：只做 A，代码里不提供 B/C 分支。**

`missing` 的计算必须自洽，**不能依赖墙钟时长**（摄像头出帧率本来就不稳，墙钟会引入假丢帧）：

1. 用 `align.json` 的 `frames[].seq` 与 `gaps[].missing`：`expect_frames = len(frames) + Σ gaps.missing`（`frames` 里已经包含了补位前实际落盘的帧，`gaps` 是它们之间的空洞）。
2. 逐帧解码 `raw.mkv`，按 `frames[].seq` 的顺序写 `frame_%06d.png`；遇到 `gaps` 区间，就把**上一张 PNG 的字节直接复制 `missing` 次**（不重新编码，省 CPU 与 IO）。
3. 若首帧就丢（理论上不该发生），复制第一个实到帧。
4. 写完后自检：`png_count == expect_frames`、编号从 000000 连续、位数为 6、目录内只有 `.png` —— 任一不满足即判 step 1 失败，**绝不把不合格的序列交给 UE**。

代价与理由：丢帧时刻画面静止，对 face solve 几乎无影响（那几帧的表情信息本来就丢了）；换来的是帧数、帧率、时间映射三者全对，Commandlet 不会因空洞失败。

---

## 11. 里程碑

| 里程碑 | 内容 | 验收 |
| --- | --- | --- |
| **M0（0.5 天，只验证不写产品代码）** | ①手搓一份 `frame_000000.png`（6 位数字）序列，真机跑 `-run=MetaHumanFace -FramesDir=<abs> -OutDir=<abs> -FPS=25/1`；②记录单帧求解耗时，判断 `core_set.timeout` 是否够；③确认 `curves/frames_meta.json` 的 `first_frame` 被采用；④确认不传 `-SequencePattern` 能通过；⑤确认 `FTrackingPathUtils` 接受该命名 | 五条结论写回本文 §8；任一条失败立即回到 §8 复核 |
| **M0'（0.5 天，可行性预研，可与 M0 并行；不做则 §6 人脸检测整块降级为"不做"）** | ①确认当前 vcpkg 的 opencv 是否带 `objdetect`（`#include <opencv2/objdetect/face.hpp>` 编一次）；②用 `cv::FaceDetectorYN`（YuNet）在 640 长边上跑通一次检测，记录 CPU 单帧耗时；③把 `face_detection_yunet_2023mar.onnx` 放到 `<root>/models/face/` 验证能被加载 | 能编过且单帧耗时 < 8 ms（3 帧降频预算）、landmark 正常输出；任一条失败则 M3 直接取消，`face_status` 恒为 `unavailable` |
| M1 | `capture_session_registry` + `capture_session` + `media_fragment_pipeline`（仅推流）+ `face_capture_stream` WS + 路由注册 + CMake | 浏览器里出现实时画面，断线重连能续 |
| M2 | `raw.mkv`（FFV1）+ `align.json` + `capture_start/stop` 状态机 + `take.mp4` 回放下载 + 兜底 `stop` REST | 录完得到 raw + align，可播放/下载 |
| M3 | `face_detector`（YuNet）+ `face_status` 下行 + 模型缺失降级 | 前端看到人脸框与状态（M0' 未通过则跳过） |
| M4 | 转码作业 step 1 + step 2（`png_sequence` 含策略 A、`mp4`）+ 作业查询/取消 | 不接 UE 也能产出合格 `frames/` 与 `take.mp4`，补位与命名校验生效 |
| M5 | 转码作业 step 3（Commandlet 编排 + 结果解析 + `frames/` 删除） | 端到端：录 → PNG → UE → `.dhfc` |
| M6 | 加固：设备互斥、会话互斥、队列背压调参、`ue_ctx` 排队观测、错误码统一 | 连续录 20 次无泄漏、无卡死 |

> M1 之后建议**先做 M4/M5、后做 M3**：整条链路的真实风险在"PNG 序列能不能被 UE 吃下"，而不是人脸框。M4/M5 一旦跑通，M3 就只是画面上多一个框。

---

## 12. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| **UE 侧命名/跟踪路径不匹配**（B 路线首风险） | `BuildWithTimecode` 直接 `CaptureDataBuildFailed`，表现为"素材错" | M0 先钉死；step 1 用 §10 的落盘前自检，不合格不提交 UE |
| `cv::VideoCapture` 阻塞 io 线程 | 整个 HTTP 服务卡死 | 只在 `rt_loop_strand` 上调用（§4.1） |
| WS 写并发 | beast 直接崩或数据错乱 | 单在途写标志 + `binary(true/false)` RAII（§3.3） |
| 页面挂起导致 WS 停读 | 分片堆积吃内存 | 有界 FIFO + 丢非关键帧 + `gap` 标记（§5.3） |
| 摄像头型号差异（分辨率/帧率/MJPG 直出） | 请求值被静默改成别的 | 回读实际值并上报（§4.3） |
| `raw.mkv` 体积巨大 | 磁盘撑爆 | 默认 720p、成功删 `frames/`、`raw.mkv` 不自动删但需要水位告警（运维） |
| `core_set.timeout` 不足以覆盖脸捕 | 长素材必失败 | M0 实测后决定是否为 `async_run_ue` 增加可选 timeout（§7.3-5） |
| 与渲染任务抢 `ue_ctx` 闩 | 脸捕要排队，前端进度长时间停在 40% | 进度里显式给出 `ue_commandlet: waiting_for_ue_lock` 子状态，别让前端以为卡死 |
| `async_run_ue` 超时后 UE 进程仍在跑 | 闩被占死，后续所有 UE 任务排队卡住 | 作业捕获超时异常后先终止该进程再返回（§7.4） |
| vcpkg 的 opencv 不带 `objdetect` | `FaceDetectorYN` 编不过 | M0' 先验证；失败则整块不做，不影响主链路 |
| ONNX 模型缺失 | 人脸检测不可用 | 降级不阻塞录制（§6.2） |
| 路由类被 `clone()` | 成员生成两份，引擎状态错乱 | 路由只持 `shared_ptr`，引擎放 registry（§3.2） |

---

## 13. 验证方式

按仓库约束：**不写测试、不跑测试**，验证手段只有编译通过（`cmake_configure Ninja_debug` + `cmake_build debug_doodle`）。运行期验证只在 M0/M0' 里程碑里各做一次真机冒烟，其余靠人工按 §3.2 的接口清单点一遍。

**编译期注意**：新增源文件必须同时登记到 `core/CMakeLists.txt`、`exe_warp/CMakeLists.txt`、`http_method/local/CMakeLists.txt` 三个 `target_sources(doodle_lib ...)` 里（现状见 [local/CMakeLists.txt](E:\Doodle\src\doodle_lib\http_method\local\CMakeLists.txt)：`PUBLIC` 只放 `.h`，`.cpp` 全在 `PRIVATE`）。漏登记表现为"链接时找不到符号"而不是语法错误。

---

## 14. 已拍板 / 仍未定

**已拍板（2026-11-05，不必再问）**：

1. 丢帧补位 = 策略 A（重复上一帧），代码里不提供 B/C 分支（§10）。
2. 脸捕 Commandlet 与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`（§7.3-2）。
3. `frames/` 在转码作业成功后**默认删除**、`raw.mkv` 保留，**不可配置**（§7.5）。

**仍未定（不阻塞实现）**：

1. `capture_start` 是否强制"首帧检出单一人脸"才允许开始？当前设计是**默认强制 `status=="ok"`**，可通过 `{"force": true}` 强制绕过；若希望完全交给前端策略，把默认改成"只警告"。
2. `package_path` 的默认值：当前设计是"留空则由 Commandlet 使用 `OutDir/Package`"，推荐前端/配置显式传 `/Game/...`。等 M0 确认包路径的实际落盘形态后再定。
3. `ue_project`（§7.3-8）：渲染任务由 kitsu 任务下发工程路径，脸捕没有这个上下文。当前设计是"前端可选传 + 缺省用 `ue4_path` 下的 `.uproject`"。若前端不方便传，需要再定一个"全局默认工程"的配置项放置位置。
4. 是否需要把转码作业登记进 `server_task_info`（从而出现在现有的任务列表 UI 里）。当前设计是**不登记**（本机短作业）。若要登记，需要额外的数据库写入与广播，属于 M6 之后的增强。

---

## 附录 A：改动面清单（实现时逐项勾掉）

| 文件 | 动作 |
| --- | --- |
| `src/doodle_lib/core/face_capture/*`（8 组 .h/.cpp） | 新增 |
| `src/doodle_lib/core/CMakeLists.txt` | 登记新增源文件 |
| `src/doodle_lib/exe_warp/face_capture/*`（2 组 .h/.cpp） | 新增 |
| `src/doodle_lib/exe_warp/CMakeLists.txt` | 登记新增源文件 |
| `src/doodle_lib/http_method/local/face_capture.h` + 4 个 .cpp | 新增 |
| `src/doodle_lib/http_method/local/CMakeLists.txt` | 登记 `.h`(PUBLIC) 与 `.cpp`(PRIVATE) |
| `src/doodle_lib/http_method/kitsu.cpp` | `create_kitsu_local_route()` 追加 §3.1 的 9 条 `reg_c` |
| `src/doodle_core/metadata/kitsu_ctx_t.h` | 新增 `face_detector_model_path_` 与 `get_face_detector_model_path()` |
| `script/uePlug/Doodle/**` | **不改** |
| `vcpkg.json` | **不改** |

## 附录 B：与既有模块的边界

| 既有模块 | 关系 |
| --- | --- |
| `doodle::ffmpeg_video` / `ffmpeg_video_resize` | **不复用**（文件到文件，无逐帧回调，自带字幕/水印/片头片尾逻辑） |
| `doodle::exe_warp::async_run_ue` | **直接复用**，新增作业只是它的调用方 |
| `core_set::get_set().ue_ctx_->queue_` | **共用**，不新开闩 |
| `socket_io::broadcast` / `/events` | 推流**不用**；列表刷新等低频事件可选接入 |
| `server_task_info` / `run_long_task_local` | 转码作业**不登记**；`run_time_info_t` 仅作为 `async_run_ue` 的入参收集起止时间 |
| `doodle::ai::doodle_depth_estimation` / `get_ort_env()` | **不涉及**（人脸检测走 `cv::FaceDetectorYN`） |
| `doodle::kitsu::kitsu_client` | 作业用它做 UE 插件版本检查（`async_run_ue` 内部所需），以及可选的产物上传 |
