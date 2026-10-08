# AI 灯光「派发功能」交接文档

> 需求原话（用户）：**「安排 ai_light 工作的派发功能」**。
> 目标：把一轮 6 个候选真正派到工作机 → UE commandlet 渲染 → 回传落库，让会话从
> `generating` 走到 `awaiting_judgement`（人工判断闸门 2 之前的那一段）。
>
> **当前状态：服务端 + 工作机编译通过、单测全绿（`ai_light_dispatch` 23 用例，
> 全量 241 用例 / 1969 断言 exit 0）；UE 侧两个插件模块源码已写完，但
> **本机无法编译 UE 代码**（`E:\Doodle\script` 下只有打包好的插件副本，没有 HostProject 也没有打包脚本），
> 只做了 API 级核对 —— 真机验证前必须重新打包插件包。**
> 设计记录见 `docs\ue_ai_lighting_plan.md` **§15.12**（服务端/工作机）· **§15.13**（UE 侧）·
> **§15.14**（产物落工作机本地 + HTTP 上传，D38~D43）· **§15.15**（产物路径改「现算」，D44）。

## 一、链路（现在的真实形状）

```
POST /api/doodle/ai/light/session/{id}/iterate
  └─ advance_session（generating 分支）
       ├─ generate_candidates(...)                      6 个 rig + 候选行（**不写路径**，D44）
       ├─ shot_render_light(project_id, task_id_)        基线关卡 / .uproject / 相机文件
       ├─ ai_light_task_payload_t → server_task_info.command_（type_ = ai_light）
       │    载荷里 out_dir / png_path / exr_path 都是**相对路径**（D39）
       └─ run_next_task() + broadcast(server-task-info:new)
                    ↓ 工作机（报在线 + 无 running 任务）
http_work::run_task → ai_light_round_distributed::run()
       ├─ 把相对路径拼上工作机本地缓存根（get_cache_root("ai_light")）→ 写进 -Params 的 JSON
       ├─ UnrealEditor-Cmd.exe <uproject> … -run=DoodleLighting -Params=<tmp>.json
       ├─ 读 <本地 out_dir>/render_report.json
       ├─ 逐候选上传：POST /api/doodle/ai/light/candidate/{id}/file/{render|render_linear}
       │    PNG 必传（失败 → 该候选判 failed）；EXR 尽力而为（D42）
       ├─ POST /api/doodle/ai/light/render_report/{task_id}（只带状态与数值，不带路径）
       └─ put_job_info（终态）→ 析构时 task_finished() 归还机器；本地产物保留不删（D43）
                    ↓
render_report_route → parse_round_render_report → apply_render_report(sql, ctx, task_id, report)
       ├─ 逐条查 D41：说 rendered 但 `candidate_render_path(ctx, 行, false)` 算出的 PNG 不在 → 降级 failed
       │    （EXR 同法算出、不在只 WARN，D42）
       ├─ 至少一张 rendered → mark_iteration_rendered（轮次 awaiting_judgement + 会话 await）
       └─ error_ 非空 / 一张都没 rendered / 全被降级 → mark_iteration_failed（只写会话 failure_reason_）
```

## 二、各部分现状

### wire 契约（`src\doodle_core\metadata\ai_light\task_payload.h`）
`E:\Doodle\src\doodle_core\metadata\ai_light\task_payload.h`

- `g_ai_light_render_report_file_name{"render_report.json"}` `:56` —— commandlet 写、工作机读。
- `ai_light_task_payload_t` `:113`（派发，进 `server_task_info.command_`）：键
  `session_id / iteration / spec_id / out_dir / baseline_level / ue_project_path / camera_file_path / candidates`。
- `ai_light_task_candidate_t` `:82`：键 `candidate_index / candidate_id / png_path / exr_path / rig`（rig 含 `ops_`）。
- `round_render_report_t`：键 `candidates / error`。
- `candidate_render_report_t`：键 `candidate_index / candidate_id / status / render_ms / light_count / metrics`
  —— **不带路径**（D27：产物落点由服务端现算，见 D44）。
- 相对路径契约件：`g_ai_light_local_cache_flag` `:61` · `make_iteration_relative_dir` `:66` ·
  `make_candidate_file_name` `:73`。
- 已登记：`src/doodle_core/metadata/ai_light/CMakeLists.txt`、伞形头 `.../ai_light/ai_light.h`。

