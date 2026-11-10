# 摄像头采集 · WebSocket 推流 · 前端控制录制 · UE 转码 方案

> 状态：方案已拍板（2026-11-05，11-06 追加"取消固定节拍"、"会话挂在 WS 上"、"选中摄像头"、"WS 只预览 / 录制显式开始 / 取消 raw.mkv"）。**采集引擎（M1/M2 + `write_png_sequence`）、WS 路由（连上只开预览、断开即关闭）、设备路由（GET 枚举 / POST 选中）、录制段路由（开始 / 停止 / 查询 / 列表 / 下载 / 删除）已实现并编译通过**，转码作业层待实现（进度见附录 A）。
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
| 3 | 丢帧补位 | **已废止**：`frames/` 由 `take.mp4` 逐帧解码得到，天然连续，不需要补位（§10 保留为历史记录） |
| 4 | UE 进程排队 | 脸捕 Commandlet **与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`**（`awaitable_queue_limitation`，默认 limit=1），不新开闩 |
| 5 | `frames/` 的保留策略 | **转码作业成功后自动删除 PNG 序列、保留 `take.mp4`；不可配置** |
| 6 | 会话与连接的绑定 | `capture_session` 是 `ws_outlet` 的**成员**，**会话生命周期 = WS 连接生命周期**：连上就打开摄像头推预览，断开就关闭。**连上不等于开录**，录制要另行 `POST /recordings`。没有进程内单例 |
| 7 | 控制通道 | **控制一律走 HTTP**：`POST /recordings` 开始一段录制、`POST /recordings/{id}/stop` 结束、`POST /transcode` 转码。**WS 是纯下行**，不接受任何命令（§3.5） |
| 8 | UE 侧源码变更 | **0**。前提是 M0 真机验证通过（§11） |
| 9 | **推流只有一条** | 本机后端、单用户，**任何时刻最多一条视频推流**。设计上**不提供第二条**：不做多连接分发、不做环形缓冲-history、不做逐连接背压（§5.3） |
| 10 | **同一时刻最多一个会话** | 会话对象只有一个，且它就是 `ws_outlet` 的成员，**不打开 WS 就不会抓摄像头**。不用查找表、不做会话池；第二条 WS 连上时旧连接因为失去 outlet 自然断开，新连接成为唯一会话（§3.3） |
| 11 | **输出帧率定死 25 fps** | 推流/`take.mp4`/UE `-FPS=25/1` 只用 25 fps，`time_base = 1/25`。**输入侧不再假设帧率**：摄像头到达即处理、带真实时间戳，由 `fps` 滤镜统一到 25 fps（§4.4） |
| 12 | **人脸检测推迟到二期** | 一期**不做**：不写检测代码、不引模型、不预留 `face_status` 事件、不做开录前的人脸校验。§6 只记二期插入点与约束 |
| 13 | **转码作业不登记任务表** | **不写 `server_task_info`、不进任务列表 UI**，只在内存注册表保活；进度只看 `GET /transcode/{job_id}` 与 WS `transcode_progress`（§7.2） |
| 14 | **`ue_project` 必填且必须是绝对路径** | 前端在 `POST /transcode` 的 body 里必须给 `.uproject` 的**绝对路径**；缺失/为空/非绝对路径/文件不存在 → `400 missing_ue_project`。**后端不做相对路径拼接、不猜默认工程**（§7.2、§7.3-8） |
| 15 | **`package_path` 可空** | 空字符串或不传 → 命令行里**不出现 `-PackagePath`**，落点由 Commandlet 自己决定；非空才校验 `/Game/...` 格式（§7.2、§7.3-9、§8.1） |
| 16 | **取消固定节拍** | 采集改为"摄像头阻塞读协程 → 到达即打 `steady_clock` 时间戳 → `post` 到采集 strand"，**没有 `steady_timer` 轮询、没有 40 ms 拍点**；帧率统一交给 `fps` 滤镜（§4.1、§4.4） |
| 17 | **不打开 WS 就不抓摄像头** | `capture_session` 由 `ws_outlet` 直接持有并随连接创建/销毁；WS 连上即 `open()`（开摄像头 + 预览推流）、断开即 `close()` 收尾。**删除进程内单例 `capture_session::instance()`** |
| 18 | **摄像头由前端先选中，WS 只负责开流** | `GET /face_capture/devices` 枚举、`POST /face_capture/devices` 选中（body `{"index":n}`）；选中值是设备路由上的**共享属性**（`shared_ptr<impl>`，clone 后仍共享），通过 `reg_c` 构造参数注入 `face_capture_stream`，WS 连上时取它传给 `capture_session::open()`。不落盘，重启回 0（§3.1、§3.2） |
| 19 | **WS 只预览、录制显式开始** | 连上 WS 只 `open()` 摄像头并推 25 fps 预览流，**不落任何文件**；`POST /recordings` 才建目录、开写 `take.mp4`，`POST .../stop` 收尾后预览继续。`capture_state` 追加 `previewing`（§3.3、§4.2） |
| 20 | **取消 `raw.mkv`，只落 `take.mp4`** | 不再 FFV1 无损录制；转码 step 1 直接从 `take.mp4` 解码 PNG，原 `mp4` 步骤删除。代价（有损输入 UE）与回退办法见 §5.4、§12 |

---

## 1. 端到端链路

```
控制面（前端 → 后端，全部 HTTP）
  POST /face_capture/recordings                    开始录制
  POST /face_capture/recordings/{id}/stop          结束录制
  POST /face_capture/transcode                     转码
  GET  /face_capture/recordings[/{id}]            状态查询（磁盘扫描，跨连接可用）

