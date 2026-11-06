# 摄像头采集 · WebSocket 推流 · 前端控制录制 · UE 转码 方案

> 状态：方案已拍板（2026-11-05，11-06 追加"取消固定节拍"、"会话挂在 WS 上"、"选中摄像头"）。**采集引擎（M1/M2 引擎侧 + `write_png_sequence`）、WS 路由（含连上即开录、断开即收尾）、设备路由（GET 枚举 / POST 选中）已实现并编译通过**，HTTP 录制端点与转码作业层待实现（进度见附录 A）。
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
| 3 | 丢帧补位 | **已废止**：取消固定节拍后 `raw.mkv` 只存真实帧，`frames/` 不再需要补位（§10 保留为历史记录） |
| 4 | UE 进程排队 | 脸捕 Commandlet **与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`**（`awaitable_queue_limitation`，默认 limit=1），不新开闩 |
| 5 | `frames/` 的保留策略 | **转码作业成功后自动删除 PNG 序列、保留 `raw.mkv`；不可配置** |
| 6 | 会话与连接的绑定 | `capture_session` 是 `ws_stream_runner` 的**成员**，**会话生命周期 = WS 连接生命周期**：连上就打开摄像头开录，断开就停止。没有进程内单例，也不挂别的对象 |
| 7 | 控制通道 | **控制一律走 HTTP**：`POST /recordings` 重开一段录制、`POST /recordings/{id}/stop` 结束、`POST /transcode` 转码。**WS 是纯下行**，不接受任何命令（§3.5） |
| 8 | UE 侧源码变更 | **0**。前提是 M0 真机验证通过（§11） |
| 9 | **推流只有一条** | 本机后端、单用户，**任何时刻最多一条视频推流**。设计上**不提供第二条**：不做多连接分发、不做环形缓冲-history、不做逐连接背压（§5.3） |
| 10 | **同一时刻最多一个会话** | 会话对象只有一个，且它就是 `ws_stream_runner` 的成员，**不打开 WS 就不会抓摄像头**。不用查找表、不做会话池；第二条 WS 连上时旧连接因为失去 outlet 自然断开，新连接成为唯一会话（§3.3） |
| 11 | **输出帧率定死 25 fps** | 推流/`take.mp4`/UE `-FPS=25/1` 只用 25 fps，`time_base = 1/25`。**输入侧不再假设帧率**：摄像头到达即处理、带真实时间戳，由 `fps` 滤镜统一到 25 fps（§4.4）；`raw.mkv` 存真实帧、时基 `{1,1000}` |
| 12 | **人脸检测推迟到二期** | 一期**不做**：不写检测代码、不引模型、不预留 `face_status` 事件、不做开录前的人脸校验。§6 只记二期插入点与约束 |
| 13 | **转码作业不登记任务表** | **不写 `server_task_info`、不进任务列表 UI**，只在内存注册表保活；进度只看 `GET /transcode/{job_id}` 与 WS `transcode_progress`（§7.2） |
| 14 | **`ue_project` 必填且必须是绝对路径** | 前端在 `POST /transcode` 的 body 里必须给 `.uproject` 的**绝对路径**；缺失/为空/非绝对路径/文件不存在 → `400 missing_ue_project`。**后端不做相对路径拼接、不猜默认工程**（§7.2、§7.3-8） |
| 15 | **`package_path` 可空** | 空字符串或不传 → 命令行里**不出现 `-PackagePath`**，落点由 Commandlet 自己决定；非空才校验 `/Game/...` 格式（§7.2、§7.3-9、§8.1） |
| 16 | **取消固定节拍** | 采集改为"摄像头阻塞读线程 → 到达即打 `steady_clock` 时间戳 → `post` 到采集 strand"，**没有 `steady_timer` 轮询、没有 40 ms 拍点**；帧率统一交给 `fps` 滤镜（§4.1、§4.4） |
| 17 | **不打开 WS 就不抓摄像头** | `capture_session` 由 `ws_stream_runner` 直接持有并随连接创建/销毁；WS 连上即 `start()` 开录、断开即 `stop()` 收尾。**删除进程内单例 `capture_session::instance()`** |
| 18 | **摄像头由前端先选中，WS 只负责开流** | `GET /face_capture/devices` 枚举、`POST /face_capture/devices` 选中（body `{"index":n}`）；选中值是设备路由上的**共享属性**（`shared_ptr<impl>`，clone 后仍共享），通过 `reg_c` 构造参数注入 `face_capture_stream`，WS 连上时取它传给 `capture_session::start()`。不落盘，重启回 0（§3.1、§3.2） |

---

## 1. 端到端链路

```
控制面（前端 → 后端，全部 HTTP）
  POST /face_capture/recordings                    开始录制
  POST /face_capture/recordings/{id}/stop          结束录制
  POST /face_capture/transcode                     转码
  GET  /face_capture/recordings[/{id}]            状态查询（磁盘扫描，跨连接可用）