### 服务端 · 编排层（`src\doodle_lib\http_method\kitsu\ai_light_orchestrator.{h,cpp}`）

| 函数 | 位置 | 职责 |
| --- | --- | --- |
| `brief_text` | `.cpp:85` | 截断到 512 + `"...(已截断)"`（`brief_error` 调它） |
| `advance_session` | `.cpp` 状态机入口 | 按入口状态分派到 spec 分支 / generating 分支；接缝抛错时回滚到入口状态并写 `failure_reason_` |
| `apply_judgement` | `.cpp` | 定稿 / 续轮 / 全否三种决定都回到 `generating`（D17）；定稿同时置 `is_selected_`（D20） |
| `parse_round_render_report` | `.cpp:760` | **严格**：`candidates` / 元素的 `candidate_id` / `status`（`json_val::require_enum<candidate_render_status>`，不许静默回落）→ 502；**宽松**：`candidate_index` / `render_ms` / `light_count` / `metrics` / 轮级 `error` 缺失取默认、形状不符丢该字段 + WARN |
| `mark_iteration_rendered` | `.cpp` | 候选 UPDATE 只写 `render_status_` / `render_ms_` / `light_count_` / `metrics_`；轮次转 `awaiting_judgement` + 起算 4 小时 |
| `mark_iteration_failed` | `.cpp:828` | 整轮失败收尾：**只**写 `session.failure_reason_`，轮次状态与候选行一概不动（D28） |
| `apply_render_report` | `.cpp:844` | 签名 `(sqlite_database&, const http::kitsu_ctx_t&, const uuid& task_id, const round_render_report_t&)`；两条出口：有 `rendered`（且落点的 PNG 真的在，D41）→ `mark_iteration_rendered`；否则 → 失败出口 |
| `candidate_render_path` | `.h:133` | 产物落点的**唯一**计算处（D44，`inline`） |
| `build_session_detail` | `.cpp` | 六表聚合读模型 |

声明都在 `ai_light_orchestrator.h` 上（带 `DOODLELIB_API`）。

### 服务端 · 派发与路由（`ai_light.{h,cpp}` + `kitsu.cpp`）
- 基线关卡推导（D8）：`ai_light_orchestrator.cpp:282-295`
  调 `http::auto_task::shot_render_light(io_session.project_id_, io_session.task_id_)`（`auto_task.cpp:768-812`），
  取 `original_map_` / `ue_main_project_path_` / `camera_file_path_`。失败只 WARN + 空串，不让整轮失败。
- 载荷组装 `:339-346`、进 `command_` `:395`。
- 回传路由：声明 `ai_light.h:158` · 注册 `kitsu.cpp:333`（`/api/doodle/ai/light/render_report/{}`）·
  实现 `ai_light.cpp:790`（先查任务类型必须是 `ai_light`，再解析、落库，回执带 `iteration_state` /
  `session_state` / `failure_reason`）。**不做项目权限检查** —— 调用方是工作机的机器 JWT。
- 客户端方法 `kitsu_client.h:174` / `.cpp:694` `post_ai_light_render_report(uuid, const nlohmann::json&)`
  （接受 200/201，回执非严格解析）。

### 工作机（`src\doodle_lib\exe_warp\ai_light_render.{h,cpp}`）
- 任务类：`ai_light_round_distributed : base_distributed_task`，成员只有
  `ai_light::ai_light_task_payload_t payload_` + `run()`。
  - 三条硬前置：`baseline_level_` / `ue_project_path_` / `candidates_` 非空，否则抛 `doodle_error`。
  - **起 UE 前先删 `<out_dir>/render_report.json` 残留**（否则 commandlet 崩在写报告前会回传上一轮结果）。
  - `async_run_ue({... "-run=DoodleLighting", "-Params=<tmp>.json"}, logger, kitsu_client, /*create_lock=*/true, run_time_info)`。
  - 异常也回传「整轮失败」报告（`l_report.error_`），**再** `put_job_info` 落终态（§10.4 契约）。
- 派发分支 `src/doodle_lib/http_client/work.cpp:237`。
- 默认允许类型 `src/doodle_lib/http_method/local/event.cpp:31-38` 含 `ai_light`。
- CMake：`src/doodle_lib/exe_warp/CMakeLists.txt`（PUBLIC `.h` / PRIVATE `.cpp`）。