数据面（后端 → 前端，单向 WS，不接受任何命令）
  浏览器 ◄──WS── hello / avc_init / avc_fragment / transcode_progress / error
                       │
        ┌──────────────┴───────────────┐
        │  摄像头读协程 (阻塞 read)        │
        │  到达即打 steady_clock 时间戳    │
        │        │ post                  │
        │        ▼                       │
        │  后端采集 strand (单 strand)     │
        │  └─► fps 滤镜 ─┬► h264_encoder ─► 分片 ─► ws_outlet ─► 浏览器 fMP4 (预览, 一直推)
        │                │                          (单在途写, 丢非关键帧)
        │                └► take.mp4        (仅录制期写; 输出时基 1/25, pts=seq)
        │  align_recorder ───────────► align.json (seq / wall_ms / gaps, 每段录制一份)
        └───────────────────────────────┘
                       │ 前端 POST stop → 200 返回时 take.mp4 已收尾
                       ▼ 前端再 POST /transcode（独立动作）
            转码作业 transcode_job（后台协程）
              step 1  png_sequence : take.mp4 ──► png_tmp/ → frames/frame_000000.png …  (逐帧解码, 无补位)
              step 2  ue_commandlet: frames/   ──► curves/*.dhfc  (-FPS=25/1, 共用 ue 闩)
                                  └──────────► 成功即删除 frames/
```
关键点四条：

1. **控制面全在 HTTP，数据面全在 WS**。WS 只往下推帧与事件，不解析任何上行命令（§3.5）。
2. **WS 是会话本体，不是可选通道**：连上 WS 才打开摄像头，并立刻开始推预览流；**预览不落盘**。录制是显式动作（`POST /recordings`），没点开始录制之前 `session_dir` 里什么都没有。断开 WS 等于关闭会话（若在录，先收尾再关）。
3. **录制与转码是两条独立的协程链**。录制在采集 strand 上逐帧串行推进；转码在 `g_io_context()` 上用另一条协程，两者只通过 `session_dir` 下的文件交互，不共享内存对象。
4. **帧率只在出口处统一**。输入是"摄像头什么时候给就什么时候要"，带真实时间戳；`fps=25` 滤镜负责把任意输入帧率采样成 25 fps，供 `take.mp4` 与推流（§4.4）。

一句话总括：`opencv 采集 → fps 滤镜 → ①H.264 分片 → WebSocket → 实时预览`（全程），`②take.mp4（录制期）→ png_sequence → frames/ → UE Commandlet → face_curves.dhfc`。

---

## 2. 模块与文件划分

### 2.1 目录布局

`src/doodle_lib/http_method/local` 是 HTTP/WS 入口层，依赖必须下沉到 `core`/`exe_warp`。因此：

```text
src/doodle_lib/core/face_capture/          ← 新增：与 HTTP 无关的采集/编码引擎（已实现）
    face_capture_types.h                    公共结构体、enums、错误码、to_json/from_json
    media_fragment_pipeline.h/.cpp          ffmpeg 编码 + fMP4 分片产出（推流与 take.mp4）
    frame_filter.h/.cpp                     fps 滤镜包装：真实时间戳输入 → 25 fps 输出
    ws_message.h/.cpp                       下行消息结构体：ws_hello / ws_avc_init / ws_avc_fragment，
                                            基类 ws_message 带 to_json() / binary() / payload() 虚函数
    ws_outlet.h/.cpp                        单推流出口 + 连接生命周期：持有 beast WS 流，send() 投递到流执行器
                                            后 co_spawn 写出；两层丢非关键帧、关键帧封顶 2 个、写挂住 2 秒判死连接
    capture_session.h/.cpp                  会话状态机（预览 / 录制两轴）+ 摄像头读协程 + 采集处理
                                            另含 capture_session_holder（跨路由共享的单槽位）
    frame_align.h/.cpp                      align.json 记录与读取
    png_sequence_writer.h/.cpp              take.mp4 → frames/（逐帧解码，无补位）
```

一期是 **8 组源文件**（`face_detector` 随人脸检测一起推到二期，§6）。`frame_filter` 自成一组是因为它只依赖 ffmpeg 的 `avfilter`，不关心 `capture_session` 的状态机；`ws_outlet` 独立成组是因为它既要被 `capture_session` 用，又要被 HTTP 层的 WS 路由持有，放在 `capture_session` 里会让两层互相包含；`ws_message` 再拆一层是因为它**只依赖 `nlohmann::json` 与 `face_capture_types`**，不碰 asio/beast，前端协议要改时只看这一个文件。

WS 连接的生命周期（建 `ws_outlet` → attach → open 预览 → 读循环 → close 清理）已经**整体并入 `core/face_capture/ws_outlet`**：早期版本把它放在 `http_method/local/face_capture_stream.cpp` 里当 `ws_stream_runner` 类，但它既要持有 beast 流、又要持有会话、又要做丢帧策略，与 `ws_outlet` 一一对应，合起来才是一条完整的连接。现在 HTTP 路由层只剩三个 override。
src/doodle_lib/exe_warp/face_capture/      ← 新增：转码作业与 UE 编排
    face_transcode_job.h/.cpp               两步作业调度、状态机、进度
    face_ue_commandlet.h/.cpp               拼 Commandlet 参数、调 async_run_ue、解析退出码与 face_capture_log.json

src/doodle_lib/http_method/local/          ← 新增：路由
    face_capture.h                          DOODLE_HTTP_FUN_* 声明：设备 + WS 流（与 local.h 同构）
    face_capture_recording.h                会话持有者 + 录制段 5 个路由类
    face_capture_device.cpp                 设备枚举 GET + 选中设备 POST
    face_capture_stream.cpp                 WS 下行推流（纯输出，无命令处理）
    face_capture_record.cpp                 录制：POST 开始 / GET 列表 / POST 停止 / GET 查询 / DELETE / 下载视频 / 下载曲线
    face_capture_transcode.cpp              转码提交/查询/取消
    CMakeLists.txt                          （改）追加上述 .cpp
```

**没有 `capture_session_registry`**：单用户本机、同一时刻只有一个会话，注册表那一层是多余抽象。跨路由共享会话靠 `capture_session_holder`（`shared_ptr<impl>` + `mutex` 的单槽位，定义在 `core/face_capture/capture_session.h`，HTTP 层用 `using face_capture_session_holder = face_capture::capture_session_holder;` 别名沿用旧名），由 `face_capture_devices` 持有并经 `reg_c` 构造参数注入其余路由。**也没有进程内单例**：`capture_session` 是 `ws_outlet` 的成员，跟着 WS 连接一起创建、一起销毁——不打开 WS 就不会抓摄像头（§0 第 17 条、§3.3）。

`local.h` 已经有 171 行、18 个路由类，再塞 10 个会失控；把新模块集中到 `face_capture.h` / `face_capture_recording.h` 与既有 `local.h` **分开**：`kitsu.cpp` 里加两行 `#include` 即可。

改动的既有文件只有四个：`http_method/local/CMakeLists.txt`、`http_method/kitsu.cpp`（注册表）、`core/CMakeLists.txt`、`exe_warp/CMakeLists.txt`。**UE 插件与 `vcpkg.json` 都不动**。

### 2.2 命名空间

统一 `doodle::face_capture`（引擎层）与 `doodle::exe_warp::face_capture`（作业层），路由层仍是 `doodle::http::local`。

---

## 3. HTTP 与 WebSocket 路由

### 3.1 注册方式

沿用现有写法，在 `create_kitsu_local_route()`（[kitsu.cpp:307-337](E:\Doodle\src\doodle_lib\http_method\kitsu.cpp)）末尾追加：

```cpp
.reg_c<local::face_capture_devices>()            // GET 设备枚举 / POST 选中设备（同时持有 session_holder_）
.reg_c<local::face_capture_stream>(              // GET + Upgrade → WS（纯下行，只预览）
    l_rout_ptr->get_function<local::face_capture_devices>()->selected_,
    l_rout_ptr->get_function<local::face_capture_devices>()->holder_)
.reg_c<local::face_capture_recordings>(          // POST 开始录制 / GET 列表
    l_rout_ptr->get_function<local::face_capture_devices>()->holder_)
.reg_c<local::face_capture_recording>(           // GET / DELETE，{session_id_}
    l_rout_ptr->get_function<local::face_capture_devices>()->holder_)
.reg_c<local::face_capture_recording_stop>(      // POST 结束录制
    l_rout_ptr->get_function<local::face_capture_devices>()->holder_)
.reg_c<local::face_capture_recording_video>()    // GET take.mp4
.reg_c<local::face_capture_recording_curves>()   // GET .dhfc
.reg_c<local::face_capture_transcode>()          // POST 提交
.reg_c<local::face_capture_transcode_instance>() // GET / DELETE，{job_id_}
```

**会话对象由 `ws_outlet` 创建并持有**（即 WS 连接本身）：成员里有唯一的 `std::shared_ptr<capture_session> session_`，连上 `open()` 预览、断开 `close()` 清理。**但录制端点必须能拿到它**，草稿里"路由拿不到会话"的问题用 `capture_session_holder` 解决：

- `face_capture_session_holder` 定义在 `face_capture_recording.h`，内部 `shared_ptr<impl>`（`impl` = `std::mutex` + `std::shared_ptr<capture_session>` 单槽位）。
- 槽位对象放在 `local::face_capture_devices` 上，字段 `std::shared_ptr<face_capture_session_holder> holder_`；其余路由经 `reg_c` 构造参数注入同一个 `shared_ptr`。
- `ws_outlet::run()` 在 `session_->attach(shared_from_this())` 之后 `holder_->reset(session_)`；`close_impl()` 里 `holder_->reset(nullptr)`。`reset()` 带保护：**槽位里已有一个不同的活会话时不覆盖**，避免第二条 WS 连接把先来的登记顶掉。
- 仍然满足"同一时刻最多一个会话"（§0 第 10 条）：槽位只有一个。

**选中摄像头的共享属性**放在 `local::face_capture_devices` 上，字段是 `std::shared_ptr<face_capture_selected_device> selected_`；`face_capture_stream` 通过 `reg_c` 的构造参数拿到同一个 `shared_ptr`（`reg_t/reg_c` 会把参数原样透传给 `std::make_shared<T>`，见 [http_route.h:54-67](E:\Doodle\src\doodle_lib\http\http_route.h)）。**设备路由必须先注册**，否则 `get_function<>()` 取到空指针。

不放在 `core_set` 的理由：`create_kitsu_local_route()` 返回的 `http_route_ptr` 在 `kitsu_supplement.cpp:179` 只是局部变量，没挂到 `core_set`；为了一个"选中第几个摄像头"的整数去动 `core_set` 会多出一处跨模块耦合。

`face_capture_selected_device` 与 `face_capture_session_holder` 内部都是 `std::shared_ptr<impl>`：路由对象每请求 clone 一次，**值成员复制的是快照，写进去的东西下一个请求就丢**；`shared_ptr` 成员的副本指向同一个 `impl`，共享才成立。加锁是因为 HTTP handler 在 `g_ioc` 线程池上跑，而 WS 协程在另一条 strand 上。

稳定性检查：**10 个路由类、12 个端点**（`devices` 一条 URL 两个方法）。（早先还考虑过把 `/stream` 拆成 open/close 两条，已否决——WS 的一生由 TCP 决定，不需要额外端点。）

路由模板必须**一个类一条 URL**（`DOODLE_HTTP_ROUTE` 只接受一个路径字面量），所以 `/recordings/{id}` 与 `/recordings/{id}/stop` 是两个类。`kitsu.cpp:312-333` 里每个排队项都是这个形态，照抄即可。
未注册的路由运行期不可达，这是新增 HTTP 功能唯一容易漏的一步。

### 3.2 接口清单

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/api/doodle/local/face_capture/devices` | 枚举摄像头：`{selected, devices:[{index, name, backend, width, height, selected}]}` |
| POST | `/api/doodle/local/face_capture/devices` | **选中摄像头**。body `{"index": 0}`；成功返回 `{selected}`；body 缺整数 `index` → `400`，负值 → `400`，设备不存在（越界/已拔出）→ `404` |
| GET | `/api/doodle/local/face_capture/stream/{session_id_}` | **WS 升级点 + 会话本体**（`has_websocket() = true`，GET + `Upgrade` 才升级）。连上即创建 `capture_session`、打开摄像头、**推预览流（不落盘）**；断开即关闭会话（若在录则先收尾）。路径里的 `session_id_` 只用于前端日志与路由占位，**不参与查找**，真实 `session_id` 在 `hello` 里回传。**这条只下发视频与状态，不接受任何命令**（§3.3） |
| POST | `/api/doodle/local/face_capture/recordings` | **开始录制**。无 body；新建 `session_dir` 并开写 `take.mp4`；返回 `{session_id, state:"recording", width, height, fps, frames, duration_ms, source_fps, truncated}`。没有预览连接 → `409`，已在录 → `409`，`take.mp4` 建不出来 → `422` |
| GET | `/api/doodle/local/face_capture/recordings` | 历史会话列表（磁盘扫描 `session_dir`） |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}` | 单会话状态、帧数、丢帧区间、产物清单 |
| POST | `/api/doodle/local/face_capture/recordings/{session_id}/stop` | **结束录制**。同步等 `take.mp4` 收尾（≤ 2 秒）后返回 `{state, frames, duration_ms, truncated, source_fps}`；`session_id` 与当前录制段不符 → `404`，没在录 → `409`。**收尾后预览继续**，不关摄像头 |
| DELETE | `/api/doodle/local/face_capture/recordings/{session_id}` | 删除会话目录与 `/Game/.../<id>` 备用包；进行中返回 `409 recording` |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}/video` | `take.mp4`（`http_body_file_t`，支持 Range） |
| GET | `/api/doodle/local/face_capture/recordings/{session_id}/curves` | `face_curves.dhfc` 下载（`set_attachment()`） |
| POST | `/api/doodle/local/face_capture/transcode` | 提交转码作业 |
| GET | `/api/doodle/local/face_capture/transcode/{job_id}` | 查询作业（含两步子状态） |
| DELETE | `/api/doodle/local/face_capture/transcode/{job_id}` | 取消作业（正在跑 Commandlet 时置取消标志） |

**控制面全部是 HTTP，WS 是纯下行且是会话本体**。前端不需要维护"命令发出去没回应"的重试逻辑：开始/结束录制都是一次 POST，成败直接看状态码与 body；WS 断了就意味着会话已经结束（在录时 `close_impl()` 会先收尾 `take.mp4`）。

路由类一律 `DOODLE_HTTP_FUN_C(xxx, local_http_fun)`（要拿登录 token）或 `DOODLE_HTTP_FUN_C(xxx, ::doodle::http::http_function)`（纯本机操作）。**注意 clone 语义**：`http_function.cpp:74` 每个请求都会 `clone()` 一次路由对象（`std::make_shared<Self>(*this)`），所以路由类的成员只能是可拷贝的值或 `shared_ptr`，**不能放 `cv::VideoCapture`、`FaceDetectorYN`、`std::unique_ptr`**。

> **会话对象的归属**：会话由 `ws_outlet`（`core/face_capture/ws_outlet.h`）创建并持有（`std::shared_ptr<capture_session> session_`）；它把裸指针登记进 `capture_session_holder`（§3.1），录制端点从那里取。**本机只有一个会话，所以不存在查找表。**

### 3.3 WS 协议

WS 升级线的关键事实（`http_session_data.cpp:157-181`）：只有 `GET` + `websocket::is_upgrade()` + `callback_->has_websocket()` 三者同时成立才升级；`websocket_init(session_data_ptr)` 在 `async_accept` **之前**同步调用，`websocket_callback(stream, session_data_ptr)` 在 accept **之后**调用且拿不到错误码——连接生命周期只能靠第一次 `async_read` 失败感知。

**实现已落地**：路由类 `local::face_capture_stream`（`http_method/local/face_capture.h` + `face_capture_stream.cpp`）只做转发，连接管理由 `core/face_capture/ws_outlet` 承担（消息体在 `core/face_capture/ws_message`）：

- `websocket_callback` 里 `runner_ = std::make_shared<face_capture::ws_outlet>(std::move(in_stream), selected_ ? selected_->index() : 0, holder_); runner_->start();`，别的一行不写。第二个参数是**连接建立那一刻**的选中设备，之后改选择不影响进行中的会话。
- `ws_outlet` 直接以 `boost::beast::websocket::stream<tcp_stream_type>` 构造，**执行器取自流本身**（`stream_.get_executor()`），不再自建 strand：所有连接状态（`closing_` / `key_backlog_` / `pending_since_`）都只在该执行器上访问，跨线程只暴露 `closed_` / `writing_` 两个 `atomic_bool`。
- **`ws_outlet` 直接持有会话**：构造时 `session_ = std::make_shared<face_capture::capture_session>(capture_session::default_root());`。会话不共享、不查表、不做单例——每条 WS 连接一个自己的会话。`capture_session::impl::outlet_` 是 **`weak_ptr<ws_outlet>`**（不是 `shared_ptr`），否则会话与连接互相持有成环。
- `run()` 的顺序是：`session_->attach(shared_from_this())` → `holder_->reset(session_)` → `co_await open_capture()`（`session_->open(capture_option{.device_index_ = device_index_})`；失败只记 WARN 不掐断连接）→ `co_await read_loop()` → `co_await close_impl()`。**先 attach 再 open** 是硬顺序：`open()` 一开摄像头就会开始出帧，此时 outlet 必须已经挂好。
- `close_impl()` 的顺序是：`co_await close_capture()`（`session_->close()`，内部若正在录制会先收 `take.mp4` 与 `align.json`，再关摄像头）→ `holder_->reset(nullptr)` → `mark_closed()` → `async_close`。**close 在断开之前**，否则最后几帧没处可写。
- 发送走协程：`send(std::unique_ptr<ws_message>)` 对调用方是同步的，内部 `post` 到流执行器后 `co_spawn` 一个 `write_message` 协程。`write_message` 先把 `to_json().dump()` 作文本帧写出，`binary()` 为真时再写一帧二进制；`stream_.binary()` 的切换与复位都在这条唯一写路径里做。`co_spawn` 绑 `app_base::Get().on_cancel` 槽（`consign(detached, shared_from_this())`），进程取消时协程随之退出。
- **消息全是结构体**（`ws_message.h`）：基类 `ws_message` 有三个虚函数——`to_json()`（序列化）、`binary()`（是否再跟一帧二进制）、`payload()`（二进制体）；派生 `ws_hello` / `ws_avc_init` / `ws_avc_fragment`，采集侧只构造对象、不拼 JSON。`transcode_progress` / `error` 两个事件一期没有发送方（转码作业未实现、摄像头失败只记 WARN），等它们落地时各加一个派生类即可。

`face_capture_stream` 的约定：

- `websocket_init`：当前是空实现（不校验 token，见 §14 仍未定第 2 条）。路由成员里只有 `runner_` / `selected_` / `holder_`（都是 `shared_ptr`，clone 安全），**不存会话**。
- **连上只开预览，不开录**：`hello` 里的 `state` 是 `open()` 之后的状态，即 `previewing`（摄像头打不开时是 `idle` 并在 `error` 事件里说明原因）。**录制必须由前端 `POST /recordings` 显式开始**，这是本方案与早期草案最大的差别。
- 接入顺序固定为 **`session_->attach(outlet)` → `open_capture()`**。`hello` 不在连接建立时发：`open()` 末尾由采集侧 `send(std::make_unique<ws_hello>(...))` 发一次 `hello`（带真实的 `session_id`/`width`/`height`/`state`），前端第一帧事件必然是 `hello`。
- **`hello` 是唯一的状态快照，之后没有任何状态推送**：连接存续期间不再下发 `capture_state` 事件。前端判断动作是否生效一律以 HTTP 响应（状态码 + `capture_status` 的 `state`）为准；需要重新取状态就再发一次 `GET /recordings/{id}`，或重连拿新的 `hello`。
- **断开即结束会话**：`close_impl()` 先 `close()` 再 `mark_closed()` 并断开。若当时正在录制，`close()` 内部等价于一次 `stop()`，`take.mp4` 与 `align.json` 照常落盘。前端要"结束但保留画面"就得自己重连（新连接 = 新会话）。
- 第二条连接 = 第二个会话：旧连接此时正卡在它自己的 `close()` 里，摄像头被 `VideoCapture::release()` 唤醒收尾后，新连接才拿得到设备。**同一时刻仍然只有一条推流**（§0 第 9 条），但"接管"退化成"旧连接被关掉、新连接重开"。
- 写端由 `ws_outlet` 保证**单在途**：`post` + `co_spawn` 落在同一个执行器上，同一时刻只有一个 `async_write` 在跑；`writing_` 只是给采集侧看的跨线程快照。
- 读端**只做保活与断连检测**：收到任何帧都忽略，靠 `async_read` 的失败感知连接已经断掉（从而关闭会话）。**没有命令解析、没有 `post` 到采集 strand 的分派逻辑**——控制面已经全在 HTTP（§3.5）。
- 降级：若客户端协商不到 `Sec-WebSocket-Protocol`，仍按普通 WS 走；诊断信息统一用 JSON 事件下发。
#### 分片装载格式

一个帧分片 = 一条 text 事件 + 紧随的一条 binary 消息（两帧，前端按到达顺序配对）。text 里的 `seq` 用于配对：

1. 前端收到 `avc_init` → `SourceBuffer.appendBuffer(init)`；
2. 前端收到 `avc_fragment` → 等下一帧 binary → `appendBuffer(moof+mdat)`；
3. 服务端**可能跳号**（§5.3：WS 写不动时丢分片），但**只会在关键帧边界重新同步**：丢掉的分片之后，下一个 `keyframe=true` 的分片一定完整送到。前端不需要实现跳片逻辑，只需在收到新的 `avc_init` 时清空 buffer。

服务端保证同一连接上 `seq` 严格递增（可能不连续）。**预览期与录制期各用一套帧号**：预览期 `seq` 是独立的预览计数，录制期 `seq` 与 `take.mp4` 的帧号一致，并通过 `align.json` 的 `seq` 字段一一对应（§4.4），这也是"录制到第几帧"的唯一口径。

#### 下行事件（text JSON + 可选紧随的 binary）

| event | 时机 | 负载要点 |
| --- | --- | --- |
| `hello` | 连接建立时一次（`open()` 成功后由采集侧下发） | `{session_id, state:"previewing", width, height, fps:25}`，连接存续期间的唯一状态快照 |
| `avc_init` | 每次新会话开始录制 | `{bytes, seq_begin, seq_end}`（fMP4 的 `ftyp+moov`），紧随一个 binary 帧装 init 段 |
| `avc_fragment` | 每个分片 | `{seq, keyframe, size}`，紧随一个 binary 帧装 `moof+mdat` |
| `transcode_progress` | 转码阶段推进（≥1 Hz 节流） | `{job_id, step:"png_sequence"/"ue_commandlet", state, progress, detail}` |
| `error` | 任何失败 | `{code, message}`，`code` 取自统一错误码表（`to_string(error_code_t)`） |

一期就这 5 个事件。**没有 `face_status`**：人脸检测在二期（§6），一期前端拿不到、也不需要拿人脸框。**没有 `capture_state`**：状态只在 `hello` 里给一次，后续以 HTTP 响应为准（§3.5）。

服务端侧每个事件都是一个 `ws_message` 派生结构体：`to_json()` 产出上表的 text 帧，`binary()` 决定是否再跟一帧 `payload()`。所以"text + binary 两帧"是发送器的固定形状，不靠调用方记得先发描述再发数据。

所有事件都是**纯下行**。`transcode_progress` 由后台作业广播到当前唯一的 WS；**WS 没连时没有会话也没有作业可跑**，所以不存在"事件丢了"这回事；跨连接的作业状态用 `GET /transcode/{job_id}` 查（`GET /recordings/{id}` 同理）。

#### 上行：什么都没有

WS 上**不定义任何上行命令**。前端不需要发 `ping`（浏览器原生 WS 没有心跳 API，beast 侧也不做空闲超时：单用户本机连接不该被服务端主动断开）；连接是否活着由浏览器自己感知，重连就是重新 `GET /stream/{session_id}`。

#### 控制面（HTTP）

| 动作 | 请求 | 成功返回 | 失败 |
| --- | --- | --- | --- |
| 开始录制 | `POST /recordings`（**显式动作**，连 WS 不会自动开始） | `200 {state:"recording", session_id, width, height, fps:25, frames:0, duration_ms:0}` | `409`（没有活跃 WS 会话）、`409`（已在录制）、`422`（`take.mp4` 建不出来） |
| 结束录制 | `POST /recordings/{session_id}/stop` | `200 {state:"previewing", frames, source_fps, duration_ms, truncated}` | `409`（当前没有在录制）、`404`（`session_id` 不是当前录制段） |
| 查询状态 | `GET /recordings/{session_id}` | `{state, session_id, frames, duration_ms, gaps, artifacts:{take_mp4,align_json,frames,curves}, width, height, fps, source_fps, truncated}` | `404` |
| 提交转码 | `POST /transcode`，body 见 §7.2 | `202 {job_id, state:"queued"}` | `409 job_running`、`400 missing_ue_project`（`ue_project` 缺失/为空/非绝对路径/文件不存在）、`400 bad_package_path`（非空但不是合法包路径） |

要点：

- **WS 连接只负责预览**：`GET /stream/{session_id}` 升级成功后，`ws_outlet::run()` 调 `session_->open()`（只做 `cv::VideoCapture` 打开、分辨率回读、`h264_fragmenter` + `fps` 滤镜的建立），随后一直把 25 fps 的 H.264 分片推给前端。**这一路不写任何磁盘文件**。摄像头打不开时连接不断，只发 `error` 事件让前端看见原因。
- **录制是显式动作**：`POST /recordings` 才建 `session_dir` 与 `take.mp4`。没有活跃 WS 会话时返回 `409`。
- **结束录制是同步的**：`stop()` 内 `filter_->flush()` → 等推流排空 → `mp4_writer_->finish()`（`av_write_trailer`）→ 写 `align.json`，上限 2 秒。返回时 `take.mp4` 已完整，前端可以直接去拉 `/video` 或提交转码，不需要轮询。**收尾之后预览继续**，摄像头不关。
- **断开 WS 走 `close()`**：若正在录制，内部等价于先 `stop()` 再关摄像头，只是没人接返回值。
- 状态变化**不通过 WS 推送**，前端以 HTTP 响应体里的 `state` 为准；WS 只在连接建立时用 `hello` 给一次状态快照。
- 重复调用是幂等的：已在 `recording` 时再发 `POST /recordings` 返回 `409`；没在录时发 `stop` 也返回 `409`。
- **同一连接可以录多段**：每次 `POST /recordings` 新生成一个 `session_id` 与目录，上一段的 `take.mp4` 留在自己的目录里。

### 3.4 为什么不用 socket.io

`/events` 命名空间的 socket.io 广播（`socket_io::broadcast`）适合低频状态广播，但它的一次写只装一段 text 和若干 binary，且广播语义是"发给所有连该命名空间的客户端"——**本机只有一个前端、只有一条推流，广播语义纯属多余**。视频分片还要自己控制"写不完时怎么办"（§5.3），在广播模板里做不了。所以推流走独立 WS；`/events` 仅用于列表页刷新这类可选的低频事件。

### 3.5 为什么控制不走 WS

WS 传命令的代价是**全双工状态的维护**：命令要有 id、要等 ack、超时要知道是"服务端没处理"还是"连接断了"、断线期间发出去的命令要不要重放。这些都是为了解决"网络不可靠"才需要的机制。而这里的现实是：**后端就在用户本机，被前端 App 拉起，HTTP 走 127.0.0.1**，请求-响应模型没有不确定性——POST 返回 `201` 就是真的开录了，返回 `409` 就是真的没开。

所以：

| 关注点 | WS 命令 | HTTP POST（采用） |
| --- | --- | --- |
| 知道动作成没成 | 等下一条状态推送，超时要自己定 | 看状态码与 body，浏览器原生支持 |
| 断线期间发命令 | 需要客户端队列与重放策略 | 请求本身就失败了，前端直接提示重试 |
| 调试 | 要开 WS 面板看帧 | `curl` 一行就能复现 |
| 服务端实现 | 读协程里解析 + `post` 到 strand + 回执状态机 | 一个 handler，返回 `awaitable` |

结论：**WS 只做单向的帧与事件下发**，是"监控画面的管道"，不是"控制通道"。控制全在 HTTP。

---

## 4. 采集引擎

### 4.1 线程与执行器

- **一条 `boost::asio::strand`（`make_strand(g_io_context())`，实现里是 `capture_session::impl::strand_`）承载全部采集步骤**：收帧、`fps` 滤镜、编码、写分片、推流、（录制期）写 `take.mp4`。`avcodec_send_frame` 是阻塞调用，必须 `post`/`co_spawn` 到 strand 上跑，**绝不能直接在 io 线程里同步阻塞**，否则整个 HTTP 服务卡住。
- **摄像头读帧在一条独立协程上**（实现里是 `capture_session::impl::read_loop()`，由 `co_spawn(g_io_context(), ...)` 起）。`cv::VideoCapture::read()` 会阻塞到有帧才返回，它跑在 io 线程池的某一个线程上，**不占采集 strand**，`fps` 滤镜与编码可以稳定运行。
  - 读协程只做两件事：`*cap_ >> mat`，然后 `boost::asio::post(strand_, [mat, now=steady_clock::now()]{ on_capture(mat, now); })`。**时间戳在 `>>` 返回的一瞬间打**，这就是"这一帧到达的时刻"，是整条链路的有效时间。
  - 读协程 `co_spawn` 时绑 `app_base::Get().on_cancel` 槽，循环条件是 `(co_await this_coro::cancellation_state).cancelled() == cancellation_type::none`；进程退出时随取消信号结束。
- **"到达即处理"，没有固定节拍**：不再有 `steady_timer` 轮询、不再有 `next_tick`。摄像头出多少帧就 `post` 多少次，帧率差异全部交给 `fps` 滤镜（§4.4）。
- **两条停止路径**：`stop()`（结束录制）**不停读协程**，只 `filter_->flush()` → 等推流排空 → 收 `take.mp4` → 写 `align.json`，预览继续；`close()`（断连）先 `cap_->release()` 唤醒阻塞中的 `read()` → 读协程看到 `read_stop_` 退出并置 `read_finished_` → 用 `steady_timer` 轮询（≤ 2 秒，`k_stop_drain_timeout_ms`）等它退出 → 收 `take.mp4`/`fragmenter_`/滤镜 → 关摄像头。读协程 2 秒内退不出就记 `truncated`，**绝不无限期阻塞 HTTP 请求**。
- 本轮**不采集音频**，`take.mp4` 不带音轨。**注意**：UE 侧 `bSkipTongueSolve` 默认就是 `true`（`MetaHumanFaceTypes.h:51`），所以无音轨与 UE 的默认参数天然一致，不需要传任何额外开关。

### 4.2 会话状态机

```
```text
stream_state:  idle ──WS 连上 open()──► previewing ──WS 断开 close()──► idle

record_state:  idle ──POST /recordings start_record()──► recording ──POST /stop、WS 断开──► stopping ──收尾完成──► stopped
                                                              ▲                                    │
                                                              └──── 再次 POST /recordings ─────────┘
```

对外只暴露一个 `capture_state`（HTTP 响应体里的 `state`，以及连接建立时 `hello` 的那一次快照），映射规则：`record_state` 不是 `idle` 就取它，否则取 `stream_state`。所以状态序列是 `idle → previewing → recording → stopping → (stopped) → previewing`：

- **预览态是新增的**：`previewing` 表示"摄像头开着、画面在推、但没有落盘"。这是本次改动唯一的契约新增值，`capture_state` 枚举里追加了 `previewing = 4`（原有 0–3 不变）。
- `stopped` 是**录制完成后的一次性状态**：`stop()` 返回时 `record_state` 已是 `stopped`，但下一次 `start_record()` 立刻回到 `recording`。`GET /recordings/{id}` 对历史会话一律报 `stopped`。
- `stopping` 是**短暂中间态**：`stop()` 把 `record_state` 置成 `stopping`，然后**在同一个协程里**等推流排空（≤ 2 秒）→ 收 `take.mp4` → 写 `align.json` → 置 `stopped`，最后才返回。这段时间 HTTP 请求一直挂着，前端要靠自己的 pending 态与按钮禁用提示用户"正在收尾"，响应超时须大于 2 秒；不上报 `stopping` 事件，所以前后端都不必为这两秒额外做时序同步。
- 等推空**必须用 `co_await timer.async_wait()` 轮询 `outlet_->idle()`**，不能用同步的 `timer.wait()`：`stop()` 与写协程都在 io 线程池上，同步阻塞会占死线程，写协程抢不到执行权，结果是真的空等满 2 秒。
- `open()` 时若仍在 `recording`/`stopping` → 抛 `doodle_error`（`409`）；已在 `previewing` 则直接返回（幂等）。`start_record()` 在未 `open` 时 → 抛 `409`，在 `recording`/`stopping` 时 → 抛 `409`；否则先 `close_record()` 收拾上一段，再建新的。`stop()` 在没在录时 → 抛 `409`。
- **会话对象只有一个，由 `ws_outlet` 创建持有**（§3.3），裸指针登记在 `face_capture_session_holder` 供录制端点使用：不存在跨连接的会话对象，所以"设备互斥"退化成两层——进程层面由 `ws_outlet` 的持有关系保证，对象层面由状态机不许重入保证。
- `session_id` 在 `start_record()` 成功时生成（`uuid`），`session_dir = core_set::get_set().get_cache_root("face_capture/sessions") / session_id_str`。**每段录制一个 `session_id`**，同一条 WS 连接可以录多段。
- 同一个 `capture_session` 上 `stopped` 之后再 `start_record()`：**不换对象，只换 `session_id` 与 `session_dir`**。上一段的 `take.mp4`/`align.json` 已经落在旧目录里，不迁移、不覆盖。
- WS 断开走 `close()`：在录时内部先等价执行一次 `stop()`，只是返回值没人接。

### 4.3 取帧与像素约定

- `cv::VideoCapture(index, cv::CAP_DSHOW)`（Windows 首选 DSHOW，失败回退 MSMF；DirectShow 更容易设置分辨率/帧率并在某些设备上更稳）。
- 请求 `CAP_PROP_FRAME_WIDTH/HEIGHT` 后**必须回读实际值**；帧率**不回读也不采用**——一律按 §4.4 固定 25 fps。设备若不支持 25 会给出它自己最接近的帧率，由我们的定时器抽帧/补帧吃掉差异。
- OpenCV 输出 BGR。**三条色彩口径，都要写死在实现里**：
  - **推流 / `take.mp4`（编码链）**：`cv::Mat` 是 BGR24，**直接**用 `av::VideoFrame::wrap(..., AV_PIX_FMT_BGR24, ...)` 交给 `av::VideoRescaler` → `AV_PIX_FMT_YUV420P`。**不做 `cvtColor(BGR→RGB)` 中转**——多一次全帧转换只是多花 CPU，`swscale` 本身就有 BGR24 输入。
  - **`frames/`（step 1）**：ffmpeg 解码 `take.mp4` → `av::VideoRescaler` 到 `RGB24` → `cv::Mat`（`CV_8UC3`，**不要**再 `cvtColor`）→ `cv::imencode(".png", ...)`（OpenCV 把 `CV_8UC3` 当 RGB 编码）。
  - **颜色来源**：swscale 默认按 `SWS_BICUBIC` 做 BGR↔YUV，`VideoRescaler` 用默认 flags（`SwsFlagAuto`）。**因为两条链路同源（同一份 `cv::Mat` BGR24、同一个 swscale），口径天然一致**，`take.mp4` 与 `frames/` 的逐像素比对（M4 验收）不应有系统偏差；若实测平均色差 > 2，先查是不是有人在中途插了 `cvtColor`，再考虑统一 `colorspace/color_primaries/color_trc`（都设成 `bt709` + `tv` range）。
- 正立与 sRGB 是 UE 求解链的硬要求（`FHyprsenseRealtimeNode` 只在正立画面上工作），摄像头驱动一般不做旋转，**画面翻转只允许在前端做，不进后端**。
- 像素格式统一到 `AV_PIX_FMT_YUV420P`（H.264 通用选择）；奇数分辨率会被 libx264 拒绝，`POST /recordings` 时把宽高向下取偶。

### 4.4 时间轴：摄像头到达时间 → `fps` 滤镜统一成 25 fps

**输出侧只有一个帧率：25 fps，`AVRational{25, 1}`，`time_base = 1/25`。** 推流、`take.mp4`、UE 的 `-FPS=25/1` 全用它。**输入侧不再假设任何帧率**：摄像头给多少、什么时候给，就是多少。

**两条时间轴**：

```text
摄像头线程(阻塞读)
  └─ 到达瞬间打 steady_clock 时间戳 → post 到采集 strand
       └─ fps 滤镜      : buffer 输入 time_base=1/1'000'000, pts=到达微秒 → fps=25
            └─ take.mp4 + WS fMP4 : 时基 {1,25}, pts = seq = 该链路的输出帧号
```

| 环节 | 时间基准 | 说明 |
| --- | --- | --- |
| 摄像头到达 | `steady_clock`，转成"自录制开始的毫秒" `wall_ms` | 读协程 `read()` 返回的瞬间打；**这是唯一的事实时间** |
| `fps` 滤镜输入 | `{1, 1'000'000}`，`pts = wall_ms × 1000` | 滤镜按时间轴采样，独此一处做帧率换算 |
| `take.mp4` / 推流 | `{1, 25}`，`pts = seq` | `seq` 由滤镜输出计数，连续、无空洞 |

- **为什么不再固定节拍**：固定 40 ms 采样时，"重复/丢弃哪一帧"由定时器相位和摄像头相位之差决定，抖动随机；改成"到达即处理 + 按真实时间戳采样"后，重复帧会被均匀地铺在时间轴上，`fps` 滤镜挑的是时间上最靠近目标时刻的那一帧。
- **`fps` 滤镜是采样器，不是插值器**：25 fps 输出里必然存在重复帧，它只改变"哪一帧被重复、重复在哪里"，不创造运动。真正的顺滑只能靠摄像头原生 25 fps 或 `minterpolate`（720p 单帧 1.5–3 秒，实时不可行，已否决）。低光长曝光造成的运动模糊也无法用滤镜消除。
- **`frames/` 来自 `take.mp4`，也就是滤镜输出**：帧数 = 录制时长 × 25（含重复帧），`frame_%06d.png` 按下标连续编号，**没有补位逻辑也天然连续**。这与"去掉 raw.mkv"的取舍一致：UE 求解输入就是前端看到的同一条流。
- **预览期与录制期各用一个帧号**：预览期用内部 `preview_seq_` 给 `fragmenter_` 打 pts；`start_record()` 把 `seq_` 与 `preview_seq_` 归零、重建 `fragmenter_`（新 `init_segment` 通过 `avc_init` 重发），因此 `take.mp4` 与推流在录制开始时都从 0 起，前端收到新的 `avc_init` 清 buffer 即可。
- **滤镜不可用时硬失败**：`fps=25` 建不出来（`av::Filter{"fps"}` 为空、graph 配置失败、尺寸非法）时 `frame_filter` 构造函数直接抛 `doodle_error`，`capture_session::open()` 以失败返回，不再降级为原始帧直通。这些失败全部来自 FFmpeg 构建配置/运行期环境而非业务逻辑，带着错误的帧率跑完整条链路只会产出时间轴错误的 `take.mp4`，比直接失败更糟。
- **`gaps` 语义收窄为"摄像头静默"**：摄像头连续 > 1 秒没有返回帧（`k_silent_timeout_ms`）时，用 `{wall_ms_begin, wall_ms_end, duration_ms}` 记一条墙钟区间。它**不参与任何帧号折算**，只是告诉下游"这段时间摄像头哑了"。
- **无界帧率 → 有界队列**：若读协程无界 `post`，摄像头 60 fps、strand 卡住时会内存暴涨；实现里 strand 侧按 `k_max_capture_queue_frames`（120 帧 ≈ 2 秒）限制积压。
- `POST /recordings` 的 body 里**没有 `fps` 字段**：前端没有选择余地。`GET .../recordings/{id}` 与 `align.json` 里有 `"fps": 25`（输出帧率）与 `"source_fps"`（实测摄像头帧率），都是只读信息。
- 时长口径：`duration_ms` = 录制帧数 × 40 ms（`fps` 滤镜的输出时间轴）；`align.json` 的 `raw_frames` 记录录制期实际写入 `take.mp4` 的帧数（等于 `frames_` 长度），`inserted_frames` 恒为 0（不再存在"真实帧 vs 输出帧"的差）。

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

> `ffmpeg_video` / `ffmpeg_video_resize` 是**文件到文件**的成品转码工具（输入 `FSys::path`、输出 `FSys::path`、`process()` 同步跑完），拿不到逐帧回调，**不能复用来做推流**；`take.mp4` 由 `media_fragment_pipeline` 的 `mp4_writer` 逐帧写入，同样不复用它。

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
camera → (fps 滤镜) ─┬─► media_fragment_pipeline(推流编码器) ─► ws_outlet ─► 那一条 WS  (一直推)
                     └─► take.mp4 编码器                                 (仅录制期)
```

- `ws_outlet` 就是当前绑定的那条连接的写接口（`shared_ptr`，可为空——没连 WS 时推流编码直接**不做**，省 CPU）。
- **线程模型（硬约束）**：`ws_outlet` 的**执行器就是它持有的 beast 流自己的执行器**（`stream_.get_executor()`，背后是 `g_ioc` 线程池）。所以"发帧"（采集 strand 上调用 `send()`）与"写帧"（流执行器上跑的 `write_message` 协程）**没有同线程假设可用**，只能靠"所有连接状态只在流执行器上访问 + 跨线程只读 `atomic_bool`"来串行化。
- **怎么做到**：`send(std::unique_ptr<ws_message>)` 内部 `boost::asio::post(exec_, ...)` 投递到流执行器，再在同一个执行器上 `co_spawn` 出 `write_message` 协程。`closing_` / `key_backlog_` / `pending_since_` / `read_buffer_` 全部只由执行器触碰，不需要 mutex，也不需要 TBB 并发容器。
- **查询走 atomic**：`closed_`（`is_attached()`）、`writing_`（`idle()` / `pending_frame()`）两个 `std::atomic_bool` 专门给采集侧查询用，只是快照，不参与执行器内的决策。
- **顺序要求**：采集侧**必须先 `attach()` 再 `send()`**；`ws_outlet::run()` 的顺序就是 `session_->attach(shared_from_this())` → `holder_->reset(session_)` → `open()`，`hello` 由 `open()` 末尾发出。
- **写不动就丢（两层判定）**：`send()` 调完即返回，不排队。
  - **第一层（采集侧）**：`drain_filter_frames()` 里非关键帧若 `outlet->pending_frame()` 为真——即上一次写还没完成——直接 `continue` 丢弃；关键帧不丢。
  - **第二层（执行器内）**：`write_message` 入口再判一次：非关键帧且 `pending_frame()` → `co_return`；关键帧则看 `key_backlog_`，已满 `k_max_pending_keys`（2）就丢弃。
- **关键帧封顶**：关键帧不被上面两层丢弃（前端要靠它重同步），但会 `++key_backlog_`，写出后 `--`；封顶 2 个，超出的直接丢——旧关键帧没有保留价值，前端拿到最新的就能重同步。
- **死连接判定**：`pending_since_` 记录在途写的开始时刻；若下一次 `send()` 投递到执行器时发现它已经超过 `k_stop_drain_timeout_ms`（2 秒）没结束，判定连接已死（前端不读、OS 缓冲又没写满时 `async_write` 会一直挂着），`mark_closed()` 之后所有 `send()` 短路。
- 为什么不需要更复杂的背压：单用户本机 + 同机前端，WS 走 loopback，单帧分片 10–60 KB，写满窗口的概率极低；偶发丢片最多让画面卡一下。**预览丢片不影响 `take.mp4`**：两者是两条独立编码器，各自从同一批滤镜输出帧取图，`take.mp4` 只取决于编码器是否跟得上，与 WS 写不写得动无关。
- 这也意味着**§3.3 里不需要 `gap` 标记**：丢片时前端只会停在上一帧，下一个关键帧到达自然恢复（GOP = 1 秒）。
- 页面切后台导致 WS 停读：`async_write` 挂着 → 非关键帧全被丢 → 2 秒后 `mark_closed()`；**录制链不受影响**（`take.mp4` 是独立编码器）。

### 5.4 录制侧（take.mp4）

- **`take.mp4` 是唯一的录制产物**（除了 `align.json`）。编码器与推流同用 `media_fragment_pipeline` 的 H.264 参数（libx264 `veryfast`/`zerolatency`、`crf 23`、`keyint 25`），封装成普通 mp4（`movflags` 不加 `frag_keyframe`），时基 `{1,25}`、`pts = seq`（§4.4）。
- **只在 `recording` 期间写入**：`start_record()` 建编码器，`stop()` / `close()` 收尾。预览期 `mp4_writer_` 为空，`emit_output_frame()` 只推流不落盘。
- **为什么不再保留 raw.mkv**：早先的设计把 raw.mkv（FFV1 无损）当"唯一权威来源"，好处是 PNG 序列逐帧无损、帧数与摄像头真实帧数一致。改成一录即出 `take.mp4` 后，收益变成：磁盘不再写两遍（原来 1 分钟约 0.5–0.9 GB 的无损文件完全消失）、转码少一步（step 1 直接从 `take.mp4` 解码，不再先解 raw 再编码 mp4 回放）、前端"录完即能看"（`/video` 直接就是录制产物）。代价是 **UE 求解输入变成有损 H.264**，见 §12 对应风险行。
- **落盘不单开线程**：`av_interleaved_write_frame` 在 `take.mp4` 这条链上每秒只有 25 帧，且与推流共用同一次滤镜输出，直接在采集 strand 上写即可；写失败 `catch` 后 WARN 并置 `truncated_ = true`。
- **短尾**：`stop()` 到达时可能还有几帧在滤镜里排队，收尾顺序固定为 `filter_->flush()` → 等推流排空 → `mp4_writer_->finish()`（`av_write_trailer`）→ 写 `align.json`，上限 2 秒。**这个等待发生在 HTTP 请求内**，所以 `stop` 的响应时间上限就是 2 秒，前端超时要设得比它大。
- `raw_writer` / `ffv1_raw_writer` / `raw_sink_thread` 的代码**保留但不再被 `capture_session` 使用**（`media_fragment_pipeline` 里仍可被单独构造），留作将来若要恢复无损录制时的现成实现。

---

## 6. 人脸检测（**二期，本方案不实现**）

**拍板：人脸检测整体移出本方案，本方案不写任何检测代码、不引入任何模型、不新增 `face_status` 下行、不做开录前的人脸校验。** 一期交付的是"能录、能推、能转、UE 能出曲线"，画面里有没有正脸由人在前端自己看。

之所以留这一节，是因为下一期要在**同一个采集循环里**插入检测，位置与约束得先钉住，免得二期把采集节奏改乱：

### 6.1 插入点（二期需要遵守的边界）

- 检测必须挂在 §4.4 的"摄像头到达"那一条路径上（`on_capture` 之后、`fps` 滤镜之前或之后二选一，固定选后者以免干扰时间戳），**不能自带定时器**，也不能反过来拖慢采帧。做法是"这一帧跑不完就跳过去"，而不是"等检测跑完再出帧"。
- 检测失败、超时、模型缺失一律**不影响录制与推流**：`take.mp4` 与 `frames/` 的产出与有没有检测结果完全无关。
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

### 7.1 两步流水（顺序固定）

| step | 输入 | 输出 | 实现 | 完成后 |
| --- | --- | --- | --- | --- |
| 1 `png_sequence` | `take.mp4`（录制产物，H.264 25 fps） | `frames/frame_{n}.png`，`n` 从 0 连续 | ffmpeg 解码（逐帧、按解码顺序，不插值）**不补位** | 通知 step 2 可启动 |
| 3 `ue_commandlet` | `frames/` | `curves/` | `async_run_ue(...)`，`-FPS=25/1`（§7.3） | 成功后删除 `frames/` |

> 前端的"结束录制"（`POST .../stop`）只负责收尾 `take.mp4`；转码是**另一次显式动作**（`POST /transcode`）。两者不自动串联，避免用户在录完之前就被迫等待一次几分钟的 UE 求解。

- step 1 失败 → 整作业失败（UE 不可能跑）；step 2 失败 → 整作业失败，**保留 `frames/` 供人工重跑**。
- 步骤顺序：step 2 等 step 1。**同一 `session_id` 同时只允许一个作业**；不同 `session_id` 在理论上可以并发（前端换过会话后再提交上一次的转码），step 3 会在 `ue_ctx` 闩上自然串行——**这一点不额外加锁，靠闩即可**。
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
| `decode_max_width` | 否 | `take.mp4 → frames/` 的目标长边，默认 1280，用于控制 PNG 体积 |

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

- 进度：`png_sequence` 0–40%，`ue_commandlet` 40–100%。**明确标注为粗粒度**，前端不得用于精确 ETA。
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

- step 2 成功 → **立即 `FSys::remove_all(session_dir / "frames")`**，保留 `take.mp4`、`align.json`、`curves/`。删除动作失败只记警告，不影响作业返回成功（产物已齐）。
- 提供重跑入口：再次 `POST /transcode` 时若 `frames/` 已不存在，自动从 `take.mp4` 重跑 step 1（不需要重录）。
- `/Game/.../<session_id>` 下的中间资产由 Commandlet 写（包路径为空时落点由它自己决定）；删除会话（`DELETE .../recordings/{session_id}`）时一并清理（尽力而为，失败只记日志）。
- **`take.mp4` 永不自动删除**；磁盘水位策略由部署侧运维负责，方案里不引入自动淘汰。
- 本方案**不改变** `DeleteSession` 对 `frames/`、`curves/` 的处理口径：它们都在 `session_dir` 下，删会话即删全部。

---

## 8. 交给 UE 的数据硬约束

全部来自 [MetaHumanFaceCaptureDataBuilder.cpp](E:\Doodle\script\uePlug\Doodle\Source\DoodleMetaHumanFace\Private\MetaHumanFaceCaptureDataBuilder.cpp) 的 `ScanSequence()` 与 `BuildWithTimecode()`，**这是整条链路最容易出错的地方**。

### 8.1 文件与目录

| 约束 | 出处 | 后端的做法 |
| --- | --- | --- |
| 文件名必须匹配 `^(([a-zA-Z0-9]*[_\s-])*)([0-9]+)[a-zA-Z_\s-]*\.[a-zA-Z]+$` | `MetaHumanFaceCaptureDataBuilder.cpp:20` | 固定 `frame_000000.png`（前缀 `frame_` + 下划线 + 6 位数字） |
| 数字位数必须全序列一致 | `:212-222` | 固定 6 位；超过 999999 帧直接拒绝，**不自动加位** |
| 帧号必须从首帧起连续，不能有空洞 | `:233-258`（缺帧即 `FramesNotFound`，**不允许跳过**） | `frames/` 由 `take.mp4` **按解码顺序逐帧写出**，`n` 从 0 连续自增，天然无空洞（不再需要补位，§10） |
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
- UE 只要求"帧序列连续"：`frames/` 来自 `take.mp4`，`-FPS=25/1` 只是告诉求解器"把这些图当作均匀 25 fps 来处理"。摄像头实际 30 fps 也不影响求解，它在录制时已被 `fps` 滤镜采样到 25 fps（每张 40 ms）。
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
<core_set cache root>/face_capture/sessions/<session_id>/
    take.mp4                   # 录制产物，唯一权威来源，永不自动删除（H.264 25 fps，时基 1/25）
    align.json                 # seq / orig_seq / wall_ms / 静默区间 / 实际宽高（输出帧率恒为 25）
    png_tmp/                   # step 1 的落盘暂存，自检通过后整体改名为 frames/
    frames/                    # step 1 产出 → UE 输入；step 2 成功后删除（§8.4）
        frame_000000.png …
    curves/                    # step 2 产出
        face_curves.dhfc           # 主交付，二进制（格式见 metahuman_face_capture_solution.md §5）
        face_curves_summary.json   # 人读摘要：元信息 + 掉帧区间 + 每条曲线的 min/max/first/last
        face_capture_log.json      # Commandlet 自写，成功失败都写（§7.4）
        frames_meta.json           # 可选，后端写 `{"first_frame": n}`（§8.2）
    logs/
        transcode.log              # 两步作业过程日志
```

`curves/` 里那三个文件由 Commandlet 在 `-OutDir` 下直接落盘（`MetaHumanFaceCommandlet.cpp:20-22` 的三个文件名常量）。`face_curves_summary.json` 的字段：

| 字段 | 说明 |
| --- | --- |
| `magic` / `version` / `flags` | 与 `.dhfc` 二进制头同源（`"DHFC0001"` / `1` / `2`），用来确认摘要和曲线文件配套 |
| `frame_count` / `curve_count` / `fps` | 帧数、曲线数、帧率（`"25/1"`），`curve_count` 实测为 257（6 条头部 + 251 条面部） |
| `start_frame` / `start_time_sec` / `duration_sec` | 起始帧号与对应秒数、时长 |
| `dropped_frame_ranges` | `[{start, end}]`；缺图在扫描阶段就失败，正常为空数组 |
| `curves` | `[{name, min, max, first, last}]`，每条曲线的极值与首末值，用于不开二进制也能判断某条曲线是不是全程为 0 |

`core_set::get_set().get_cache_root(FSys::path{"face_capture"} / "sessions")` 与 `movie.cpp` 用 `get_cache_root("http")` 同款。

体积预估（1280×720，1 分钟）：`take.mp4` 约 12–21 MB（输出恒为 25 fps）、`frames/` 约 0.6–1.2 GB（1500 张 PNG，取决于画面细节）。**`frames/` 比 `take.mp4` 大两个数量级，所以"成功即删 `frames/`"不是优化而是必需**。

判读 `align.json` 的字段（实现时按此定结构体）：

```json
{
  "session_id": "…", "width": 1280, "height": 720, "fps": 25,
  "source_fps": 29.97, "started_at": "2026-11-05T09:00:00+08:00", "duration_ms": 61000,
  "raw_frames": 1525, "inserted_frames": 0,
  "frames": [{"seq": 0, "orig_seq": 0, "wall_ms": 0}],
  "gaps": [{"wall_ms_begin": 16480, "wall_ms_end": 17120, "duration_ms": 640}],
  "truncated": false
}
```

- **没有 `face` 字段**：人脸检测在二期（§6），一期不往权威记录里写任何检测结果。
- **`fps` 是输出帧率（恒 25），`source_fps` 是实测摄像头帧率**（由真实帧数与墙钟算得），两者不必相等。
- `frames[]` 一项对应 `take.mp4` 里的一个帧，`i` 就是解码顺序下标：`seq` 是该帧在 25 fps 输出时间轴上的位置（`fps` 滤镜给它分配的序号），`orig_seq` 是原始采集序号（0,1,2…严格连续），`wall_ms` 是它到达的时刻。
- `gaps` 只在摄像头连续 > 1 秒没返回帧时出现（§4.4），记的是**墙钟区间**，不参与帧号折算。
- 录制期 `seq` 就是 25 fps 输出帧号，从 0 起连续；`orig_seq` 是录制期摄像头原始帧号，两者在摄像头帧率高于 25 时不再一一对应。**这不影响 `frames/`**：`write_png_sequence()` 按下标逐帧写 PNG，不用 `seq`（§10）。`inserted_frames` 恒为 0（保留字段只为兼容旧记录）。

---

## 10. 丢帧补位：已废止

**取消固定节拍后，这一节不再有实现。** `frames/` 由 `take.mp4` 按下标逐帧写出，`n` 从 0 连续自增，天然满足 UE 的"帧号连续无空洞"约束（§8.1），**不需要复制任何帧**。

`write_png_sequence()` 的现行做法（已实现并编译通过）：

1. `take.mp4` 按解码顺序逐帧取图（`av::VideoDecoderContext`）→ `av::VideoRescaler` 到 `RGB24` → `cv::imencode(".png")`；
2. 第 i 个解码帧写成 `png_tmp/frame_{i:06d}.png`，**下标自增，与 `align.json` 的 `seq` 无关**；
3. 全部成功才 `rename(png_tmp → frames/)`；失败保留 `png_tmp/` 并返回 `png_sequence_invalid`；
4. 写完后比对解码帧数与 `align.json` 的 `raw_frames`，不一致只记 `SPDLOG_WARN`（不复位、不补帧）——差异只可能来自编码器掉帧，由 `truncated` 标记（§5.4）。

历史方案（保留以备查阅）：固定 40 ms 采样时代，采集侧产出的帧率恒为 25，而摄像头卡死会造成帧号空洞，于是按"策略 A：重复上一帧"把 PNG 序列补齐到 `len(frames) + Σ gaps.missing` 张。**该机制随固定节拍一起删除。**

---

## 11. 里程碑

| 里程碑 | 内容 | 验收 |
| --- | --- | --- |
| **M0（0.5 天，只验证不写产品代码）** | ①手搓一份 `frame_000000.png`（6 位数字）序列，真机跑 `-run=MetaHumanFace -FramesDir=<abs> -OutDir=<abs> -FPS=25/1`；②记录单帧求解耗时，判断 `core_set.timeout` 是否够；③确认 `curves/frames_meta.json` 的 `first_frame` 被采用；④确认不传 `-SequencePattern` 能通过；⑤确认 `FTrackingPathUtils` 接受该命名 | 五条结论写回本文 §8；任一条失败立即回到 §8 复核 |
| **M1（已完成 ✅）** | 摄像头读协程 + `frame_filter` + `media_fragment_pipeline`（推流）+ `ws_outlet` + `ws_message` + WS 路由 `face_capture_stream`（`ws_outlet`：连上 `open()` 预览、断开 `close()` 收尾）+ 设备路由 `face_capture_devices`（GET 枚举 / POST 选中）+ `capture_session_holder` | 浏览器里选设备 → 连 WS 就出现实时画面，**且磁盘上不产生任何文件** |
| **M2（已完成 ✅）** | `take.mp4`（H.264 25 fps）+ `align.json` + 两轴状态机 + 录制段路由（`POST/GET /recordings`、`stop`、`GET /video`、`GET /curves`、`DELETE`） | 点"开始录制"后目录里出现 `take.mp4`；`ffprobe take.mp4` 的相邻 `pts_time` 间隔恒为 0.04s；`stop` 返回后 `align.json` 已存在 |
| M3 | 转码作业 step 1（`png_sequence` 逐帧无补位）+ 作业查询/取消。**`write_png_sequence()` 已实现并编译通过**，等待最终作业调度层 | 不接 UE 也能从 `take.mp4` 产出合格 `frames/`，命名校验生效 |
| M4 | 转码作业 step 2（Commandlet 编排 + 结果解析 + `frames/` 删除） | 端到端：录 → PNG → UE → `.dhfc` |
| M5 | 加固：状态机重入、`ws_outlet` 丢片调参、`ue_ctx` 排队观测、错误码统一 | 连续录 20 次无泄漏、无卡死 |

> 一期只有 M0–M5 六个阶段，没有人脸检测相关的里程碑。二期的检测工作另立方案，开工第一步是验 `objdetect` 能否编过（§6.2）。

---

## 12. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| **UE 侧命名/跟踪路径不匹配**（B 路线首风险） | `BuildWithTimecode` 直接 `CaptureDataBuildFailed`，表现为"素材错" | M0 先钉死；step 1 写盘后比对帧数与 `raw_frames`，不合格不提交 UE |
| `cv::VideoCapture` 阻塞 io 线程 | 整个 HTTP 服务卡死 | 读帧放在独立 `std::thread`，strand 只处理已到手的帧（§4.1） |
| `stop()` 时 `read()` 卡死不返回 | HTTP 请求挂住 | `cap_->release()` 唤醒阻塞读；2 秒内退不出就记 `truncated` 并 `detach`（§4.1） |
| 摄像头帧率高于 25 | 推流/`take.mp4` 时间轴抖动 | `fps=25` 滤镜按真实时间戳采样（§4.4） |
| 摄像头帧率低于 25 | 推流里出现重复帧 | `fps` 滤镜自动重复帧；只在画面运动上有观感差异，不影响求解 |
| `fps` 滤镜建不出来 | `take.mp4` 的时间轴按帧数而非墙钟时间推进 | 不降级，`frame_filter` 构造即抛异常，`open()` 失败并把原因回给前端（§4.4） |
| WS 写并发 | beast 直接崩或数据错乱 | 所有连接状态只在流自己的执行器上访问：`send()` 一律 `post` 到执行器再 `co_spawn` 写出协程；查询只读 `closed_`/`writing_` 两个 atomic（§5.3） |
| `stop()` 同步阻塞 strand | 写协程饿死，等满 2 秒才返回 | `wait_outlet_idle()` 改成 `co_await timer.async_wait()` 轮询，让出 strand（§4.2） |
| 页面挂起导致 WS 停读 | 写挂住、推流分片被丢 | 两层丢非关键帧 + 关键帧封顶 2 个 + 2 秒判死连接（§5.3）；**录制链不受影响**，丢的只是画面 |
| 摄像头型号差异（分辨率/帧率/MJPG 直出） | 请求的分辨率被静默改成别的 | 回读实际分辨率并上报（§4.3）；帧率不需要协商，滤镜负责统一（§4.4） |
| 摄像头静默 / 驱动掉帧 | 录制里出现时长空洞 | `gaps` 记墙钟区间（§4.4），不折算帧号，UE 不受影响（§10） |
| **`take.mp4` 是有损 H.264** | 喂给 UE 的画面带压缩痕迹，曲线精度可能不如无损源 | 这是"去掉 raw.mkv"的直接代价。对策：录制码率不足以支撑求解时，把 `mp4_writer` 的 `crf` 从 23 调到 12–15（体积涨 2–3 倍，仍远小于 FFV1）；M4 验收时用同一段素材比对"无损 vs H.264"的曲线差异，若不可接受再恢复 FFV1 录制（`raw_writer` 代码仍在，§5.4） |
| `take.mp4` 体积 | 磁盘占用 | 720p/25fps/crf23 约 12–21 MB/分钟，比原 `raw.mkv` 小两个数量级；不自动删除，水位告警仍在 |
| `core_set.timeout` 不足以覆盖脸捕 | 长素材必失败 | M0 实测后决定是否为 `async_run_ue` 增加可选 timeout（§7.3-5） |
| 与渲染任务抢 `ue_ctx` 闩 | 脸捕要排队，前端进度长时间停在 40% | 进度里显式给出 `ue_commandlet: waiting_for_ue_lock` 子状态，别让前端以为卡死 |
| `async_run_ue` 超时后 UE 进程仍在跑 | 闩被占死，后续所有 UE 任务排队卡住 | 作业捕获超时异常后先终止该进程再返回（§7.4） |
| 路由类被 `clone()` | 成员生成两份，引擎状态错乱 | 会话由 `ws_outlet` 持有，路由成员只有 `shared_ptr`（`runner_` / `selected_` / `holder_`）；跨请求共享状态一律 `shared_ptr<impl>` 而不是值成员（§3.1） |
| WS 断开时录制收尾未完成 | `take.mp4` 没有 `moov`/`trailer`、`align.json` 缺失 | `close()` 内部先等价执行 `stop()` 再关摄像头，`close_impl()` 等它跑完再关流；进程退出时由 `app_base::Get().on_cancel` 槽控制协程退出顺序 |

> 一期没有"模型缺失""opencv 不带 objdetect"这类风险——人脸检测整体在二期（§6）。

---

## 13. 验证方式

按仓库约束：**不写测试、不跑测试**，验证手段只有编译通过（`cmake_configure Ninja_debug` + `cmake_build debug_doodle`）。运行期验证只在 M0 里程碑里做一次真机冒烟，其余靠人工按 §3.2 的接口清单点一遍。

**当前状态**：采集引擎 7 组源文件 + WS 路由（`face_capture_stream`）+ 设备路由（`face_capture_devices`）+ 录制段路由（`face_capture_recordings` / `_recording` / `_recording_stop` / `_recording_video` / `_recording_curves`）+ 会话持有者（`face_capture_session_holder`）已全部实现，`capture_session` 已拆成 `open/start_record/stop/close` 两轴状态机，不再写 `raw.mkv`。`cmake_build(--preset debug_doodle)` **exit 0**（`doodle_lib.dll` / `test_main.exe` / `doodle_kitsu_supplement.exe` 全链接通过）。**转码作业层（`exe_warp/face_capture`）尚未开始**。

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

1. ~~丢帧补位 = 策略 A（重复上一帧）~~ → **已废止**：`frames/` 逐帧写出天然连续（§10）。
2. 脸捕 Commandlet 与渲染任务共用 `core_set::get_set().ue_ctx_->queue_`（§7.3-2）。
3. `frames/` 在转码作业成功后**默认删除**、`take.mp4` 保留，**不可配置**（§7.5）。
4. **推流只有一条**，不存在第二条，设计上也不为它留口子（§0 第 9/10 条、§5.3）。
5. **控制一律走 HTTP POST，WS 纯下行**（§0 第 7 条、§3.5）。
6. **人脸检测推迟到二期**，一期不实现、不预留字段（§0 第 12 条、§6）。
7. **转码作业不登记 `server_task_info`**：不写库、不进任务列表，只走 `GET /transcode/{job_id}` 与 WS 进度（§7.2）。
8. **`ue_project` 由前端在转码请求里必填，且必须是绝对路径**：缺失/为空/非绝对路径/文件不存在即 `400 missing_ue_project`；后端不拼相对路径、不猜默认工程（§7.2、§7.3-8）。
9. **`package_path` 可以为空，空则不传 `-PackagePath`**，由 Commandlet 自己决定落点；非空时后端只校验格式（§7.2、§7.3-9、§7.3 的条件拼参）。
10. **取消固定节拍**（2026-11-06）：不再用 `steady_timer` 40 ms 轮询采样；摄像头阻塞读放独立线程，到达即打时间戳 `post` 到 strand，`fps` 滤镜统一成 25 fps（§0 第 16 条、§4.1、§4.4）。
11. ~~`raw.mkv` 存真实帧、时基 `{1,1000}`~~ → **已被第 14 条取代**：不再保留 `raw.mkv`。
12. **`capture_session` 由 `ws_outlet` 创建持有**（2026-11-06），裸指针登记进 `capture_session_holder` 供录制端点使用；不打开 WS 就不抓摄像头，WS 连上只开预览、断开即关闭；删除进程内单例与 `make()`（§0 第 6/17 条、§3.1、§3.3）。
13. **选中摄像头是 `face_capture_devices` 路由上的共享属性**（2026-11-06）：`shared_ptr<impl>` 保证 clone 后仍共享，通过 `reg_c` 构造参数注入 `face_capture_stream`；不进 `core_set`（§3.1）。
14. **WS 连接只推预览，录制是显式动作**（2026-11-06，A 计划）：连上 WS 只 `open()` 摄像头并推 25 fps 预览流，**不落任何文件**；`POST /recordings` 才建目录、开写 `take.mp4`。`capture_state` 新增 `previewing`（§0 第 19 条、§3.3、§4.2）。
15. **取消 `raw.mkv`，录制只落 `take.mp4`**（2026-11-06）：不再 FFV1 无损录制，不再有"真实帧"这条轴；转码 step 1 直接从 `take.mp4` 解码，原来的 `mp4` 步骤删除。代价与回退办法见 §5.4、§12 风险表。

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
| `src/doodle_lib/http_method/local/face_capture.h` + `face_capture_stream.cpp` | 新增 | **已完成**（`websocket_callback` 只 `make_shared<face_capture::ws_outlet>` + `start()`；连接管理在 `core/face_capture/ws_outlet`） |
| `src/doodle_lib/http_method/local/face_capture_recording.h` + `face_capture_record.cpp` | 新增 | **已完成**（会话持有者 + 录制段 5 个路由类） |
| `src/doodle_lib/http_method/local/face_capture_device.cpp` | 新增 | **已完成**（GET 枚举 / POST 选中）；转码 1 个 .cpp 未开始 |
| `src/doodle_lib/http_method/local/CMakeLists.txt` | 登记 `.h`(PUBLIC) 与 `.cpp`(PRIVATE) | **已完成** |
| `src/doodle_lib/http_method/kitsu.cpp` | `create_kitsu_local_route()` 追加 §3.1 的 `reg_c` | **已完成 7 条**（`devices`、`stream`、`recordings`、`recording`、`recording_stop`、`recording_video`、`recording_curves`），转码 2 条未开始 |
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
| `capture_session_registry`（`session_id` 查找表） | 删除，动作改由 `ws_outlet` 持有的会话对象承担 | 本机只有一个会话 |
| `capture_session::instance()` 进程内单例 | 删除，`capture_session` 变成 `ws_outlet` 的成员，随 WS 连接创建/销毁 | 不打开 WS 就不该抓摄像头；单例会让"有连接/没连接"与"有没有会话"脱钩 |
| WS 按 `session_id` 分路径、多连接互斥 + `409 device_busy` | 路径保留占位但不参与查找；第二条连接 attach 后自然接管，旧 outlet 不再被推送（不发 `superseded`，旧连接自己从 `async_read` 失败退出） | 摄像头只有一个消费者，不需要仲裁 |
| 多标签页：`control:"observer"` 观察者模式 | 删除 | 只有一条推流、一个控制者 |
| 每连接 `fragment_sink` + 环形缓冲 + 逐连接 FIFO + `gap` 标记 | 单在途写 + 两层丢非关键帧 + 关键帧封顶，`gap` 字段删除 | 只有一个出口，不需要分发与回放补发 |
| `align.json` 里的 `dropped_by_sink` | 删除 | 推流丢片不影响录制，不写进权威记录 |
| `capture_start` 的 `fill_mode` 字段（只接受一个值） | 删除 | 策略 A 已拍板，协议里不留无效开关 |
| WS 上行命令（`capture_start` / `capture_stop` / `transcode_start` / `ping`） | 全部删除，改为 `POST /recordings`、`POST .../stop`、`POST /transcode` | 本机 loopback，请求-响应没有不确定性，没必要维护 ack/超时/重放（§3.5） |
| WS 读协程里的命令解析与 `post` 到 strand 的分派 | 删除，读端只用于断连检测 | 没有上行命令可解析 |
| WS 心跳 `ping/pong` | 删除，服务端不做空闲超时 | 单用户本机连接不该被服务端主动断开 |
| 单独的"兜底停止"REST 端点 | 合并为唯一的 `POST .../stop` | 控制面只有一条路，不存在"主路用完用兜底" |
| 帧率可配：`POST /recordings` 的 `fps`、转码的 `fps:"num/den"`、`align.json` 的 `fps_num/fps_den`、按实际出帧率设 `time_base` | 全部删除，输出帧率常量 25 fps；`align.json` 只留 `"fps": 25`（输出）与 `"source_fps"`（实测），`raw_pts` 一列也删 | 帧率定死（§4.4），分子分母推理、实际帧率协商、`host_pts → pts` 换算全都不需要 |
| 丢帧判定的 `1.8 × 帧间隔` 自适应阈值 | 删除；`gaps` 只记录"连续 > 1 秒无新帧"的静默区间 | 帧率固定后没有"帧率抖动"这个丢帧来源 |
| 编码链 `cvtColor(BGR→RGB)` 中转后交 `SwsConverter` | 直接 `AV_PIX_FMT_BGR24` → `av::VideoRescaler` → `YUV420P`；`frames/` 侧用 `cv::imencode` 而不是 `cv::imwrite` | 少一次全帧转换；两条链路同源同 swscale，色彩口径天然一致 |
| 固定 40 ms 拍点（`steady_timer` + `next_tick += 40ms`）+ 拍点无新帧时重复上一帧 + `gaps` 按 `missing` 折算 + `expect_frame_count = len + Σmissing` + step 1 补位策略 A | 全部删除：改为摄像头阻塞读协程"到达即处理"，真实时间戳进 `fps=25` 滤镜统一输出帧率；`raw.mkv` 只存真实帧（时基 `{1,1000}`），`frames/` 逐帧写、不补位；`gaps` 改为墙钟区间 | 定时器相位与摄像头相位之差会随机决定"重复/丢弃哪一帧"；按真实时间戳采样既均匀，又让 PNG 输入不含任何复制帧（§4.4、§10） |
| `align.json` 的 `frames[].host_pts_us`（仅诊断，不参与换算） | 改为 `{seq, orig_seq, wall_ms}` + 顶层 `source_fps` / `raw_frames` / `inserted_frames` | 真实时间戳现在是链路的一等公民（喂滤镜），不再是旁注 |
| `face_capture_stream` 的 `websocket_callback` 里直接 `co_spawn(g_io_context(), run(std::move(stream)))`，连接生命周期写在回调里 | 独立成类承担生命周期，`websocket_callback` 只 `make_shared` + `start()`（先是就地写在 `face_capture_stream.cpp` 的 `ws_stream_runner`，后来并入 `core/face_capture/ws_outlet`，见下面两条） | 连接生命周期要独立于回调栈帧存活 |
| `ws_outlet::write_fn` 只收 `(data, size)`，文本/二进制由调用方自己 `stream.binary()` 切换 | 帧类型判定收进 `ws_outlet` 的写协程，按消息的 `binary()` 决定是否再发一帧二进制 | 写协程是唯一能安全切换帧类型的地方（单在途），调用方拿不到这个时序 |
| `capture_session::impl` 的 `stream_` / `set_stream()` / `stream()` / `write_ws()`（早期给 WS 预留的写通路） | 全部删除，`attach()` 只接管 outlet | 两条并存的写通路没有调用者，且 `write_ws` 拿不到 text/binary 信息 |
| `core/face_capture/ws_stream_runner.h/.cpp` 与 `http_method/local/face_capture_stream.cpp` 里的连接管理类（两代实现） | **合并进 `ws_outlet`**：它直接以 `websocket::stream<tcp_stream_type>` 构造、执行器取自流、自己 co_spawn 读写协程；`capture_session` 持 `weak_ptr<ws_outlet>` 断环 | 一个"出口"和一个"连接"本来就一一对应，分两个类只是把同一份状态拆到两处；HTTP 层也不必再前置声明一个只为它服务的类 |
| `ws_outlet` 的 `send_hello` / `send_event` / `send_fragment`（裸函数 + 手拼 JSON）+ `set_session_info` / `set_init_segment` | 全部替换为结构体消息 `ws_message` 派生（`ws_hello` / `ws_avc_init` / `ws_avc_fragment`）+ 唯一入口 `send(std::unique_ptr<ws_message>)`；状态只在 `hello` 里带出，不再有 `set_*` 充填状态 | 协议形状从散落在 5 个函数里的 `nlohmann::json{{...}}` 收敛到一个 `.cpp`，加事件只加一个派生类；`set_*` 那套"先填字段再发"的顺序约束随之消失 |
| `ws_outlet` 自建 `make_strand(g_io_context())` + 上限 8 片 `std::deque` + `drain()` 泵 | 执行器直接取 beast 流自己的（`stream_.get_executor()`），发送改为"`post` 到该执行器 + `co_spawn` 一个写协程"，不排队 | 流已经在那个执行器上，另建 strand 只会多一层跨 strand 投递；`co_spawn` 天然串行，等价于原来的泵，但没有队列与长度上限 |
| "写不动"只靠丢非关键帧 + 队列满 | 两层判定（采集侧见 `pending_frame()` 丢非关键帧、执行器内再判一次）+ 关键帧封顶 `k_max_pending_keys`=2 + `pending_since_` 超 2 秒判死连接 | 页签切后台时 `async_write` 会一直挂着而不报错，没有超时判定就只剩"永远不恢复"；关键帧不封顶则会在前端不读时无限堆积 |
| `face_capture_session_holder`（定义在 `http_method/local/face_capture.h`） | 下沉为 `core/face_capture/capture_session.h` 里的 `capture_session_holder`，HTTP 层用 `using face_capture_session_holder = face_capture::capture_session_holder;` 沿用旧名 | `ws_outlet` 在 core 里要它注销会话，HTTP 头不能反向被 core 引用 |
| 人脸检测整块（`core/face_capture/face_detector.*`、`face_status` 事件、`hello` 的 `face_detector` 字段、`align.json` 的 `face`、`POST /recordings` 的 `422 no_face` 与 `force`、`kitsu_ctx_t::face_detector_model_path_`、M0' 预研与 M3 里程碑） | 全部推迟到二期，一期不做也不预留 | 一期目标是把"录 → 推 → PNG → UE 曲线"打通，检测是独立增量（§6） |
| 摄像头选择放进 `core_set` / 环境变量 / 配置文件 | 定为 `face_capture_devices` 路由上的 `shared_ptr<face_capture_selected_device>`，`reg_c` 构造参数注入 `face_capture_stream` | `create_kitsu_local_route()` 的 `http_route_ptr` 本就没挂 `core_set`；选中值只在同一次路由注册里流转，不必惊动全局配置 |
| **连上 WS 即开始录制**（`run()` 里直接 `session_->start()`），录制产物与连接生命周期绑定 | 拆成三段：`open()` 只开摄像头推预览（不落盘）、`start_record()` 由 `POST /recordings` 触发、`stop()` 收尾后**继续预览**；`capture_state` 追加 `previewing` | 用户要求"没点开始录制之前只推预览流"；预览期不该在磁盘上留下任何东西 |
| **`raw.mkv`（FFV1 无损，时基 1/1000）+ 转码三步（png_sequence / mp4 / ue_commandlet）** | 取消 `raw.mkv`，录制只落 `take.mp4`（H.264 25 fps）；转码变两步：`take.mp4 → frames/ → curves/`，原 `mp4` 步骤删除 | 用户要求"不保存 raw.mkv，只保存 take"：少写一份 GB 级文件、少一次编码、录完即能回放；代价是有损输入 UE，列入 §12 风险并留了恢复 `raw_writer` 的口子 |
| `capture_session::start()` 一个 `state_` 走全程 | 拆成 `stream_state_`（idle/previewing）与 `record_state_`（idle/recording/stopping/stopped）两轴，对外用 `display_state()` 合成 `capture_state` | 一根轴表达不了"摄像头开着但没在录"这个中间状态 |
| WS 的 `capture_state` 状态推送事件 + `ws_outlet::notify_state` + `impl::state_` 的持续维护 | 删除：状态只在连接建立时由 `open()` 末尾发一次 `hello` 快照，之后状态一律以 HTTP 响应体为准 | 动作是否生效本来就由 `POST /recordings`、`POST .../stop` 的状态码与 `capture_status` 回答（§3.5）；同一份状态有两条上报通路只会制造"WS 说 stopping、HTTP 已返回 stopped"的不一致。代价：前端需自己处理 `stop()` 最长 2 秒的 pending，以及重连后靠 `hello` 或 `GET /recordings/{id}` 重新取状态 |
| `align.json` 的 `raw_frames` 表示"真实帧数"、`inserted_frames` 表示滤镜补帧数 | `raw_frames` 改为"录制期写入 `take.mp4` 的帧数"（= `frames_` 长度），`inserted_frames` 恒 0 保留字段 | 没有 `raw.mkv` 之后就不存在"真实帧"这条轴 |