数据面（后端 → 前端，单向 WS，不接受任何命令）
  浏览器 ◄──WS── avc_init / avc_fragment / capture_state / transcode_progress / error
                       │
        ┌──────────────┴───────────────┐
        │  摄像头读线程 (阻塞 read)        │
        │  到达即打 steady_clock 时间戳    │
        │        │ post                  │
        │        ▼                       │
        │  后端采集 strand (单 strand)     │
        │  ├─► raw_sink   ───────────────► raw.mkv   (FFV1 无损, 真实帧, 时基 1/1000)
        │  ├─► fps 滤镜 ─► h264_encoder ─► 分片 ─► ws_outlet(≤8 片) ──► 浏览器 fMP4
        │  │            └► take.mp4                    (输出时基 1/25, pts=seq)
        │  └─► align_recorder ───────────► align.json (seq / wall_ms / gaps)
        └───────────────────────────────┘
                       │ 前端 POST stop → 200 返回时 raw.mkv 已收尾
                       ▼ 前端再 POST /transcode（独立动作）
            转码作业 transcode_job（后台协程）
              step 1  png_sequence : raw.mkv ──► png_tmp/ → frames/frame_000000.png …  (逐帧, 无补位)
              step 2  mp4          : raw.mkv ──► take.mp4                               (前端回放)
              step 3  ue_commandlet: frames/  ──► curves/*.dhfc  (-FPS=25/1, 共用 ue 闩)
                                  └──────────► 成功即删除 frames/
```

关键点四条：

1. **控制面全在 HTTP，数据面全在 WS**。WS 只往下推帧与事件，不解析任何上行命令（§3.5）。
2. **WS 是会话本体，不是可选通道**：连上 WS 才打开摄像头、才开录；断开就等于结束录制。`POST /recordings/{id}/stop` 只是"提前收尾"，不连 WS 时没有会话可控制。
3. **录制与转码是两条独立的协程链**。录制在采集 strand 上逐帧串行推进；转码在 `g_io_context()` 上用另一条协程，两者只通过 `session_dir` 下的文件交互，不共享内存对象。
4. **帧率只在出口处统一**。输入是"摄像头什么时候给就什么时候要"，带真实时间戳；`fps=25` 滤镜负责把任意输入帧率采样成 25 fps，供 `take.mp4` 与推流；`raw.mkv` 与 `frames/` 保存的是真实帧，不做任何复制（§4.4）。

一句话总括：`opencv 采集 → raw.mkv（FFV1 无损，唯一权威来源）→ ①png_sequence → frames/ → UE Commandlet → face_curves.dhfc`，`②mp4 → 浏览器回放`，`③H.264 分片 → WebSocket → 实时预览`。

---

## 2. 模块与文件划分

### 2.1 目录布局

`src/doodle_lib/http_method/local` 是 HTTP/WS 入口层，依赖必须下沉到 `core`/`exe_warp`。因此：

```text
src/doodle_lib/core/face_capture/          ← 新增：与 HTTP 无关的采集/编码引擎（已实现）
    face_capture_types.h                    公共结构体、enums、错误码、to_json/from_json
    media_fragment_pipeline.h/.cpp          ffmpeg 编码 + fMP4 分片产出 + raw_sink（无损）
    frame_filter.h/.cpp                     fps 滤镜包装：真实时间戳输入 → 25 fps 输出
    ws_outlet.h/.cpp                        单推流出口：单在途写、上限 8 片、丢非关键帧；send_* post 到 strand，查询走 atomic
    capture_session.h/.cpp                  会话状态机 + 摄像头读线程 + 采集处理（本机唯一的会话）
    frame_align.h/.cpp                      align.json 记录与读取
    png_sequence_writer.h/.cpp              raw.mkv → frames/（逐帧，无补位）
```

一期是 **7 组源文件**（`face_detector` 随人脸检测一起推到二期，§6）。`frame_filter` 自成一组是因为它只依赖 ffmpeg 的 `avfilter`，不关心 `capture_session` 的状态机；`ws_outlet` 独立成组是因为它既要被 `capture_session` 用，又要被 HTTP 层的 WS 协程持有，放在 `capture_session` 里会让两层互相包含。

WS 连接的生命周期（建 `ws_outlet` → attach → 开录 → 读循环 → 停录 → 断连清理）不进 core，而是以 `ws_stream_runner` 类的形式**就地写在** `http_method/local/face_capture_stream.cpp` 的 `doodle::http::local` 命名空间里，由 `face_capture.h` 前置声明（`class ws_stream_runner;`）供成员 `std::shared_ptr<ws_stream_runner> runner_` 使用。连接管理只对一个 HTTP 路由有意义，没有第二个消费者，下沉到 core 只会多一对要登记的 `.h/.cpp`。
src/doodle_lib/exe_warp/face_capture/      ← 新增：转码作业与 UE 编排
    face_transcode_job.h/.cpp               三步作业调度、状态机、进度
    face_ue_commandlet.h/.cpp               拼 Commandlet 参数、调 async_run_ue、解析退出码与 face_capture_log.json

src/doodle_lib/http_method/local/          ← 新增：路由
    face_capture.h                          DOODLE_HTTP_FUN_* 声明（全部路由类集中在此，与 local.h 同构）
    face_capture_device.cpp                 设备枚举 GET + 选中设备 POST
    face_capture_stream.cpp                 WS 下行推流（纯输出，无命令处理）
    face_capture_record.cpp                 录制：POST 开始 / POST 停止 / GET 列表 / GET 查询 / 下载 / DELETE
    face_capture_transcode.cpp              转码提交/查询/取消
    CMakeLists.txt                          （改）追加上述 .cpp
```

**没有 `capture_session_registry`**：单用户本机、同一时刻只有一个会话，注册表那一层是多余抽象。**也没有进程内单例**：`capture_session` 是 `ws_stream_runner` 的成员，跟着 WS 连接一起创建、一起销毁——不打开 WS 就不会抓摄像头（§0 第 17 条、§3.3）。

`local.h` 已经有 171 行、18 个路由类，再塞 10 个会失控；把新模块集中到 `face_capture.h` 与既有 `local.h` **分开**：`kitsu.cpp` 里加一行 `#include <doodle_lib/http_method/local/face_capture.h>` 即可。

改动的既有文件只有四个：`http_method/local/CMakeLists.txt`、`http_method/kitsu.cpp`（注册表）、`core/CMakeLists.txt`、`exe_warp/CMakeLists.txt`。**UE 插件与 `vcpkg.json` 都不动**。

### 2.2 命名空间

统一 `doodle::face_capture`（引擎层）与 `doodle::exe_warp::face_capture`（作业层），路由层仍是 `doodle::http::local`。

---

## 3. HTTP 与 WebSocket 路由

### 3.1 注册方式

沿用现有写法，在 `create_kitsu_local_route()`（[kitsu.cpp:307-337](E:\Doodle\src\doodle_lib\http_method\kitsu.cpp)）末尾追加：

```cpp
.reg_c<local::face_capture_devices>()            // GET 设备枚举 / POST 选中设备
.reg_c<local::face_capture_stream>(              // GET + Upgrade → WS（纯下行）
    l_rout_ptr->get_function<local::face_capture_devices>()->selected_)
.reg_c<local::face_capture_recordings>()         // POST 开始录制 / GET 列表
.reg_c<local::face_capture_recording>()          // GET / DELETE，{session_id_}
.reg_c<local::face_capture_recording_stop>()     // POST 结束录制
.reg_c<local::face_capture_recording_video>()    // GET take.mp4
.reg_c<local::face_capture_recording_curves>()   // GET .dhfc
.reg_c<local::face_capture_transcode>()          // POST 提交
.reg_c<local::face_capture_transcode_instance>() // GET / DELETE，{job_id_}
```

**持有会话对象的是 `local::face_capture_stream` 的 `ws_stream_runner`**（即 WS 连接本身）：`ws_stream_runner` 成员里有唯一的 `std::shared_ptr<capture_session> session_`，连上 `start()`、断开 `stop()`。**没有任何路由或单例长期持有会话**——不打开 WS 就不会打开摄像头（§0 第 17 条）。

**选中摄像头的共享属性**放在 `local::face_capture_devices` 上，字段是 `std::shared_ptr<face_capture_selected_device> selected_`；`face_capture_stream` 通过 `reg_c` 的构造参数拿到同一个 `shared_ptr`（`reg_t/reg_c` 会把参数原样透传给 `std::make_shared<T>`，见 [http_route.h:54-67](E:\Doodle\src\doodle_lib\http\http_route.h)）。**设备路由必须先注册**，否则 `get_function<>()` 取到空指针。

不放在 `core_set` 的理由：`create_kitsu_local_route()` 返回的 `http_route_ptr` 在 `kitsu_supplement.cpp:179` 只是局部变量，没挂到 `core_set`；为了一个"选中第几个摄像头"的整数去动 `core_set` 会多出一处跨模块耦合。

`face_capture_selected_device` 内部是 `std::shared_ptr<impl>`（`impl` 里 `std::mutex` + `std::int32_t index_`）：路由对象每请求 clone 一次，**值成员复制的是快照，写进去的东西下一个请求就丢**；`shared_ptr` 成员的副本指向同一个 `impl`，共享才成立。加锁是因为 HTTP handler 在 `g_ioc` 线程池上跑，而读它的 WS 协程在另一条 strand 上。

稳定性检查：**10 个路由类、12 个端点**（`devices` 一条 URL 两个方法）。（早先还考虑过把 `/stream` 拆成 open/close 两条，已否决——WS 的一生由 TCP 决定，不需要额外端点。）

路由模板必须**一个类一条 URL**（`DOODLE_HTTP_ROUTE` 只接受一个路径字面量），所以 `/recordings/{id}` 与 `/recordings/{id}/stop` 是两个类。`kitsu.cpp:312-333` 里每个排队项都是这个形态，照抄即可。

未注册的路由运行期不可达，这是新增 HTTP 功能唯一容易漏的一步。

### 3.2 接口清单

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/api/doodle/local/face_capture/devices` | 枚举摄像头：`{selected, devices:[{index, name, backend, width, height, selected}]}` |
| POST | `/api/doodle/local/face_capture/devices` | **选中摄像头**。body `{"index": 0}`；成功返回 `{selected}`；body 缺整数 `index` → `400`，负值 → `400`，设备不存在（越界/已拔出）→ `404` |
| GET | `/api/doodle/local/face_capture/stream/{session_id_}` | **WS 升级点 + 会话本体**（`has_websocket() = true`，GET + `Upgrade` 才升级）。连上即创建 `capture_session`、打开摄像头、开始录制并推流；断开即停止录制。路径里的 `session_id_` 只用于前端日志与路由占位，**不参与查找**，真实 `session_id` 在 `hello` 里回传。**这条只下发视频与状态，不接受任何命令**（§3.3） |
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

**控制面全部是 HTTP，WS 是纯下行且是会话本体**。前端不需要维护"命令发出去没回应"的重试逻辑：开始/结束录制都是一次 POST，成败直接看状态码与 body；WS 断了就意味着录制已经结束（`stop()` 在 `close_impl()` 里收尾）。

路由类一律 `DOODLE_HTTP_FUN_C(xxx, local_http_fun)`（要拿登录 token）或 `DOODLE_HTTP_FUN_C(xxx, ::doodle::http::http_function)`（纯本机操作）。**注意 clone 语义**：`http_function.cpp:74` 每个请求都会 `clone()` 一次路由对象（`std::make_shared<Self>(*this)`），所以路由类的成员只能是可拷贝的值或 `shared_ptr`，**不能放 `cv::VideoCapture`、`FaceDetectorYN`、`std::unique_ptr`**。

> **会话对象的归属**：会话由 `ws_stream_runner`（定义在 `http_method/local/face_capture_stream.cpp`）持有，成员是唯一的 `std::shared_ptr<capture_session> session_`；`stop` / `GET recordings` 等 HTTP handler 通过"当前唯一活跃会话"的弱引用登记拿它。**本机只有一个会话，所以不存在查找表。**

### 3.3 WS 协议

WS 升级线的关键事实（`http_session_data.cpp:157-181`）：只有 `GET` + `websocket::is_upgrade()` + `callback_->has_websocket()` 三者同时成立才升级；`websocket_init(session_data_ptr)` 在 `async_accept` **之前**同步调用，`websocket_callback(stream, session_data_ptr)` 在 accept **之后**调用且拿不到错误码——连接生命周期只能靠第一次 `async_read` 失败感知。

**实现已落地**：路由类 `local::face_capture_stream`（`http_method/local/face_capture.h` + `face_capture_stream.cpp`）只做转发，连接管理由同一个 `.cpp` 里的 `ws_stream_runner` 类承担：

- `websocket_callback` 里 `runner_ = std::make_shared<ws_stream_runner>(std::move(in_stream), in_handle->logger_, selected_->index()); runner_->start();`，别的一行不写。第三个参数是**连接建立那一刻**的选中设备，之后改选择不影响进行中的会话。
- `ws_stream_runner` 声明在 `face_capture_stream.cpp` 的 `namespace doodle::http::local` 下（**不能塞进匿名 namespace**，否则 `face_capture.h` 里前置声明的 `std::shared_ptr<ws_stream_runner> runner_` 指向的是另一个类型，编译报 C2027/C2039），`face_capture.h` 只前置声明。
- `ws_stream_runner` 自建 `make_strand(g_io_context())`，用同一个 strand 构造 `ws_outlet`——`send_*` 的 post 与 `drain()` 落在同一串行队列里，单在途写才成立。
- **`ws_stream_runner` 直接持有会话**：构造时 `session_ = std::make_shared<face_capture::capture_session>(face_capture::capture_session::default_root());`。会话不共享、不查表、不做单例——每条 WS 连接一个自己的会话。
- `run()` 的顺序是：建 outlet → `outlet->attach(write_fn)` → `session_->attach(outlet)` → `co_await start_capture()`（`session_->start(capture_option{.device_index_ = device_index_})`，`device_index_` 来自 POST devices 的选中值，从未选过就是 0；失败只记 WARN 不掐断连接）→ `co_await read_loop()` → `co_await close_impl()`。**先 attach 再 start** 是硬顺序：`start()` 建完会话目录就会开始出帧，此时 outlet 必须已经挂好。
- `close_impl()` 的顺序是：`open_=false` → `co_await stop_capture()`（`session_->stop()`，内部会等推流排空、收 `raw.mkv`、写 `align.json`）→ `outlet_->detach()` → `async_close`。**stop 在 detach 之前**，否则最后几帧没处可写。
- `write_fn` 签名是 `awaitable<bool>(bool in_binary, const uint8_t*, size_t)`：文本事件走 `binary(false)`，视频分片走 `binary(true)`，实现体是 `stream_.binary(x)` → `async_write_some(true, buffer)` → `stream_.binary(false)`。
- `co_spawn` 绑 `app_base::Get().on_cancel` 槽，进程取消时协程随之退出。

`face_capture_stream` 的约定：

- `websocket_init`：当前是空实现（不校验 token，见 §14 仍未定第 2 条）。路由成员里只有 `runner_`（`shared_ptr`，clone 安全），**不存会话**。
- **连上就开录**：`hello` 里的 `state` 是 `start()` 之后的状态（通常是 `recording`；摄像头打不开时是 `idle` 并在 `error` 事件里说明原因）。**没有"先连 WS 等 POST 再开录"这条路**。
- 接入顺序固定为 **先 `attach(write_fn)` → `session_->attach(outlet)` → `start_capture()`**。`hello` 不在连接建立时发：`start()` 建完会话后由采集侧驱动 `ws_outlet` 发 `hello`（带真实的 `session_id`/`width`/`height`/`state`），前端第一帧事件必然是 `hello`。
- **断开即结束录制**：`close_impl()` 先 `stop()` 再 `detach()`。前端要"结束但保留画面"就得自己重连（新连接 = 新会话、新 `session_id`）。
- 第二条连接 = 第二个会话：旧连接此时正卡在它自己的 `stop()` 里，摄像头被 `VideoCapture::release()` 唤醒收尾后，新连接才拿得到设备。**同一时刻仍然只有一条推流**（§0 第 9 条），但"接管"退化成"旧连接被关掉、新连接重开"。
- `websocket_callback`：`runner_ = std::make_shared<ws_stream_runner>(std::move(in_stream), in_handle->logger_); runner_->start();`，协程在 `runner_` 内部 `co_spawn`，自己 `async_read`/`async_write`。
- 写端由 `ws_outlet` 保证**单在途**（`drain()` 串行），不需要额外 `atomic_bool writing_`。
- `write_fn` 的实现体内直接 `co_await stream.async_write_some(true, buffer, use_awaitable)`（`ws_outlet` 保证单在途），不需要再套一层 `writing_` 标志位；首参 `in_binary` 决定 `stream.binary(true/false)`。
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

所有事件都是**纯下行**。`transcode_progress` 由后台作业广播到当前唯一的 WS；**WS 没连时没有会话也没有作业可跑**，所以不存在"事件丢了"这回事；跨连接的作业状态用 `GET /transcode/{job_id}` 查（`GET /recordings/{id}` 同理）。

#### 上行：什么都没有

WS 上**不定义任何上行命令**。前端不需要发 `ping`（浏览器原生 WS 没有心跳 API，beast 侧也不做空闲超时：单用户本机连接不该被服务端主动断开）；连接是否活着由浏览器自己感知，重连就是重新 `GET /stream/{session_id}`。

#### 控制面（HTTP）

| 动作 | 请求 | 成功返回 | 失败 |
| --- | --- | --- | --- |
| 开始录制 | 由连 WS 隐式触发；`POST /recordings` 只用于"主动重开"（断开重连后的显式请求） | `201 {ok, state:"recording", width, height, fps:25, frames:0, duration_ms:0}` + `session_id` | `409 already_recording`（`error_code:"already_recording"`）、`422 camera_open_failed`、`409 no_stream`（没有活跃 WS 会话） |
| 结束录制 | `POST /recordings/{session_id}/stop` | `200 {ok, state:"stopped", frames, raw_frames, inserted_frames, source_fps, duration_ms, truncated}` | `409 not_recording`、`404 unknown_session` |
| 查询状态 | `GET /recordings/{session_id}` | `{state, frames, gaps, artifacts, session_id, width, height, fps, raw_frames, source_fps}` | `404` |
| 提交转码 | `POST /transcode`，body 见 §7.2 | `202 {job_id, state:"queued"}` | `409 job_running`、`400 missing_ue_project`（`ue_project` 缺失/为空/非绝对路径/文件不存在）、`400 bad_package_path`（非空但不是合法包路径） |

要点：

- **录制由 WS 连接的开合决定**：`GET /stream/{session_id}` 一升级成功，`ws_stream_runner` 就 `session_->start()`（同步完成 `cv::VideoCapture` 打开、分辨率回读、编码器与 `raw.mkv` 的建立），连接一断就 `session_->stop()`。摄像头打不开时连接不断，只发 `error` 事件让前端看见原因。
- **`POST /recordings` 不是开录的必经之路**：它只是"同一连接上重开一段录制"的显式入口；没有活跃 WS 会话时返回 `409 no_stream`。
- **结束录制也是同步的**：等 `raw.mkv` 的 `av_write_trailer` 落完（上限 2 秒）再返回，前端拿到 `200` 就可以直接去拉 `/video` 或提交转码，不需要轮询等 `stopped`。断开 WS 走的是同一条 `stop()`，只是没人接返回值。
- 状态变化仍通过 WS 的 `capture_state` 推送一次，作为"另一条路也能看到"的冗余，但**前端应以 WS 事件与 POST 返回值为准**。
- 重复调用是幂等的：已在 `recording` 时再发 `POST /recordings` 返回 `409`；已 `stopped` 时再发 `stop` 返回 `409`。

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

- **一条 `boost::asio::strand`（`make_strand(g_io_context())`，实现里是 `capture_session::impl::strand_`）承载全部采集步骤**：收帧、`raw.mkv` 入队、`fps` 滤镜、编码、写分片、推流分发。`avcodec_send_frame` 是阻塞调用，必须 `post`/`co_spawn` 到 strand 上跑，**绝不能直接在 io 线程里同步阻塞**，否则整个 HTTP 服务卡住。
- **摄像头读帧在一条独立线程上**（实现里是 `capture_session::impl::read_thread_`，`std::thread`）。`cv::VideoCapture::read()` 会阻塞到有帧才返回，把它放在 strand 上会占死一个 io 线程；放独立线程后 strand 不阻塞，`fps` 滤镜与编码可以稳定运行。
  - 读线程只做两件事：`*cap_ >> mat`，然后 `boost::asio::post(strand_, [mat, now=steady_clock::now()]{ on_capture(mat, now); })`。**时间戳在读线程返回的一瞬间打**，这就是"这一帧到达的时刻"，是整条链路的有效时间。
- **"到达即处理"，没有固定节拍**：不再有 `steady_timer` 轮询、不再有 `next_tick`。摄像头出多少帧就 `post` 多少次，帧率差异全部交给 `fps` 滤镜（§4.4）。
- 停止路径：`stop()` 把 `state_` 置 `stopping` → `cap_->release()` 唤醒阻塞中的 `read()` → 读线程看到 `read_stop_` 退出并置 `read_finished_` → `stop()` 用 `steady_timer` 轮询（≤ 2 秒，`k_stop_drain_timeout_ms`）等它退出 → `filter_->flush()` 排空滤镜 → 等推流排空 → 收尾编码器 → 写 `align.json`。读线程 2 秒内退不出就记 `truncated` 并 `detach`（线程随进程结束），**绝不无限期阻塞 HTTP 请求**。
- 本轮**不采集音频**，`raw.mkv` 不带音轨。**注意**：UE 侧 `bSkipTongueSolve` 默认就是 `true`（`MetaHumanFaceTypes.h:51`），所以无音轨与 UE 的默认参数天然一致，不需要传任何额外开关。

### 4.2 会话状态机

```
idle ──WS 连上 / POST /recordings──► recording ──POST /stop、WS 断开、错误──► stopping ──收尾完成──► stopped
                                        │                                                    ▲
                                        └── 析构 / 下一个 start() ───────────────────────────┘
```

- `stopping` 是**短暂中间态**：`stop()` 把状态置成 `stopping` 并下发 `capture_state` 事件，然后**在同一个协程里**等推流排空（≤ 2 秒）→ 收尾三个编码器 → 写 `align.json` → 置 `stopped` 再发一次事件。所以前端看到 `stopping` 之后必定能等到 `stopped`，`POST .../stop` 返回时状态已经是 `stopped`。
- 等推空**必须用 `co_await timer.async_wait()` 轮询 `outlet_->idle()`**，不能用同步的 `timer.wait()`：`stop()` 与写协程在同一个 strand 上，同步阻塞会把 strand 占死，写协程永远抢不到执行权，结果是真的空等满 2 秒。
- 新一轮 `start()` 进来时若仍是 `recording`/`stopping` → `409 already_recording`（`error_code_t::already_recording`）；否则先 `close_resources()` 收拾上一个会话的残留，再建新的。
- **会话对象只有一个，且归 `ws_stream_runner` 所有**（§3.3）：不存在跨连接的会话对象，所以"设备互斥"退化成两层——进程层面由 `ws_stream_runner` 的持有关系保证，对象层面由状态机不许重入保证。
- `session_id` 在 `start()` 成功时生成（`uuid`），`session_dir = core_set::get_set().get_cache_root("face_capture") / session_id_str`。**每次 WS 连接都是一个新 `session_id`**。
- 同一个 `capture_session` 上 `stopped` 之后再 `start()`：**不换对象，只换 `session_id` 与 `session_dir`**。上一个会话的 `raw.mkv`/`align.json` 已经被落在旧目录里，不迁移、不覆盖。
- WS 断开**直接走 `stop()`**，与 `POST .../stop` 是同一条路径，只是返回值没人接。

### 4.3 取帧与像素约定

- `cv::VideoCapture(index, cv::CAP_DSHOW)`（Windows 首选 DSHOW，失败回退 MSMF；DirectShow 更容易设置分辨率/帧率并在某些设备上更稳）。
- 请求 `CAP_PROP_FRAME_WIDTH/HEIGHT` 后**必须回读实际值**；帧率**不回读也不采用**——一律按 §4.4 固定 25 fps。设备若不支持 25 会给出它自己最接近的帧率，由我们的定时器抽帧/补帧吃掉差异。
- OpenCV 输出 BGR。**三条色彩口径，都要写死在实现里**：
  - **推流 / `raw.mkv` / `take.mp4`（编码链）**：`cv::Mat` 是 BGR24，**直接**用 `av::VideoFrame::wrap(..., AV_PIX_FMT_BGR24, ...)` 交给 `av::VideoRescaler` → `AV_PIX_FMT_YUV420P`。**不做 `cvtColor(BGR→RGB)` 中转**——多一次全帧转换只是多花 CPU，`swscale` 本身就有 BGR24 输入。
  - **`frames/`（step 1）**：ffmpeg 解码 `raw.mkv` → `av::VideoRescaler` 到 `RGB24` → `cv::Mat`（`CV_8UC3`，**不要**再 `cvtColor`）→ `cv::imencode(".png", ...)`（OpenCV 把 `CV_8UC3` 当 RGB 编码）。
  - **颜色来源**：swscale 默认按 `SWS_BICUBIC` 做 BGR↔YUV，`VideoRescaler` 用默认 flags（`SwsFlagAuto`）。**因为两条链路同源（同一份 `cv::Mat` BGR24、同一个 swscale），口径天然一致**，`take.mp4` 与 `frames/` 的逐像素比对（M4 验收）不应有系统偏差；若实测平均色差 > 2，先查是不是有人在中途插了 `cvtColor`，再考虑统一 `colorspace/color_primaries/color_trc`（都设成 `bt709` + `tv` range）。
- 正立与 sRGB 是 UE 求解链的硬要求（`FHyprsenseRealtimeNode` 只在正立画面上工作），摄像头驱动一般不做旋转，**画面翻转只允许在前端做，不进后端**。
- 像素格式统一到 `AV_PIX_FMT_YUV420P`（H.264 通用选择）；奇数分辨率会被 libx264 拒绝，`POST /recordings` 时把宽高向下取偶。

### 4.4 时间轴：摄像头到达时间 → `fps` 滤镜统一成 25 fps

**输出侧只有一个帧率：25 fps，`AVRational{25, 1}`，`time_base = 1/25`。** 推流、`take.mp4`、UE 的 `-FPS=25/1` 全用它。**输入侧不再假设任何帧率**：摄像头给多少、什么时候给，就是多少。

**三条时间轴，各管各的**：

```text
摄像头线程(阻塞读)
  └─ 到达瞬间打 steady_clock 时间戳 → post 到采集 strand
       ├─ raw.mkv       : 只写真实新帧, 时基 {1,1000}, pts = 到达毫秒（VFR，保真）
       └─ fps 滤镜      : buffer 输入 time_base=1/1'000'000, pts=到达微秒 → fps=25
            └─ take.mp4 + WS fMP4 : 时基 {1,25}, pts = seq = 该链路的输出帧号
```

| 环节 | 时间基准 | 说明 |
| --- | --- | --- |
| 摄像头到达 | `steady_clock`，转成"自开始录制的毫秒" `wall_ms` | 读线程 `read()` 返回的瞬间打；**这是唯一的事实时间** |
| `raw.mkv` | `{1, 1000}`，`pts = wall_ms` | 只存真实帧，**不做任何复制帧**；变帧率是正常的，matroska 原生支持 |
| `fps` 滤镜输入 | `{1, 1'000'000}`，`pts = wall_ms × 1000` | 滤镜按时间轴采样，独此一处做帧率换算 |
| `take.mp4` / 推流 | `{1, 25}`，`pts = seq` | `seq` 由滤镜输出计数，连续、无空洞 |

- **为什么不再固定节拍**：固定 40 ms 采样时，"重复/丢弃哪一帧"由定时器相位和摄像头相位之差决定，抖动随机；改成"到达即处理 + 按真实时间戳采样"后，重复帧会被均匀地铺在时间轴上，`fps` 滤镜挑的是时间上最靠近目标时刻的那一帧。
- **`fps` 滤镜是采样器，不是插值器**：25 fps 输出里必然存在重复帧，它只改变"哪一帧被重复、重复在哪里"，不创造运动。真正的顺滑只能靠摄像头原生 25 fps 或 `minterpolate`（720p 单帧 1.5–3 秒，实时不可行，已否决）。低光长曝光造成的运动模糊也无法用滤镜消除。
- **`raw.mkv` 与 `frames/` 因此不含重复帧**：这是本次改动的实质收益——喂给 UE 求解的是真实采集到的每一张画面，帧数 = 摄像头在录制期间实际产出的帧数（30 fps 摄像头录 10 秒就是约 300 张，而不是恒定的 250 张）。PNG 序列与帧率无关，`write_png_sequence()` 只是把 `raw.mkv` 按解码顺序逐帧写成 `frame_%06d.png`，**没有补位逻辑**。
- **滤镜不可用时降级**：`fps=25` 建不出来（`av::Filter{"fps"}` 为空、graph 配置失败）时 `frame_filter::valid()` 返回 false，直接丢弃该对象；录制照常，`take.mp4` / 推流直接用原始帧并逐帧自增 pts。此时 `take.mp4` 的时间轴会按"一帧一格"播放，与墙钟不等，`capture_result` / `align.json` 里 `source_fps` 与 `inserted_frames` 会暴露这一点。
- **`gaps` 语义收窄为"原始素材缺失"**：摄像头连续 > 1 秒没有返回帧（`k_silent_timeout_ms`）时，用 `{wall_ms_begin, wall_ms_end, duration_ms}` 记一条墙钟区间。它**不参与任何帧号折算**，只是告诉下游"这段时间摄像头哑了"。
- **无界帧率 → 有界队列**：若读线程无界 `post`，摄像头 60 fps、strand 卡住时会内存暴涨；实现里 strand 侧按 `k_max_capture_queue_frames`（120 帧 ≈ 2 秒）限制积压，超过就丢最老的一帧并计入 `gaps`。
- `POST /recordings` 的 body 里**没有 `fps` 字段**：前端没有选择余地。`GET .../recordings/{id}` 与 `align.json` 里有 `"fps": 25`（输出帧率）与 `"source_fps"`（实测摄像头帧率），都是只读信息。
- 时长口径：`duration_ms` = 输出帧数 × 40 ms（`fps` 滤镜的输出时间轴）；`raw_frames` / `inserted_frames` 分别记录真实帧数与由滤镜补出的重复帧数。

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
- 推流与 `take.mp4` 的编码器时基都是 `{1, 25}`，`pts` 就是 `seq`（`fps` 滤镜输出的帧号）；**不要**从墙钟时间戳换算，也不要按摄像头真实出帧率设时基——真实时间戳只进 `fps` 滤镜的输入（§4.4）。

### 5.3 分发（单推流，无背压层）

**本机只有一条推流**，所以这里没有"分发器"，只有一个**出口**：

```text
camera → (fps 滤镜) ─┬─► media_fragment_pipeline(推流编码器) ─► ws_outlet ─► 那一条 WS
                     ├─► take.mp4 编码器
                     └─► 原始帧 ─► raw_sink(FFV1) ─► raw.mkv
```

- `ws_outlet` 就是当前绑定的那条连接的写接口（`shared_ptr`，可为空——没连 WS 时推流编码直接**不做**，省 CPU）。
- **线程模型（硬约束）**：`ws_outlet` 的 strand 背后是 `g_ioc` 线程池，**strand 上的处理函数与它的调用线程不保证同一线程**。所以"发帧"（`send_*`，可能从采集 strand 调用）与"写帧"（`drain()`，在 outlet 自己的 strand 上）之间**没有同线程假设可用**，只能靠"所有共享状态只在 strand 上访问"来串行化。
- **怎么做到**：`send_hello` / `send_event` / `send_fragment` / `notify_state` 一律 `boost::asio::post(strand_, ...)` 投递，逻辑体在 strand 上执行；`attach` / `detach` / `set_init_segment` / `set_session_info` 保留同步签名，但**必须在注入的 strand 上调用**（调用方就是采集 strand，满足）。`queue_` / `pumping_` / `closing_` / `write_` / `state_` / `init_segment_` / `session_id_` / `width_` / `height_` 全部只由 strand 触碰，不需要 mutex，也不需要 TBB 并发容器。
- **查询走 atomic**：`attached_`、`idle_` 两个 `std::atomic_bool` 专门给外部查询用（`is_attached()` / `idle()`），只是快照，不参与 strand 上的任何决策。决策一律用 strand 内的 `closing_` / `write_` / `queue_` 判断。
- **顺序要求**：因为 `send_*` 变成异步投递，**必须先 `attach()` 再发任何 `send_*`**（`send_hello` / `send_fragment`）；反过来的话投递的函数先执行、看到未 attach 就丢弃。`ws_stream_runner` 的顺序就是 attach outlet → `session_->attach()` → `start()`。
- **写不动就丢**：`send_fragment(fragment)` 调完即返回。内部是一个**上限 8 片**的 `std::deque`，`pumping_` 保证**同时只有一条写协程在途**；`drain()` 依次取队首写出去。
- **丢非关键帧**：队列里还压着没发出去的关键帧分片时，新进来的**非关键帧分片直接丢弃**（它反正要靠后面的关键帧重同步）。所以异常时队列最多积到 8 片，随后被关键帧覆盖式清空。**没有任何环形缓冲、没有 history 补发**。
- 为什么不需要更复杂的背压：单用户本机 + 同机前端，WS 走 loopback，单帧分片 10–60 KB，写满窗口的概率极低；偶发丢片最多让画面卡一下，**录制链（`raw.mkv`）完全不受影响**，因为 `raw.mkv` 与推流是两条独立的编码器，各自从同一批 `cv::Mat` 取帧。
- 这也意味着**§3.3 里不需要 `gap` 标记**：丢片时前端只会停在上一帧，下一个关键帧到达自然恢复（GOP = 1 秒）。
- 只由采集 strand 触碰 `ws_outlet` 的同步接口（`attach`/`detach`/`set_*`），投递型接口（`send_*`）可以任意线程调用。**不要裸 `std::queue` 跨线程**。
- 页面切后台导致 WS 停读：beast 的 `async_write` 挂着，队列一直满，分片被丢 —— 这正是期望行为，**不超时、不主动断开**（录制还在跑，断开就没法控制停止）。

### 5.4 录制侧（raw.mkv）

- 与推流**同源但不同帧集**：`raw.mkv` 写的是**摄像头原始帧**（不经 `fps` 滤镜），推流与 `take.mp4` 写的是**滤镜输出帧**。编码器：`FFV1`（`AV_CODEC_ID_FFV1`，level 3），封装 `matroska`。**像素格式不强制转换**：直接用 `AV_PIX_FMT_BGR24`（FFV1 支持 bgr0/bgr0p16 等 RGB 布局），省掉一次 swscale；若实测该布局编码失败再退回 `yuv444p`。
- **时基是 `{1, 1000}`，`pts` 是该帧的到达毫秒**（§4.4）。所以 `raw.mkv` 是变帧率文件，帧内容与时间都是真的，**不含任何复制帧**。`ffprobe` 会显示 `r_frame_rate≈25 左右` 但时间戳不均匀，这是预期行为。
- 为什么 FFV1 而不是 H.264：`raw.mkv` 是转码与 `frames/` 的唯一权威来源，必须**逐帧无损**，否则 PNG 序列里会有压缩痕迹，UE 求解精度受影响。
- 体积代价：1280×720 视摄像头帧率而定，30 fps 时约 8–15 MB/s，1 分钟约 0.5–0.9 GB；1080p 再放大 4 倍。默认分辨率取 **1280×720**（分辨率在 `POST /recordings` 时可调）；超时长（> 10 分钟，见 UE 侧"单段 ≤ 10 分钟"约束）在 `capture_state` 里给 `warning`，不强制中断。
- 落盘线程：`av_interleaved_write_frame` 在无损编码下是大块顺序写，放在采集 strand 上会和采集抢同一个 io 线程。用一条**专用写盘线程**（实现里是 `std::thread` + `mutex`/`condition_variable`）+ 有界队列（上限 300 帧，`k_max_raw_queue_frames`），队列满时**丢帧并返回 false**，**绝不阻塞采集主循环**。
  - 丢帧记在哪：`raw_writer::push()` 返回 false 时 `capture_session` 置 `truncated_ = true`，`align.json` 的 `truncated` 就是这个含义（`raw.mkv` 比 `frames[]` 少帧）。**注意**这会让 `frames[]` 与 `raw.mkv` 包序不再一一对应 —— 这是"写盘线程丢帧"的已知代价，一期不做额外补偿，只标记 `truncated`；转码作业看到 `truncated: true` 时应在 step 1 前后对帧数做一次一致性检查（解码出来的帧数 vs `align.json` 的 `raw_frames`），不一致就判 `raw_frames_mismatch`。
- **坏帧与短尾**：`POST .../stop` 到达时可能还有几帧在队列里，收尾必须先 `drain` 队列再 `av_write_trailer`；`stopping` 状态最长等 2 秒，超时直接 trailer 并记 `align.json` 的 `truncated: true`。**这个等待发生在 HTTP 请求内**，所以 `stop` 的响应时间上限就是 2 秒，前端超时要设得比它大。

---

## 6. 人脸检测（**二期，本方案不实现**）

**拍板：人脸检测整体移出本方案，本方案不写任何检测代码、不引入任何模型、不新增 `face_status` 下行、不做开录前的人脸校验。** 一期交付的是"能录、能推、能转、UE 能出曲线"，画面里有没有正脸由人在前端自己看。

之所以留这一节，是因为下一期要在**同一个采集循环里**插入检测，位置与约束得先钉住，免得二期把采集节奏改乱：

### 6.1 插入点（二期需要遵守的边界）

- 检测必须挂在 §4.4 的"摄像头到达"那一条路径上（`on_capture` 之后、`fps` 滤镜之前或之后二选一，固定选后者以免干扰时间戳），**不能自带定时器**，也不能反过来拖慢采帧。做法是"这一帧跑不完就跳过去"，而不是"等检测跑完再出帧"。
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

- 检测**降频**：固定每 2 帧一次 = **12.5 Hz**（输出帧率 25，§4.4），跑在采集 strand 上；单次检测耗时超过两帧预算就跳过后续帧，把实际频率如实上报。
- 检测分辨率：把帧 `resize` 到长边 640 再送检，边界框按比例映射回原尺寸。
- `status` 取 `none` / `ok` / `multi`，**任何取值都不影响录制**。是否在开录前拦下"没检到脸"的场景留到二期再定（一期没有这道闸，所以一期也没有 `422 no_face`）。
- 线程模型：`FaceDetectorYN` 对象**不可拷贝**，放进 `std::shared_ptr<face_detector>`，只在采集 strand 上调用 `detect()`，销毁也在 strand 上完成。

---

## 7. 转码作业

### 7.1 三步流水（顺序固定）

| step | 输入 | 输出 | 实现 | 完成后 |
| --- | --- | --- | --- | --- |
| 1 `png_sequence` | `raw.mkv`（真实帧，VFR） | `frames/frame_{n}.png`，`n` 从 0 连续 | ffmpeg 解码（逐帧、按解码顺序，不插值）**不补位** | 通知 step 3 可启动 |
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
     "-nosplash", "-stdout",
     "-AllowCommandletRendering", "-RenderOffscreen",
     "-run=MetaHumanFace",
     fmt::format(R"(-FramesDir="{}")", l_frames_dir.generic_string()),
     fmt::format(R"(-OutDir="{}")",    l_curves_dir.generic_string()),
     fmt::format("-FPS={}",            l_fps),
     /* package_path 非空时才追加：*/ fmt::format("-PackagePath={}", l_package_path)},
    logger_ptr_, kitsu_client_, /*create_lock=*/true, l_run_time_info
);
```

**`-PackagePath` 是条件项**：请求里的 `package_path` 为空字符串（或不传）时，这一项**根本不拼进 `in_arg`**，由 Commandlet 用它自己的默认值；后端不写死、不猜、不补。拼参函数做成"传空就跳过"的形式，避免出现 `-PackagePath=` 这种空值参数被 Commandlet 当成非法包路径。

#### 渲染开关：必须给 `-AllowCommandletRendering`

这是 M0 真机跑出来的结论，**与"惯用写法"相反**。`FApp::CanEverRender()`（[App.h:411-419](E:\UnrealEngine\Engine\Source\Runtime\Core\Public\Misc\App.h)）的实现是：

```cpp
return (!IsRunningCommandlet() || IsAllowCommandletRendering()) && !IsRunningDedicatedServer()
       && !(USE_NULL_RHI || bHasNullRHIOnCommandline);