### UE 侧插件（`script\uePlug\Doodle\Source\DoodleLighting{,Editor}`，20 个文件）
`script\uePlug\Doodle\Source\DoodleLighting\`（11 个文件）· `script\uePlug\Doodle\Source\DoodleLightingEditor\`（9 个文件）

- 预览用**交付机位**（载荷 `camera_file_path_` 的 FBX 经临时 level sequence 导入）、
  预览分辨率 **1920×1080 固定常量**（不进 wire 契约）。
- `Doodle.uplugin` 两条 Module：`DoodleLighting`（Runtime/Default/Win64）、
  `DoodleLightingEditor`（**Editor/Default**/Win64 —— 必须是 `Default`，`-run=X` 的类查找在插件加载之后）。
- `DoodleLightingEditor.Build.cs`：不含 MRQ，带 `Sequencer`/`MovieSceneTools`/`FBX`/`Imath`/`UEOpenExr`/`UEOpenExrRTTI` + `bEnableExceptions = true`。
- 入口：`-run=DoodleLighting` → `UDoodleLightingCommandlet`（引擎按名字找，无需注册），只认 `-Params=<json>`（D29）。
- **渲染方式 D36 = `USceneCaptureComponent2D`**（MRQ 要第二个进程驱动、无单帧/同步 API；
  LDR 通道 `RTF_RGBA8_SRGB`+`SCS_FinalColorLDR` → PNG，HDR 通道 `RTF_RGBA16f`+`SCS_FinalColorHDR` → EXR，
  8 次丢弃式预热抓帧让 Lumen 收敛）。
- **D37 = 不复制关卡**（`LoadObject<UWorld>` 后换 `GWorld`，不 `DuplicateAsset`/不 `SaveLoadedAssets`/不 `MarkPackageDirty`）；
  **D31 = 以 `rig.lights_` 全量重建**（`rig.ops_` 只审计，`diff_rigs` 永不产生 add）；**D30 = UE 不算指标**（`metrics` 不写）。
- 命令行必须带 `-AllowCommandletRendering` ——
  `CommandletHelpers::TickEngine` 少这个开关不 tick 渲染。

### 测试（`src/test/core/ai_light_dispatch.cpp`，已登记 `src/test/core/CMakeLists.txt`）
- **23 用例**：解析组 8 例（严格/宽松分界逐条）、落库组 5 例（两条出口 + 部分失败仍前进）、
  归属组 5 例（404 / 400 / judged 拒回填 / 原因截断）、产物存在性 4 例（D41）、路径纯度 1 例（D44）。
- 夹具造**真文件**（`media_dir_` + `write_probe_file()` + `candidate_png()`/`candidate_exr()`），
  因为 D41 判的是「算出来的落点上有没有文件」。

### 产物路径与上传（D38~D44）
> 用户原话（改方案）：「修改方案, 这里的输出目录改为工作机本地目录, 在输出后, 使用 http 上传, 并记录 使用
> `http::kitsu_ctx_t` 类中, 生成的目录不在 网络盘/UNC 挂载, 使用 http 上传, 而在
> `src/doodle_lib/core/entity_path.h` `entity_path.cpp` 文件中, 生成的目录才会使用 网络盘/UNC 挂载」
>
> 用户原话（路径改现算）：「…渲染文件路径属性 … 直接将 属性改为方法, 每次重新计算, 是否可行?」
> 「…去除前端契约, 前端不可获取这两个路径, 是否可行」→ 拍板：**做，并把轮次目录名一并换成轮次 uuid**。

- **目录归属（D38，全局规则）**：`http::kitsu_ctx_t` 派生的目录（`root_` 底下这一片）只保证
  **同一台机器同卷**、**不是**共享盘 —— 那份 `root_` 是**服务端**的 `D:/kitsu_data`
  （`launch/kitsu_supplement.cpp:150`），工作机对它的可见性纯属偶然；工作机产物一律写
  `core_set::get_set().get_cache_root("ai_light")`。只有 `core::entity_path` 派生的项目/资产目录才在网络盘/UNC
  （判定 `FSys::is_windows_remote_path`，`file_sys.cpp:209-211`）。
- **契约（D39）**：载荷里的路径一律**相对**（`out_dir = <session_uuid>/<轮次 uuid>`，
  候选 `<candidate_uuid>.png|.exr`）；命名服务端定、根工作机定。构造器在
  `task_payload.h`：`make_iteration_relative_dir()` / `make_candidate_file_name()`。
  候选行**不存路径**（D44）：要用时按 `candidate_render_path(ctx, 行, linear)` 现算。
- **服务端新路由（D40）**：`candidate_file_route`（`ai_light.h:166/:187`，role 段是 scoped enum
  `ai_light_candidate_file_role{render, render_linear}`，路由宏自动展开白名单正则）·
  注册 `kitsu.cpp:338` `/api/doodle/ai/light/candidate/{}/file/{}` · 实现 `ai_light.cpp:828`。
  要求 `application/octet-stream`；落点由候选行现算（`candidate_render_path`）；
  轮次 `judged` 拒绝、`awaiting_judgement` 允许重传；
  先写 `<目标>.uploading` 再 rename（前端不会读到半张图）；成功回 **204**。
  **故意不复用 `up_file_base`**（那套是按项目派生目录的，且跨卷 rename 会抛）。
- **工作机**：`exe_warp/ai_light_render.cpp` 加 `make_ue_payload()`（相对 → 本地绝对）；
  **上传必须在回传报告之前**（服务端 D41 靠查文件在不在判可判断性）；PNG 必传、EXR 尽力而为（D42）；
  本地产物保留不删（D43）。客户端方法 `kitsu_client::upload_ai_light_candidate_render(uuid, path, bool linear)`
  复用私有 `upload_asset_file`（`kitsu_client.cpp:261-290`，只认 204）。
- **UE 侧一行未改**：UE 只按载荷给的 `PngPath`/`ExrPath` 写文件、按 `OutDir` 写报告 —— 工作机把绝对路径
  塞进载荷即可，§15.13 那 20 个文件不动（也就没有加重「本机无法编译 UE」这个缺口）。
- **服务端回传增强**：`apply_render_report` 的 D41（按现算的落点找不到 PNG → 该候选降级 `failed`，
  全被降级 → `failure_reason_ = "轮次 {} 的 {} 个候选渲染成功但产物文件缺失，本轮无可判断结果"`，
  轮次留在 `rendering`）与 D42（EXR 不在 → 只 WARN）。计数只数**被降级的**，回传里本来就 failed 的不算。
- **D44**：候选**不存渲染路径**。`ai_light_candidate` 只有状态与实测数值，候选 JSON 里也没有路径键；
  路径由 `candidate_render_path(const http::kitsu_ctx_t&, const ai_light_candidate&, bool in_linear = false)`
  （`ai_light_orchestrator.h`）现算 —— 吃候选行自带的 `session_id_` / `iteration_id_` / `uuid_id_`
  三个 uuid。轮次目录名就是**轮次自己的 uuid**（`kitsu_ctx_t::get_ai_light_iteration_dir`
  的入参是 `const uuid&`），六个 `get_ai_light_*` getter 都是 `const`，于是派生函数是纯计算。
- **为什么**：① 三个 uuid 都在候选行上，再存一份路径 = 同一事实记两遍、两份可能漂移；
  ② 存下来就把**服务端磁盘路径**带进了对外契约（前端只该拿 uuid 与图片字节）；
  ③ 目录名用 uuid 后连「回查轮次行拿序号」都不需要了。
  **代价**：库里没有「文件实际在哪」的记录；将来出现非派生产物（人工补图 / 对象存储）得重新引入路径字段。
- **改动落点**：`kitsu_ctx_t.h`（命名规则）· `ai_light_orchestrator.h`（`candidate_render_path` +
  `apply_render_report` 的第 2 参 `const http::kitsu_ctx_t&`）· `sqlite_database.cpp`（候选表无路径列）·
  `candidate.h`（无路径字段与键）· 派发段（`out_dir_ = make_iteration_relative_dir(session, l_it.uuid_id_)`，
  本函数不需要 `kitsu_ctx_t`）· `mark_iteration_rendered`（UPDATE 只写状态与数值）· `apply_render_report`
  （D41 现算，D42 只 WARN）· 上传 / 取图 / 定稿路由（同一函数）· **工作机零改动**（它只用
  `make_candidate_file_name` 与 `out_dir_`）· `docs\ue_ai_lighting_frontend.md`（候选对象无路径键）。
- **迁移**：**不用加版本号** —— `sync_schema()` 对已存在的表只 `continue`（不加列不改列不删列），
  所以候选表没有路径列这件事只需 `reg_table` 里不写那两行；已在 v31 的开发库会留下两个没人引用的列，无害。
- **测试**：夹具新增 `server_ctx_`（**不能叫 `ctx_`**，基类那个是 asio `io_context`）+ `candidate_png()` /
  `candidate_exr()` 两个助手（真文件按**同一个函数**造）；12 处 `apply_render_report` 调用带 ctx。
- 回归：全量 **241 用例 / 1969 断言 exit 0**；`ai_light_*` **124 用例 / 1216 断言**；
  `ai_light_dispatch` **23 用例 / 186 断言**。

## 三、剩余待办（按优先级）

1. **重新打包 UE 插件包**（真机验证的唯一前置动作）：
   `src\doodle_lib\exe_warp\ue_exe.cpp:84-128` 的 `installUePath` 每次跑 UE 前都会用
   `kitsu_client->get_ue_plugin(version)` 下载插件包覆盖 `Engine/Plugins/Doodle` ——
   **不打包新包，工作机上还是旧插件，`-run=DoodleLighting` 会找不到类**。打包流程不在本仓库里（由用户/美术侧执行）。
2. **第一次真机跑通一轮**（预期会有编译/运行期修补，因为 UE 代码没在本机编译过）：
   逐个盯 —— ① 两个模块能编译（`bEnableExceptions`、OpenEXR 头、`MovieSceneTools`/`FBX` 依赖）；
   ② commandlet 能起来（`-run=DoodleLighting` + `-AllowCommandletRendering`）；
   ③ 交付机位 FBX 能导入并求出位姿（`GetCameraView` 报错就退化成「相机在原点」）；
   ④ 6 张 PNG + 6 张 EXR 落在**工作机本地** `get_cache_root("ai_light")/<session>/<轮次 uuid>/` 下，
   并逐张上传成功落到服务端按命名规则算出的位置（`candidate_render_path`，D44）；
   ⑤ `render_report.json` 回传后会话真的进 `awaiting_judgement`（若 PNG 上传失败会看到候选被降级成 failed）。
3. **服务端按 D30 对回传 PNG 复算显示空间指标**（`ai_light_stats` 已有实现，但 `apply_render_report`
   目前只沿用回传里的 `metrics`）—— 这是「工作机不算指标」的另一半，缺了它候选的 `metrics_` 就一直是空。
4. 与派发无关、但仍在同一里程碑上的后续：前端页面（§14 + `docs\ue_ai_lighting_frontend.md`）、
   `/judge` 之后的下一轮梯度、`preview_file` 登记（只有被接受的候选会登记）、VLM 评分（`vlm_score_`）。
5. `baseline_level_` 的来源就在 D8 里（真的调
   `auto_task::shot_render_light` 取 `original_map_`/`ue_main_project_path_`/`camera_file_path_`），
   只剩真机确认这些字段在真实工程里非空。

## 四、关键文件路径

| 作用 | 路径 |
| --- | --- |
| 载荷 / 回传 wire 契约 | `src\doodle_core\metadata\ai_light\task_payload.h` |
| 编排器（状态机 + 回传处理） | `src\doodle_lib\http_method\kitsu\ai_light_orchestrator.{h,cpp}` |
| JSON 校验层（严格/宽松读取器） | `src\doodle_lib\http_method\kitsu\ai_light_json_validation.{h,cpp}` |
| 三条接缝 | `src\doodle_lib\http_method\kitsu\ai_light_seam_{spec,candidates,feedback}.{h,cpp}` |
| rig 纯运算 / 共用部件 | `ai_light_rig_ops.{h,cpp}` / `ai_light_seam_common.{h,cpp}` |
| HTTP 路由与注册 | `src\doodle_lib\http_method\kitsu\ai_light.{h,cpp}`、`kitsu.cpp:333`（回传）与 `:338`（候选产物上传） |
| 工作机任务类 | `src\doodle_lib\exe_warp\ai_light_render.{h,cpp}`（相对路径→本地绝对、上传、保留产物） |
| 工作机派发分支 | `src\doodle_lib\http_client\work.cpp:237` |
| 回传客户端方法 | `src\doodle_lib\http_client\kitsu_client.{h,cpp}`（`.h:174` / `.cpp:694`） |
| 候选产物上传客户端方法 | `src\doodle_lib\http_client\kitsu_client.cpp` `upload_ai_light_candidate_render`（复用私有 `upload_asset_file` `:261-290`） |
| UE 启动设施 | `src\doodle_lib\exe_warp\import_and_render_ue.cpp:194-296`、`ue_exe.{h,cpp}` |
| 基线关卡来源 | `src\doodle_lib\http_method\kitsu\auto_task.cpp:768-812` |
| UE 侧现有 commandlet（照它写） | `script\uePlug\Doodle\Source\DoodleEditor\{Public,Private}\DoodleAutoAnimationCommandlet.{h,cpp}` |
| UE 侧插件清单 | `script\uePlug\Doodle\Doodle.uplugin`（引擎里的部署副本 `E:\UnrealEngine\Engine\Plugins\Doodle`） |
| **UE 侧 Runtime 模块（新）** | `script\uePlug\Doodle\Source\DoodleLighting\`（`DoodleLighting.Build.cs`；`Public\DoodleLighting{,Types,Json,Report,Ops}.h` + 同名 `Private\*.cpp`） |
| **UE 侧 Editor 模块（新）** | `script\uePlug\Doodle\Source\DoodleLightingEditor\`（`DoodleLightingEditor.Build.cs`；`Public\DoodleLighting{,Capture,Executor,Commandlet}.h` + 同名 `Private\*.cpp`） |
| 工作机 UE 命令行 | `src\doodle_lib\exe_warp\ai_light_render.cpp:53-58`（`-AllowCommandletRendering` 在这里） |
| 新单测 | `src\test\core\ai_light_dispatch.cpp` |
| 设计文档 | `docs\ue_ai_lighting_plan.md`（§7.x UE 契约、§15.12 服务端/工作机、§15.13 UE 侧、§15.14 本地上传 D38~D43、§15.15 路径现算 D44） |
| 产物路径的**唯一真相** | `src\doodle_core\metadata\kitsu_ctx_t.h` 的 `get_ai_light_iteration_dir` / `get_ai_light_candidate_file`；包装成 `candidate_render_path`（`ai_light_orchestrator.h`，D44） |

## 五、构建 / 测试 / 铁律

```powershell
# 构建（每次先删掉 exe，否则 LNK1168：文件被上一次的 test_main.exe 占着）
Get-Process test_main -EA SilentlyContinue | Stop-Process -Force
Remove-Item 'E:\Doodle\build\Ninja_debug\bin\test_main.exe' -Force -EA SilentlyContinue
$env:TEMP = $env:TMP = 'E:\Doodle\build\vcpkg_tmp'; $env:VCPKG_KEEP_ENV_VARS = 'TEMP;TMP'
& 'E:\Doodle\build_script\build\set_env.ps1' cmake --build 'E:\Doodle\build\Ninja_debug' --target test_main

# 测试（workdir = build\Ninja_debug\bin）
$env:TEMP = $env:TMP = 'E:\Doodle\build\test_tmp'
.\test_main.exe --run_test=ai_light_* --log_level=error --report_level=detailed
```

- **不要跑 `Ninja_release`**（只浪费编译时间）。
- 管道后的 exit code **不可信**：一律回看日志里的 `error C[0-9]+` / `error LNK` / `FAILED:` / `ninja: build stopped`。
- 基线：全量 **241 用例 / 1969 断言全通过、exit 0**；`ai_light_*` **124 用例 / 1216 断言** exit 0；
  `ai_light_dispatch` **23 用例 / 186 断言**（其中 4 个盯 D41 的产物存在性、1 个盯 D44 的路径纯度）。
- 单测坑（见 §15.12 坑 36–40、§15.15 坑 47–50）：`ai_light_candidate::metrics_` 是 JSON 列要用 `get_metrics()`；
  纯同步用例不能用 `DOODLE_FIXTURE_TEST_CASE_CORO`（没有 `co_await` → C4716），要用 `BOOST_FIXTURE_TEST_CASE`；
  夹具的 asio `io_context` 成员叫 `ctx_`，别的上下文别用这个名字（会静默遮蔽）；
  `task_payload.h` 的类型都在 `doodle::ai_light`；`run_ue_assembly_arg` 在 `doodle::import_and_render_ue_ns`；
  `test_main` 里 `g_ctx()` 不可用，成功路径不要依赖它（所以产物落点由参数传 ctx 现算）。
- **UE 侧代码在本机无法编译**：`E:\Doodle\script` 下只有打包好的插件副本（`uePlug\5.7\Plugins\Doodle_*`），
  没有 HostProject、也没有插件打包脚本 —— 那 20 个文件只做了对照 `E:\UnrealEngine\Engine\Source` 的 API 级核对。
- 本轮所有改动 **未 git commit**（由用户自己提交）。