```

Commandlet 默认 `IsAllowCommandletRendering()` 为假（`-AllowCommandletRendering` 是独立的开关，见 [LaunchEngineLoop.cpp:2207](E:\UnrealEngine\Engine\Source\Runtime\Launch\Private\LaunchEngineLoop.cpp)），于是 `RHIInit`（[DynamicRHI.cpp:278-362](E:\UnrealEngine\Engine\Source\Runtime\RHI\Private\DynamicRHI.cpp)）走 `InitNullRHI()`。而 MetaHuman 的动作捕捉求解只认 D3D12（`FMetaHumanSupportedRHI::IsSupported()`），NullRHI 下 `UMetaHumanPerformance::CanProcess()` 直接返回 false，表现为 `ProcessingRejected` + 退出码 4。

| 命令行片段 | 后果 |
| --- | --- |
| 什么都不给 | NullRHI → `ProcessingRejected`，退出码 4 |
| `-unattended` | **同样落进 NullRHI**，且 `FMessageDialog` 被抑制后错误只剩日志，更难查 |
| `-nullrhi` | NullRHI → 同上 |
| `-nullrhi=0` | **仍然等于 NullRHI**：`bHasNullRHIOnCommandline = FParse::Param(...)` 只判断开关是否出现，不解析它的值 |
| `-AllowCommandletRendering -RenderOffscreen` | ✅ D3D12 + 离屏渲染，求解正常 |

`-RenderOffscreen` 让 RHI 在没有窗口的情况下也能创建设备；`-nosplash` / `-stdout` 是常规的无界面参数，`-stdout` 保证 `async_run_ue` 的管道能实时读到日志。

> 副作用：启用 RHI 后首帧会编译光追 PSO（`LogD3D12RHI: Compiled PathTracingMainRG for RTPSO in 20432.52 ms`，冷启可能出现 4 次、每次 20–45 秒）。这部分耗时进 `elapsed_seconds`，**不属于求解**，判断性能时要用 `Finished Pipeline ... in X seconds` 那一行。

#### 参数表（Commandlet 实际接受的形式）

解析走 `UCommandlet::ParseCommandLine(Params, Tokens, Switches, Params)`（`MetaHumanFaceCommandLine.cpp:82`）：`=` 两侧都是值 → 进 `Params` 表；没有 `=` 的 → 进 `Switches`。取值**大小写不敏感**，`-FramesDir` 与 `-framesdir` 等价。

| 参数 | 必需 | 语义 | 缺失/非法时 |
| --- | --- | --- | --- |
| `-FramesDir=<绝对目录>` | **是** | 帧序列所在目录（不递归） | 缺 / 相对路径 → `InvalidRequest`，退出码 2 |
| `-OutDir=<绝对目录>` | **是** | 曲线产物落点，不存在会自动创建 | 缺 / 相对路径 → `InvalidRequest`，退出码 2 |
| `-FPS=<num>/<den>` | 否 | 素材帧率，**默认 `25/1`** | 不是 `正整数/正整数` → 退出码 2 |
| `-SequencePattern=<含{frame}的模板>` | 否 | 一般**不传**（命名规则见 §8.1） | 传了但不含 `{frame}` → 退出码 2 |
| `-PackagePath=/Game/...` | 否 | 中间资产的包路径，末段不得含 `.` | 非空且格式错 → 退出码 2；**空/不传 → 用 `<OutDir>/Package`** |
| `-NoHeadStabilize` | 否 | 无值开关，关掉头部稳定 | — |
| `-KeepTongueSolve` | 否 | 无值开关，打开舌头求解（默认关） | — |
| `-H` / `-Help` / `-?` | 否 | 打印 `Usage()` 到日志并退出，退出码 2 | — |

- **`-NoHeadStabilize` / `-KeepTongueSolve` 给值会报错**：写成 `-NoHeadStabilize=1` 会被 `UCommandlet::ParseCommandLine` 放进 `Params` 表，Commandlet 明确拒绝（`argument "-NoHeadStabilize" does not take a value`，退出码 2），不会静默失效。
- **其它任何参数一律静默忽略**（`MetaHumanFaceCommandLine.cpp:184` 的 `return false`）。引擎自己会往命令行塞大量开关（`-nosplash`、`-stdout`、`-RenderOffscreen` 等），Commandlet 只校验自己认识的那几个。
- **`-Range` 已不存在**：本命令只处理整段序列（§8.1）。残留的 `-Range=...` 会被当成未知参数静默吞掉，不报错也不生效。

要点：

1. **不新写进程封装**。`async_run_ue`（[ue_exe.cpp:160-241](E:\Doodle\src\doodle_lib\exe_warp\ue_exe.cpp)）已经做了：UE 版本/插件安装 → 进程启动（`boost::process::v2`，读 stdout/stderr 管道）→ `timeout` 计时 → 结束后回捞 `<Project>/Saved/Logs` 最新日志 → 非 0 退出码抛 `doodle_error`。调用范例见 [import_and_render_ue.cpp:242-248](E:\Doodle\src\doodle_lib\exe_warp\import_and_render_ue.cpp)。
2. **`create_lock=true`**——与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`（`ue_ctx` 定义见 [ue_exe.h:26-31](E:\Doodle\src\doodle_lib\exe_warp\ue_exe.h)，闩本身是 [co_queue.h](E:\Doodle\src\doodle_lib\core\co_queue.h) 的 `awaitable_queue_limitation`，`limit` 默认 1）。**同一台机器上不允许两个 UE 进程并发**，脸捕排队在渲染任务后面是可接受的代价，换来零新增跨模块改动。
3. **`in_arg.front()` 必须是 `.uproject` 路径**：`async_run_ue` 用 `ue_exe_ns::find_ue_project_file(in_arg.front())` 回捞日志（`ue_exe.cpp:206`），顺序错了日志就丢。
4. **不能用 `-nullrhi`，也不要用 `-unattended`，必须给 `-AllowCommandletRendering -RenderOffscreen`**：求解走 `NNERuntimeORTDml` + D3D12，而 Commandlet 默认不创建 RHI 上下文（`FApp::CanEverRender()` 为假 → NullRHI）。三个开关的判定细节见上面的「渲染开关」小节，`-nullrhi=0` 同样无效。
5. **超时**：`async_run_ue` 用的是全局 `core_set::get_set().timeout`，脸捕耗时与帧数线性相关，可能不够。**本轮不改 `async_run_ue` 签名**；M0 实测出单帧耗时后，若 10 分钟素材会超时，再单独提"给 `async_run_ue` 加可选 timeout 参数"的改动（只加一个带默认值的形参，不改现有调用点）。
6. **`kitsu_client_` 用来做插件版本检查与安装**；脸捕作业没有 kitsu 上下文时传 `nullptr` 会在 `installUePath` 里解引用。因此作业在构造时从路由层拿到 `kitsu_client`（同 `local` 其它路由的做法：`std::make_shared<doodle::kitsu::kitsu_client>(core_set::get_set().server_ip)` + `set_token(token_)`）。
7. **`-nosplash` / `-stdout` 保实时日志**：`-stdout` 让引擎把日志写进 stdout，`async_run_ue` 的管道才读得到过程日志；`-nosplash` 省掉启动画面。**不要再补 `-Unattended`**（见要点 4）——它会把 RHI 打回 NullRHI，而且抑制了 `FMessageDialog` 之后失败只剩日志，比不加更难查。
8. **`.uproject` 由前端必填且必须是绝对路径**：脸捕是"某个人在某台机器上用某个工程跑"，工程路径不能从代码里猜、也不能从配置里猜。前端在 `POST /transcode` 的 body 里传 `ue_project`（**必填、绝对路径**），后端**只做两个校验**：`path::is_absolute()` 为真、该文件存在。**不拼 `ue4_path`、不做相对路径解析、不回退 `find_ue_project_file` 或任何默认工程**；任一校验不过，一律在提交时返回 `400 missing_ue_project`——猜错工程比报错更糟（用户会拿到别的工程的曲线）。
9. **`package_path` 由 Commandlet 决定**：请求里为空就不传 `-PackagePath`（见上面的条件项）。后端只做"非空时必须形如 `/Game/...` 且末段不含 `.`"的校验，不提供默认值。

### 7.4 结果解析

- 优先读 `curves/face_capture_log.json`（Commandlet 自己写，`MetaHumanFaceCommandlet.cpp:421-448`），字段：`stage / code / detail / curves_path / summary_path / log_path / frame_count / curve_count / elapsed_seconds / dropped_frame_ranges / missing_model_assets`（`MakeResultJson`，`MetaHumanFaceCommandlet.cpp:165-202`）。**成功和失败都会写**，前提是一次运行已经走到 `OutDir` 已知的那一步；参数解析阶段就失败（`-FramesDir` 缺失等）时不写——那时 `RunOutDir` 还没确定落点。
- 退出码映射（`ToExitCode`，`MetaHumanFaceCommandlet.cpp:450-470`），**以 `code` 字段为准、退出码只用于粗分类**：

| 退出码 | 覆盖的 `code` |
| --- | --- |
| `0` | `None`（成功） |
| `2` | `InvalidRequest`、`FramesNotFound`、`FramesFormatMismatch`、`FrameCountMismatch`、`CaptureDataBuildFailed` |
| `3` | `MetaHumanModelMissing`、`InferenceRuntimeMissing` |
| `4` | `PerformanceCreationFailed`、`ProcessingFailed`、`NoAnimationData`、`CurveSetMismatch`、`Cancelled`、`TimedOut`，以及 `NotInGameThread`/`StartError`/`ProcessError`/`EndError`/`TooFast`/`InsufficientThreadsForNodes` 等管线退出状态 |
| `5` | `ExportFailed`、`FileSystemFailed` |

  `stage` 取 `Queued / Building / Solving / Exporting / Done / Failed`，与退出码组合能区分"素材错"（2 / Building）与"求解错"（4 / Solving）。
- `async_run_ue` 在非 0 退出码时抛异常，作业里 `catch (const doodle_error&)` 后**仍然去读 `face_capture_log.json`**（文件已落盘），把 `code/detail` 转成前端的 `error.code`。
- **`exit_code == 0` 不等于产物齐全**：必须同时校验 `curves_path` 存在且非空、`summary_path` 存在且非空、`frame_count > 0`。
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
| 文件名必须匹配 `^(([a-zA-Z0-9]*[_\s-])*)([0-9]+)[a-zA-Z_\s-]*\.[a-zA-Z]+$` | `MetaHumanFaceCaptureDataBuilder.cpp:20` | 固定 `frame_000000.png`（前缀 `frame_` + 下划线 + 6 位数字） |
| 数字位数必须全序列一致 | `:212-222` | 固定 6 位；超过 999999 帧直接拒绝，**不自动加位** |
| 帧号必须从首帧起连续，不能有空洞 | `:233-258`（缺帧即 `FramesNotFound`，**不允许跳过**） | `frames/` 由 `raw.mkv` **按解码顺序逐帧写出**，`n` 从 0 连续自增，天然无空洞（真实帧按下标编号，不再需要补位，§10） |
| 扩展名限 `bmp/dds/exr/jpg/jpeg/png` 且必须能被 ImgMedia 读 | `:287-298` | 只用 PNG |
| 目录里不能有别的图片文件 | `:148-156` 会扫描所有图片扩展名 | `frames/` 下**只放 PNG**，任何 meta 文件放 `curves/` 或 `session_dir` 根 |
| `-FramesDir` / `-OutDir` 必须是绝对路径 | `MetaHumanFaceCommandLine.cpp:194-206` | 传 `FSys::path::generic_string()` 的绝对路径并整体加引号 |
| `-PackagePath` 必须是 `/Game/...` 形式的包路径（不得含 `.`） | `MetaHumanFaceCommandLine.cpp:208-214` | **请求给了就校验**（非空必须是合法包路径，否则 `400 bad_package_path`）；**请求没给就不传这个参数**，落点由 Commandlet 决定（§7.2、§7.3） |
| ImgMedia 侧还要能解析出同一组帧 | `:449-474` `FTrackingPathUtils::GetTrackingFilePathAndInfo` 的 offset / 帧数必须与扫描结果一致 | M0 真机验证项（§11） |
| **不传 `-SequencePattern`** | `MetaHumanFaceCommandLine.cpp:161-165` 接收、`:216-221` 只校验"传了但不含 `{frame}`" | 省略；文件命名固定为 `frame_000000.png` |

### 8.2 帧率与时间基准

- **`-FPS=25/1`，写死。** Commandlet 要的是 `<num>/<den>` 分数形式（`MetaHumanFaceCommandLine.cpp:263-298`），但后端不需要做"帧率 → 分数"的换算：输出帧率恒为 25（§4.4），常量字符串直接拼进命令行。参数缺省值也是 `25/1`（`MetaHumanFaceTypes.h:46-47`），所以即使漏传也不会退回 23.976 那种反直觉的默认值。
- **`-FramesDir` / `-OutDir` 必须绝对路径**（`MetaHumanFaceCommandLine.cpp:194-206`），相对路径直接拒绝、不做 `FPaths::ConvertRelativePathToFull` 兜底。
- **必须在 `curves/`（即 `-OutDir`）写 `frames_meta.json`，字段 `first_frame`**（`ResolveStartTimecode`，`MetaHumanFaceCaptureDataBuilder.cpp:312-340`）。三级兜底：`frames_meta.json` 的 `first_frame` → 目录里扫描到的最小帧号（`frame_000000.png` 起就是 0）→ 当前墙钟。
  - 落到墙钟这一级不是"随机值"，而是**一个合法的任意起始时间码**：`MakeNonZeroTimecode`（`:91-102`）专门避开全零时间码（`UImageSequenceTimecodeUtils::IsValidTimecode` 把 `FTimecode()` 判为非法）。素材从 `frame_000000.png` 起步时用哪一级结果都一样，所以后端**可以省略这个文件**；`first_frame` 只对"序列从非零帧号开始"的场景有意义。
- UE 只要求"帧序列连续"，**不要求源素材真的是 25 fps**：`frames/` 来自 `raw.mkv` 的真实帧，`-FPS=25/1` 只是告诉求解器"把这些图当作均匀 25 fps 来处理"。所以摄像头实际 30 fps 也不影响求解，只是时间轴被压到 25 fps 的节奏上（每张 40 ms）。
- **没有 `-Range`**：本命令只处理整段序列，`StartFrameToProcess` / `EndFrameToProcess` 由 `UMetaHumanPerformance::GetProcessingLimitFrameRange()` 自动取全区间。需要只处理某一段，由调用方先裁好帧放进独立目录（§8.1）。

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
    raw.mkv                    # 录制结束即固定，唯一权威来源，永不自动删除（真实帧，时基 1/1000）
    align.json                 # seq / orig_seq / wall_ms / 静默区间 / 实际宽高（输出帧率恒为 25）
    take.mp4                   # 前端回放（step 2）
    png_tmp/                   # step 1 的落盘暂存，自检通过后整体改名为 frames/
    frames/                    # step 1 产出 → UE 输入；step 3 成功后删除（§8.4）
        frame_000000.png …
    curves/                    # step 3 产出
        face_curves.dhfc           # 主交付，二进制（格式见 metahuman_face_capture_solution.md §5）
        face_curves_summary.json   # 人读摘要：元信息 + 掉帧区间 + 每条曲线的 min/max/first/last
        face_capture_log.json      # Commandlet 自写，成功失败都写（§7.4）
        frames_meta.json           # 可选，后端写 `{"first_frame": n}`（§8.2）
    logs/
        transcode.log              # 三步作业过程日志
```

`curves/` 里那三个文件由 Commandlet 在 `-OutDir` 下直接落盘（`MetaHumanFaceCommandlet.cpp:20-22` 的三个文件名常量）。`face_curves_summary.json` 的字段：

| 字段 | 说明 |
| --- | --- |
| `magic` / `version` / `flags` | 与 `.dhfc` 二进制头同源（`"DHFC0001"` / `1` / `2`），用来确认摘要和曲线文件配套 |
| `frame_count` / `curve_count` / `fps` | 帧数、曲线数、帧率（`"25/1"`），`curve_count` 实测为 257（6 条头部 + 251 条面部） |
| `start_frame` / `start_time_sec` / `duration_sec` | 起始帧号与对应秒数、时长 |
| `dropped_frame_ranges` | `[{start, end}]`；缺图在扫描阶段就失败，正常为空数组 |
| `curves` | `[{name, min, max, first, last}]`，每条曲线的极值与首末值，用于不开二进制也能判断某条曲线是不是全程为 0 |

`core_set::get_set().get_cache_root("face_capture")` 与 `movie.cpp` 用 `get_cache_root("http")` 同款。

体积预估（1280×720，1 分钟，按 30 fps 摄像头）：`raw.mkv` 约 0.5–0.9 GB、`frames/` 约 0.6–1.2 GB（PNG 逐帧，取决于画面细节与帧数）、`take.mp4` 约 12–21 MB（输出恒为 25 fps）。`frames/` 与 `raw.mkv` 同一量级，**所以"成功即删 `frames/`"不是优化而是必需**。

判读 `align.json` 的字段（实现时按此定结构体）：

```json
{
  "session_id": "…", "width": 1280, "height": 720, "fps": 25,
  "source_fps": 29.97, "started_at": "2026-11-05T09:00:00+08:00", "duration_ms": 61000,
  "raw_frames": 1832, "inserted_frames": 0,
  "frames": [{"seq": 0, "orig_seq": 0, "wall_ms": 0}],
  "gaps": [{"wall_ms_begin": 16480, "wall_ms_end": 17120, "duration_ms": 640}],
  "truncated": false
}
```

- **没有 `face` 字段**：人脸检测在二期（§6），一期不往权威记录里写任何检测结果。
- **`fps` 是输出帧率（恒 25），`source_fps` 是实测摄像头帧率**（由真实帧数与墙钟算得），两者不必相等。
- `frames[]` 一项对应 `raw.mkv` 里的一个真实帧，`i` 就是解码顺序下标：`seq` 是该帧在 25 fps 输出时间轴上的位置（`fps` 滤镜给它分配的序号），`orig_seq` 是原始采集序号（0,1,2…严格连续），`wall_ms` 是它到达的时刻。
- `gaps` 只在摄像头连续 > 1 秒没返回帧时出现（§4.4），记的是**墙钟区间**，不参与帧号折算。
- 当摄像头帧率高于 25 时，`seq` 仍从 0 起连续编号（滤镜输出的第 n 帧），与 `orig_seq` 不再一一对应；**这不影响 `frames/`**：`write_png_sequence()` 按真实帧下标逐帧写 PNG，不用 `seq`（§10）。`inserted_frames` 统计滤镜补出的重复帧数（输出帧数 > 真实帧数时其值 > 0）。

---

## 10. 丢帧补位：已废止

**取消固定节拍后，这一节不再有实现。** `raw.mkv` 只存摄像头真实帧（§4.4），`frames/` 按下标逐帧写出，`n` 从 0 连续自增，天然满足 UE 的"帧号连续无空洞"约束（§8.1），**不需要复制任何帧**。

`write_png_sequence()` 的现行做法（已实现并编译通过）：

1. `raw.mkv` 按解码顺序逐帧取图（`av::VideoDecoderContext`）→ `av::VideoRescaler` 到 `RGB24` → `cv::imencode(".png")`；
2. 第 i 个解码帧写成 `png_tmp/frame_{i:06d}.png`，**下标自增，与 `align.json` 的 `seq` 无关**；
3. 全部成功才 `rename(png_tmp → frames/)`；失败保留 `png_tmp/` 并返回 `png_sequence_invalid`；
4. 写完后比对解码帧数与 `align.json` 的 `raw_frames`，不一致只记 `SPDLOG_WARN`（不复位、不补帧）——真实差集是"写盘线程丢帧"，由 `truncated` 标记（§5.4）。

历史方案（保留以备查阅）：固定 40 ms 采样时代，采集侧产出的帧率恒为 25，而摄像头卡死会造成帧号空洞，于是按"策略 A：重复上一帧"把 PNG 序列补齐到 `len(frames) + Σ gaps.missing` 张。**该机制随固定节拍一起删除。**

---

## 11. 里程碑

| 里程碑 | 内容 | 验收 |
| --- | --- | --- |
| **M0（0.5 天，只验证不写产品代码）** | ①手搓一份 `frame_000000.png`（6 位数字）序列，真机跑 `-run=MetaHumanFace -FramesDir=<abs> -OutDir=<abs> -FPS=25/1`；②记录单帧求解耗时，判断 `core_set.timeout` 是否够；③确认 `curves/frames_meta.json` 的 `first_frame` 被采用；④确认不传 `-SequencePattern` 能通过；⑤确认 `FTrackingPathUtils` 接受该命名 | 五条结论写回本文 §8；任一条失败立即回到 §8 复核 |
| **M1（引擎 + WS + 设备已完成）** | 摄像头读线程 + `frame_filter` + `media_fragment_pipeline`（推流）+ `ws_outlet` + **WS 路由 `face_capture_stream`（内含 `ws_stream_runner`，连上即开录、断开即收尾）与注册** + **设备路由 `face_capture_devices`（GET 枚举 / POST 选中）**；**剩下 HTTP 录制端点**：`POST/GET recordings` | 引擎、WS、设备侧：编译通过 ✅；剩下浏览器里选设备 → 连 WS 就出现实时画面 + 断开后 `raw.mkv`/`align.json` 已落盘 |
| **M2（引擎部分已完成）** | `raw.mkv`（FFV1，时基 1/1000）+ `align.json` + 状态机 + `take.mp4`；**HTTP 层待做**：`POST/GET/stop` 端点 | 引擎侧：编译通过 ✅；`ffprobe take.mp4` 的相邻 `pts_time` 间隔恒为 0.04s、`raw.mkv` 帧数≈摄像头帧率×秒数 |
| M3 | 转码作业 step 1 + step 2（`png_sequence` 逐帧无补位、`mp4`）+ 作业查询/取消。**`write_png_sequence()` 已实现并编译通过**，等待最终作业调度层 | 不接 UE 也能产出合格 `frames/` 与 `take.mp4`，命名校验生效 |
| M4 | 转码作业 step 3（Commandlet 编排 + 结果解析 + `frames/` 删除） | 端到端：录 → PNG → UE → `.dhfc` |
| M5 | 加固：状态机重入、`ws_outlet` 丢片调参、`ue_ctx` 排队观测、错误码统一 | 连续录 20 次无泄漏、无卡死 |

> 一期只有 M0–M5 六个阶段，没有人脸检测相关的里程碑。二期的检测工作另立方案，开工第一步是验 `objdetect` 能否编过（§6.2）。

---

## 12. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| **UE 侧命名/跟踪路径不匹配**（B 路线首风险） | `BuildWithTimecode` 直接 `CaptureDataBuildFailed`，表现为"素材错" | M0 先钉死；step 1 写盘后比对帧数与 `raw_frames`，不合格不提交 UE |
| `cv::VideoCapture` 阻塞 io 线程 | 整个 HTTP 服务卡死 | 读帧放在独立 `std::thread`，strand 只处理已到手的帧（§4.1） |
| `stop()` 时 `read()` 卡死不返回 | HTTP 请求挂住 | `cap_->release()` 唤醒阻塞读；2 秒内退不出就记 `truncated` 并 `detach`（§4.1） |
| 摄像头帧率高于 25 | 推流/`take.mp4` 时间轴抖动 | `fps=25` 滤镜按真实时间戳采样；`raw.mkv` 与 `frames/` 不受影响（§4.4） |
| 摄像头帧率低于 25 | 推流里出现重复帧 | `fps` 滤镜自动补重复帧，`inserted_frames` 记录数量；只在画面运动上有观感差异，不影响求解 |
| `fps` 滤镜建不出来 | `take.mp4` 时间轴按帧数而非墙钟 | `frame_filter::valid()` 为 false 时丢弃滤镜、降级为原始帧逐帧编码，`source_fps` / `inserted_frames` 暴露实情（§4.4） |
| WS 写并发 | beast 直接崩或数据错乱 | 所有共享状态只在 outlet 自己的 strand 上访问：`send_*` 一律 post 到 strand，`attach`/`detach`/`set_*` 要求调用方已在 strand 上；查询只读 `attached_`/`idle_` 两个 atomic（§5.3） |
| `stop()` 同步阻塞 strand | 写协程饿死，等满 2 秒才返回 | `wait_outlet_idle()` 改成 `co_await timer.async_wait()` 轮询，让出 strand（§4.2） |
| 页面挂起导致 WS 停读 | 写挂住、推流分片被丢 | 上限 8 片队列 + 丢非关键帧（§5.3）；**录制链不受影响**，丢的只是画面 |
| 摄像头型号差异（分辨率/帧率/MJPG 直出） | 请求的分辨率被静默改成别的 | 回读实际分辨率并上报（§4.3）；帧率不需要协商，滤镜负责统一（§4.4） |
| 摄像头静默 / 驱动掉帧 | 录制里出现时长空洞 | `gaps` 记墙钟区间（§4.4）；`raw.mkv` 与 `frames/` 只少几张，UE 不受影响（§10） |
| `raw.mkv` 体积巨大 | 磁盘撑爆 | 默认 720p、成功删 `frames/`、`raw.mkv` 不自动删但需要水位告警（运维） |
| `core_set.timeout` 不足以覆盖脸捕 | 长素材必失败 | M0 实测后决定是否为 `async_run_ue` 增加可选 timeout（§7.3-5） |
| 与渲染任务抢 `ue_ctx` 闩 | 脸捕要排队，前端进度长时间停在 40% | 进度里显式给出 `ue_commandlet: waiting_for_ue_lock` 子状态，别让前端以为卡死 |
| `async_run_ue` 超时后 UE 进程仍在跑 | 闩被占死，后续所有 UE 任务排队卡住 | 作业捕获超时异常后先终止该进程再返回（§7.4） |
| 路由类被 `clone()` | 成员生成两份，引擎状态错乱 | 会话归 `ws_stream_runner` 所有，路由成员只有 `shared_ptr<ws_stream_runner> runner_`；跨请求共享的选中设备用 `shared_ptr<impl>` 而不是值成员；HTTP handler 不存会话（§3.2） |
| WS 断开时 `stop()` 未完成 | `raw.mkv` 没有 `trailer`、`align.json` 缺失 | `close_impl()` 先 `co_await stop()` 再关流；进程退出时由 `app_base::Get().on_cancel` 槽控制协程退出顺序 |

> 一期没有"模型缺失""opencv 不带 objdetect"这类风险——人脸检测整体在二期（§6）。

---

## 13. 验证方式

按仓库约束：**不写测试、不跑测试**，验证手段只有编译通过（`cmake_configure Ninja_debug` + `cmake_build debug_doodle`）。运行期验证只在 M0 里程碑里做一次真机冒烟，其余靠人工按 §3.2 的接口清单点一遍。

**当前状态**：采集引擎 7 组源文件已全部实现，WS 路由（`face_capture_stream` + `kitsu.cpp` 注册 + 两个 CMakeLists）也已完成——`ws_stream_runner` 就是会话持有者，连上开录、断开收尾。设备路由 `face_capture_devices`（GET 枚举 / POST 选中）已实现，选中值以共享 `shared_ptr` 注入 `face_capture_stream`，WS 连接时传给 `capture_session::start()`。`cmake_build(--preset debug_doodle)` **exit 0**（`doodle_lib.dll` / `test_main.exe` / `doodle_kitsu_supplement.exe` 全链接通过）。HTTP 录制端点（`POST/GET recordings`、`stop`、`video`、`curves`）、转码作业层尚未开始。

**编译期踩过的坑**（写后续代码时照此避让）：
- `av::VideoFrame` **没有 `copyTo`**，取像素要用 `copyToBuffer(uint8_t*, size_t, align)`，且它是**非 const 成员** —— 所以取帧的函数参数要按值收 `av::VideoFrame`。
- `av::VideoRescaler::rescale()` 返回新帧，缩放这一步不能省（直接 `wrap` 原始帧会拿到未缩放的尺寸）。
- `av::Timestamp` 与 `AV_NOPTS_VALUE` 直接比会 `C2593: operator == is ambiguous`（Timestamp 可隐式转 `int`/`double`），要用 `pts().isValid()`。
- `boost/asio/make_strand.hpp` 不存在，`make_strand` 在 `boost/asio/strand.hpp`。
- beast 的流类型是 `boost::beast::tcp_stream`（头 `boost/beast/core/tcp_stream.hpp`），不是 `boost::asio::ip::tcp::tcp_stream`。
- beast websocket 写接口 `async_write_some(bool fin, ...)` 第一参必须传 `true`。
- 重写整个 `.cpp` 时务必确认没有残留旧函数体，否则 `C2084 ... already has a body`。
- 用了 `boost::asio::post` 要包含 `boost/asio/post.hpp`；`std::atomic_bool` 成员要包含 `<atomic>`（`ws_outlet.cpp` 已补）。

**编译期注意**：新增源文件必须同时登记到 `core/CMakeLists.txt`、`exe_warp/CMakeLists.txt`、`http_method/local/CMakeLists.txt` 三个 `target_sources(doodle_lib ...)` 里（现状见 [local/CMakeLists.txt](E:\Doodle\src\doodle_lib\http_method\local\CMakeLists.txt)：`PUBLIC` 只放 `.h`，`.cpp` 全在 `PRIVATE`）。漏登记表现为"链接时找不到符号"而不是语法错误。

---

## 14. 已拍板 / 仍未定

**已拍板（2026-11-05 / 11-06，不必再问）**：

1. ~~丢帧补位 = 策略 A（重复上一帧）~~ → **已废止**：取消固定节拍后 `raw.mkv` 只存真实帧，`frames/` 逐帧写出天然连续（§10）。
2. 脸捕 Commandlet 与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`（§7.3-2）。
3. `frames/` 在转码作业成功后**默认删除**、`raw.mkv` 保留，**不可配置**（§7.5）。
4. **推流只有一条**，不存在第二条，设计上也不为它留口子（§0 第 9/10 条、§5.3）。
5. **控制一律走 HTTP POST，WS 纯下行**（§0 第 7 条、§3.5）。
6. **人脸检测推迟到二期**，一期不实现、不预留字段（§0 第 12 条、§6）。
7. **转码作业不登记 `server_task_info`**：不写库、不进任务列表，只走 `GET /transcode/{job_id}` 与 WS 进度（§7.2）。
8. **`ue_project` 由前端在转码请求里必填，且必须是绝对路径**：缺失/为空/非绝对路径/文件不存在即 `400 missing_ue_project`；后端不拼相对路径、不猜默认工程（§7.2、§7.3-8）。
9. **`package_path` 可以为空，空则不传 `-PackagePath`**，由 Commandlet 自己决定落点；非空时后端只校验格式（§7.2、§7.3-9、§7.3 的条件拼参）。
10. **取消固定节拍**（2026-11-06）：不再用 `steady_timer` 40 ms 轮询采样；摄像头阻塞读放独立线程，到达即打时间戳 `post` 到 strand，`fps` 滤镜统一成 25 fps（§0 第 16 条、§4.1、§4.4）。
11. **`raw.mkv` 存真实帧、时基 `{1,1000}`**：不写复制帧，`frames/` 由它逐帧解码得到，不做补位（§4.4、§5.4、§10）。
12. **`capture_session` 归 `ws_stream_runner` 所有**（2026-11-06）：不打开 WS 就不抓摄像头，WS 连上即开录、断开即收尾；删除进程内单例与 `make()`（§0 第 6/17 条、§3.3）。
13. **选中摄像头是 `face_capture_devices` 路由上的共享属性**（2026-11-06）：`shared_ptr<impl>` 保证 clone 后仍共享，通过 `reg_c` 构造参数注入 `face_capture_stream`；不进 `core_set`（§3.1）。

**仍未定（不阻塞实现）**：

1. M0 真机跑一次 Commandlet 后确认：不传 `-PackagePath` 时它的实际默认落点是否稳定（决定前端要不要养成显式传的习惯），以及 `-PackagePath=` 空值是否会被它当非法参数（所以后端才做条件拼参，不拼空值）。
2. `/face_capture/stream` 的 WS 是否校验登录 token：当前 `websocket_init` 是空实现（纯下行、只推本机画面），要校验就得在 accept 之前抛异常，而 `websocket_init` 的异常在 `async_websocket_session` 协程里的行为未验证。
3. 选中的摄像头**不持久化**：进程重启回 0。要不要落盘（写 `core_set` 或配置文件）等前端提需求再定。

---

## 附录 A：改动面清单（实现时逐项勾掉）

| 文件 | 动作 | 状态 |
| --- | --- | --- |
| `src/doodle_lib/core/face_capture/*`（7 组 .h/.cpp：`face_capture_types.h`、`media_fragment_pipeline`、`frame_filter`、`ws_outlet`、`capture_session`、`frame_align`、`png_sequence_writer`） | 新增 | **已完成，编译通过** |
| `src/doodle_lib/core/CMakeLists.txt` | 登记新增源文件 | **已完成** |
| `src/doodle_lib/exe_warp/face_capture/*`（2 组 .h/.cpp） | 新增 | 未开始 |
| `src/doodle_lib/exe_warp/CMakeLists.txt` | 登记新增源文件 | 未开始 |
| `src/doodle_lib/http_method/local/face_capture.h` + `face_capture_stream.cpp` | 新增 | **已完成**（WS 部分，`ws_stream_runner` 就地定义在 `.cpp` 里；构造参数注入选中的设备） |
| `src/doodle_lib/http_method/local/face_capture_device.cpp` | 新增 | **已完成**（GET 枚举 / POST 选中，`face_capture_selected_device` 共享 `shared_ptr<impl>`）；录制 3 个 .cpp 与转码 1 个 .cpp 未开始 |
| `src/doodle_lib/http_method/local/CMakeLists.txt` | 登记 `.h`(PUBLIC) 与 `.cpp`(PRIVATE) | **已完成**（后续新增 .cpp 继续登记） |
| `src/doodle_lib/http_method/kitsu.cpp` | `create_kitsu_local_route()` 追加 §3.1 的 10 条 `reg_c` | **已完成 2 条**（`face_capture_devices`、`face_capture_stream`），其余 8 条未开始 |
| `src/doodle_core/metadata/kitsu_ctx_t.h` / `core_set` | **不改**（人脸检测在二期才加 `face_detector_model_path_`；选中摄像头不落 `core_set`） | — |
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
| `capture_session_registry`（`session_id` 查找表） | 删除，动作改由 `ws_stream_runner` 持有的会话对象承担 | 本机只有一个会话 |
| `capture_session::instance()` 进程内单例 | 删除，`capture_session` 变成 `ws_stream_runner` 的成员，随 WS 连接创建/销毁 | 不打开 WS 就不该抓摄像头；单例会让"有连接/没连接"与"有没有会话"脱钩 |
| WS 按 `session_id` 分路径、多连接互斥 + `409 device_busy` | 路径保留占位但不参与查找；第二条连接 attach 后自然接管，旧 outlet 不再被推送（不发 `superseded`，旧连接自己从 `async_read` 失败退出） | 摄像头只有一个消费者，不需要仲裁 |
| 多标签页：`control:"observer"` 观察者模式 | 删除 | 只有一条推流、一个控制者 |
| 每连接 `fragment_sink` + 环形缓冲 + 逐连接 FIFO(8 片) + `gap` 标记 | 上限 8 片队列 + 关键帧优先级覆盖 + 丢非关键帧，`gap` 字段删除 | 只有一个出口，不需要分发与回放补发 |
| `align.json` 里的 `dropped_by_sink` | 删除 | 推流丢片不影响录制，不写进权威记录 |
| `capture_start` 的 `fill_mode` 字段（只接受一个值） | 删除 | 策略 A 已拍板，协议里不留无效开关 |
| WS 上行命令（`capture_start` / `capture_stop` / `transcode_start` / `ping`） | 全部删除，改为 `POST /recordings`、`POST .../stop`、`POST /transcode` | 本机 loopback，请求-响应没有不确定性，没必要维护 ack/超时/重放（§3.5） |
| WS 读协程里的命令解析与 `post` 到 strand 的分派 | 删除，读端只用于断连检测 | 没有上行命令可解析 |
| WS 心跳 `ping/pong` | 删除，服务端不做空闲超时 | 单用户本机连接不该被服务端主动断开 |
| 单独的"兜底停止"REST 端点 | 合并为唯一的 `POST .../stop` | 控制面只有一条路，不存在"主路用完用兜底" |
| 帧率可配：`POST /recordings` 的 `fps`、转码的 `fps:"num/den"`、`align.json` 的 `fps_num/fps_den`、按实际出帧率设 `time_base` | 全部删除，输出帧率常量 25 fps；`align.json` 只留 `"fps": 25`（输出）与 `"source_fps"`（实测），`raw_pts` 一列也删 | 帧率定死（§4.4），分子分母推理、实际帧率协商、`host_pts → pts` 换算全都不需要 |
| 丢帧判定的 `1.8 × 帧间隔` 自适应阈值 | 删除；`gaps` 只记录"连续 > 1 秒无新帧"的静默区间 | 帧率固定后没有"帧率抖动"这个丢帧来源 |
| 编码链 `cvtColor(BGR→RGB)` 中转后交 `SwsConverter` | 直接 `AV_PIX_FMT_BGR24` → `av::VideoRescaler` → `YUV420P`；`frames/` 侧用 `cv::imencode` 而不是 `cv::imwrite` | 少一次全帧转换；两条链路同源同 swscale，色彩口径天然一致 |
| 固定 40 ms 拍点（`steady_timer` + `next_tick += 40ms`）+ 拍点无新帧时重复上一帧 + `gaps` 按 `missing` 折算 + `expect_frame_count = len + Σmissing` + step 1 补位策略 A | 全部删除：改为摄像头阻塞读线程"到达即处理"，真实时间戳进 `fps=25` 滤镜统一输出帧率；`raw.mkv` 只存真实帧（时基 `{1,1000}`），`frames/` 逐帧写、不补位；`gaps` 改为墙钟区间 | 定时器相位与摄像头相位之差会随机决定"重复/丢弃哪一帧"；按真实时间戳采样既均匀，又让 PNG 输入不含任何复制帧（§4.4、§10） |
| `align.json` 的 `frames[].host_pts_us`（仅诊断，不参与换算） | 改为 `{seq, orig_seq, wall_ms}` + 顶层 `source_fps` / `raw_frames` / `inserted_frames` | 真实时间戳现在是链路的一等公民（喂滤镜），不再是旁注 |
| `face_capture_stream` 的 `websocket_callback` 里直接 `co_spawn(g_io_context(), run(std::move(stream)))`，连接生命周期写在回调里 | 独立成 `ws_stream_runner` 类，**就地写在 `face_capture_stream.cpp` 里**（`face_capture.h` 前置声明），`websocket_callback` 只 `make_shared` + `start()` | 连接生命周期要独立于回调栈帧存活，但只有一个消费者，不值得下沉到 core 多开一对 `.h/.cpp` |
| `ws_outlet::write_fn` 只收 `(data, size)`，文本/二进制由调用方自己 `stream.binary()` 切换 | 签名加首参 `bool in_binary`，`ws_outlet` 内部按 item 记录 text/binary | 写协程是唯一能安全切换帧类型的地方（单在途），调用方拿不到这个时序 |
| `capture_session::impl` 的 `stream_` / `set_stream()` / `stream()` / `write_ws()`（早期给 WS 预留的写通路） | 全部删除，`attach()` 只接管 outlet，`write_fn` 由 `ws_stream_runner` 注入 | 两条并存的写通路没有调用者，且 `write_ws` 拿不到 text/binary 信息 |
| `core/face_capture/ws_stream_runner.h/.cpp`（曾下沉到 core 的独立一组） | 删除该组文件，类移入 `http_method/local/face_capture_stream.cpp` | 只有一个 HTTP 路由用它；core 不必知道 WS 连接怎么活 |
| 人脸检测整块（`core/face_capture/face_detector.*`、`face_status` 事件、`hello` 的 `face_detector` 字段、`align.json` 的 `face`、`POST /recordings` 的 `422 no_face` 与 `force`、`kitsu_ctx_t::face_detector_model_path_`、M0' 预研与 M3 里程碑） | 全部推迟到二期，一期不做也不预留 | 一期目标是把"录 → 推 → PNG → UE 曲线"打通，检测是独立增量（§6） |
| 摄像头选择放进 `core_set` / 环境变量 / 配置文件 | 定为 `face_capture_devices` 路由上的 `shared_ptr<face_capture_selected_device>`，`reg_c` 构造参数注入 `face_capture_stream` | `create_kitsu_local_route()` 的 `http_route_ptr` 本就没挂 `core_set`；选中值只在同一次路由注册里流转，不必惊动全局配置 |
