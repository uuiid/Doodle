# AI 灯光「派发功能」交接文档

> 需求原话（用户）：**「安排 ai_light 工作的派发功能」**。
> 目标：把一轮 6 个候选真正派到工作机 → UE 渲染（MRQ，无 `-run=`）→ 回传落库，让会话从
> `generating` 走到 `awaiting_judgement`（人工判断闸门 2 之前的那一段）。
>
> **当前状态：服务端 + 工作机编译通过、单测全绿（全量 245 用例 / 2110 断言 exit 0）；
> UE 侧两个插件模块源码已写完（新增 `UDoodleLightingRoundRunner`，批 9 已删掉旧的
> SceneCapture/commandlet 路径），但
> **本机无法编译 UE 代码**（`E:\Doodle\script` 下只有打包好的插件副本，没有 HostProject 也没有打包脚本），
> 只做了 API 级核对 —— 真机验证前必须重新打包插件包。**
> 设计记录见 `docs\ue_ai_lighting_plan.md` **§15.12**（服务端/工作机）· **§15.13**（UE 侧）·
> **§15.14**（产物落工作机本地 + HTTP 上传，D38~D43）· **§15.15**（产物路径改「现算」，D44）·
> **§15.16**（候选产物改视频：无 `-run=` 启动 + PIE + MRQ，D45~D50）· **§15.17**（删旧 SceneCapture/commandlet 路径）。

## 一、链路（现在的真实形状）

```
POST /api/doodle/ai/light/session/{id}/iterate
  └─ advance_session（generating 分支）
       ├─ generate_candidates(...)                      6 个 rig + 候选行（**不写路径**，D44）
       ├─ shot_render_light(project_id, task_id_)        基线关卡 / .uproject / 相机文件
       ├─ ai_light_task_payload_t → server_task_info.command_（type_ = ai_light）
       │    载荷里的 out_dir / frames_dir / png_path / exr_path / movie_path 都是**相对路径**（D39）；
       │    另带渲染装配 render_map / level_sequence / movie_pipeline_config（D50：sequence 空 = 派发即失败）
       └─ run_next_task() + broadcast(server-task-info:new)
                    ↓ 工作机（报在线 + 无 running 任务）
http_work::run_task → ai_light_round_distributed::run()
       ├─ 把相对路径拼上工作机本地缓存根（get_cache_root("ai_light")）→ 写进 -DoodleAiLightParams 那份 JSON
       ├─ UnrealEditor-Cmd.exe <uproject> <render_map> -log … -DoodleAiLightParams=<tmp>.json
       │    （**没有** -run=：MRQ 靠 OnBeginFrame 推进，commandlet 里不广播 → 用 PIE 执行器，D45）
       ├─ UE：剥掉序列里的灯光 spawnable → 建 6 个 MRQ job（每 job 一个候选的帧目录）→ 逐 job 在
       │    OnIndividualJobStarted 里把该候选 rig 建进 GEditor->PlayWorld → 出 PNG 帧序列
       ├─ 工作机：帧序列 → create_move(fps = 回传的 frame_rate_) 合成 <候选 uuid>.mp4；首帧复制成海报
       ├─ 逐候选上传：POST /api/doodle/ai/light/candidate/{id}/file/{render|render_movie|render_linear}
       │    海报与成片**都必传**（任一失败 → 该候选判 failed）；EXR 可选（D42）
       ├─ POST /api/doodle/ai/light/render_report/{task_id}（只带状态与数值，不带路径）
       └─ put_job_info（终态）→ 析构时 task_finished() 归还机器；本地产物保留不删（D43）
                    ↓
render_report_route → parse_round_render_report → apply_render_report(sql, ctx, task_id, report)
       ├─ 逐条查 D41：说 rendered 但 `candidate_render_path(ctx, 行, false)` 算出的海报、或
       │    `candidate_movie_path(ctx, 行)` 算出的成片不在 → 降级 failed（两者缺一不可）
       │    （EXR 同法算出、不在只 WARN，D42）
       ├─ 至少一张 rendered → mark_iteration_rendered（轮次 awaiting_judgement + 会话 await）
       └─ error_ 非空 / 一张都没 rendered / 全被降级 → mark_iteration_failed（只写会话 failure_reason_）
```

## 二、各部分现状

### wire 契约（`src\doodle_core\metadata\ai_light\task_payload.h`）
`E:\Doodle\src\doodle_core\metadata\ai_light\task_payload.h`

- `g_ai_light_render_report_file_name{"render_report.json"}` —— 渲染进程写、工作机读。
- `ai_light_task_payload_t`（派发，进 `server_task_info.command_`）：键
  `session_id / iteration / spec_id / out_dir / baseline_level / render_map / level_sequence /
  movie_pipeline_config / frame_begin / frame_end / ue_project_path / camera_file_path / project_dir /
  assembly_files / candidates`。
- `ai_light_assembly_file_t`（**D57 装配同步清单**）：键 `from / to`。
  - `from` = 共享盘/UNC 上的**只读源**（`prj_.path_ / ...`，绝对路径）。
  - `to` = **相对工程缓存根**的相对路径（不带盘符），工作机把它拼到**本机** `user_work_root_` 上。
  - 为什么不复用 `import_and_render_ue_ns::file_copy_info`：后者 `to_` 是服务端算出的缓存根**绝对**路径，
    语义不同 —— 工作机的缓存根与生成端的根本不是同一个盘符/目录。
- `project_dir_`（键 `project_dir`）：`{项目code}/{场景编号}/{uproject 文件名去后缀}`，相对 `user_work_root_`。
- **D57**：`D:/sy_magic/ue_projects/{code}/EP{集数}/{工程名}` 是**每台工作机各自的缓存**，不是网络盘。
  自动组装必须先按 `assembly_files_` 把共享盘素材差分拷进本机缓存，再起 UE
  （多个 UE 同时打开网络盘上的同一个工程是非法的）。**老载荷没有这两个键**：`from_json` 全走
  `contains`，缺键即默认，工作机退回直接用 `ue_project_path_`（滚动升级期间两侧都不崩）。
  冻结用例：`assembly_files_round_trip` / `assembly_files_absent_is_tolerated` /
  `launch_plan_carries_assembly_files`。
- `ai_light_task_candidate_t`：键 `candidate_index / candidate_id / frames_dir / png_path / exr_path / movie_path / rig`
  （rig 含 `ops_`）。
- `round_render_report_t`：键 `candidates / frame_rate / error`（`frame_rate` = 序列 display rate，工作机据此合成视频）。
- `candidate_render_report_t`：键 `candidate_index / candidate_id / status / render_ms / light_count / frame_count / metrics`
  —— **不带路径**（D27：产物落点由服务端现算，见 D44；`frame_count` 只是诊断字段，候选行没有对应列）。
- 路径契约件：`g_ai_light_local_cache_flag` · `make_iteration_relative_dir` · `make_candidate_file_name` ·
  `make_candidate_frames_dir` · `make_candidate_movie_file`；角色枚举
  `ai_light_candidate_file_role{render, render_linear, render_movie}`（**枚举名会进 URL**，改名等于改外部契约）。
  冻结用例：`task_payload_json_keys_are_frozen`。
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
- 渲染装配推导（D8 / D50）：`ai_light_orchestrator.cpp` 派发段
  调 `http::auto_task::shot_render_light_with_sync(io_session.project_id_, io_session.task_id_)`
  （`auto_task.cpp`；返回 `shot_render_light_result{arg_, project_dir_, assembly_files_}`，
  取 `arg_` 的 `original_map_`（→ `baseline_level_`）/ `ue_main_project_path_` / `camera_file_path_` /
  `render_map_` / `level_sequence_import_` / `movie_pipeline_config_` / `begin_time_` / `end_time_`）。
  **注意**：旧的 `shot_render_light(...)` 仍然保留且语义不变（`distributed_task.cpp:22,57` 仍在用），
  它内部就是 `shot_render_light_builder{make_shot_render_light_input(...)}.run()`；
  带同步清单的版本走 `.run_with_sync()`，两条 run 路径阶段顺序完全一致，
  只是后者多一步 `build_assembly_files(l_scene)` 并按 `ue_asset_path_` 逐个 `lexically_relative(scene_ue_path_)`
  生成相对目标（落在工程根之外的目标跳过 + WARN）。
  **两条硬前置**：没有 task_id 或装配抛错 →
  `"AI 灯光会话 {} 第 {} 轮：无法推导本镜头的渲染装配，本轮无法渲染（{}）"`；
  `level_sequence_import_` 空 → `"…：镜头装配没有给出 level sequence（{}），本轮无法渲染"`
  （不再「WARN 后照跑」）。
- 载荷组装（候选 7 成员聚合 + 上述装配字段 + `project_dir_` / `assembly_files_`）、进 `command_`。
  引擎内路线（`iterate` → `launch_plan`）同样带这两项。
- 回传路由：声明 `ai_light.h:158` · 注册 `kitsu.cpp:333`（`/api/doodle/ai/light/render_report/{}`）·
  实现 `ai_light.cpp:790`（先查任务类型必须是 `ai_light`，再解析、落库，回执带 `iteration_state` /
  `session_state` / `failure_reason`）。**不做项目权限检查** —— 调用方是工作机的机器 JWT。
- 客户端方法 `kitsu_client.h:174` / `.cpp:694` `post_ai_light_render_report(uuid, const nlohmann::json&)`
  （接受 200/201，回执非严格解析）。

### 工作机（`src\doodle_lib\exe_warp\ai_light_render.{h,cpp}`）
- 任务类：`ai_light_round_distributed : base_distributed_task`，成员只有
  `ai_light::ai_light_task_payload_t payload_` + `run()`。
  - **四条硬前置**：`ue_project_path_` / `render_map_` / `level_sequence_` / `candidates_` 非空，
    否则抛 `doodle_error`（`movie_pipeline_config_` 空只 WARN）。
  - **起 UE 前先同步装配文件（D57）**：`sync_assembly_files(payload_, l_logger)` ——
    `user_work_root_` 为**本机** UE 工程缓存根，空的则抛
    `"工作机没有配置 UE 工程缓存根（user_work_root），无法把装配文件同步到本地，本轮无法渲染"`；
    `project_dir_` 拼到它下面（空则直接用根），逐条 `FSys::copy_diff(from_, l_prj_dir / to_)`
    （**差分**拷贝：mtime/size 相同就跳过，所以反复跑同一镜头的成本可接受）；
    拼出 `l_uproject = l_prj_dir / <ue_project_path_ 的文件名>`，取不到就抛
    `"装配同步后仍找不到 UE 工程文件 {}，无法启动 UE"`。
    紧接着 `fix_project(l_uproject)` + `fix_config(l_uproject)`（改 `.uproject` 插件段与
    `Config/DefaultEngine.ini` —— 服务端同步过来的 Config 未必是本机要的那份）。
    **`assembly_files_` 为空 = 老服务端载荷**：只 WARN，退回用 `ue_project_path_` 原路径。
  - **起 UE 前先删 `<out_dir>/render_report.json` 残留**（否则渲染进程崩在写报告前会回传上一轮结果）。
  - `async_run_ue({l_uproject.generic_string(), render_map_, "-log", "-AllowStdOutLogVerbosity", "-ForceLogFlush",
    "-Unattended", "-DoodleAiLightParams=<tmp>.json"}, logger, kitsu_client, /*create_lock=*/true, run_time_info)`
    —— 首元素是**同步后**的本机工程路径；**无 `-run=`**（D45）。
  - 报告读回后逐候选：`collect_render_frames()` 按文件名排序收 `frame_*.png` → 少于 2 张记 failed →
    `copy_file(首帧 → 海报)` + `detail::create_move(mp4, logger, make_default_attr(frames), {}, fps)` →
    上传海报（`render`）与成片（`render_movie`），**任一失败 → 该候选 failed**；本轮不产 EXR。
  - 异常也回传「整轮失败」报告（`l_report.error_`），**再** `put_job_info` 落终态（§10.4 契约）。
- 派发分支 `src/doodle_lib/http_client/work.cpp:237`。
- 默认允许类型 `src/doodle_lib/http_method/local/event.cpp:31-38` 含 `ai_light`。
- CMake：`src/doodle_lib/exe_warp/CMakeLists.txt`（PUBLIC `.h` / PRIVATE `.cpp`）。

### UE 侧插件（`script\uePlug\Doodle\Source\DoodleLighting{,Editor}`，20 个文件）
`script\uePlug\Doodle\Source\DoodleLighting\`（11 个文件）· `script\uePlug\Doodle\Source\DoodleLightingEditor\`（9 个文件）

> **批 9（删掉旧的 SceneCapture / commandlet 路径，用户指令）**：`DoodleLightingCommandlet.{h,cpp}` 已删；
> `DoodleLightingCapture.{h,cpp}` 只剩 `ApplyFrozenPostProcessBaseline`（冻结后处理基线，MRQ 路线仍要它，
> 由执行器的 `BuildPostProcessSettings` 调用）；`DoodleLightingExecutor.{h,cpp}` 里
> `PrepareRound`/`FinishRound`/`RenderCandidate`/`GetCameraView` 与 `SwapToWorld`/`NormalizeLevelPath`/
> `G_Capture`/`g_warmup_iterations`/`WritePng`/`WriteExr` 已删（现在编辑器模块 11 → 9 个文件）。
> 详见设计文档 §15.17。

- 机位：**交付机位**（载荷 `camera_file_path_` 的 FBX 经临时 level sequence 导入，只借来求一次姿态，
  随后 `ReleaseDeliveryCamera()` 把导入物拆掉 —— 它不该跟着进 PIE）；分辨率由 MRQ 定
  （有 `movie_pipeline_config_` 就沿用它的，自建配置默认 1920×1080）。
- `Doodle.uplugin` 两条 Module：`DoodleLighting`（Runtime/Default/Win64）、
  `DoodleLightingEditor`（**Editor/Default**/Win64）。
- `DoodleLightingEditor.Build.cs`：带 `Sequencer`/`MovieSceneTools`/`FBX`
  + **`MovieRenderPipelineCore`/`MovieRenderPipelineEditor`/`MovieRenderPipelineRenderPasses`/`MovieRenderPipelineSettings`**
  + `AssetTools`/`EditorSubsystem`/`EditorScriptingUtilities`/`UnrealEd`/`AssetRegistry`。
  批 9：`ImageCore`/`ImageWrapper`/`Imath`/`UEOpenExr`/`UEOpenExrRTTI` 与 `bEnableExceptions = true`
  都是为那个已删的 PNG/EXR 写盘器开的，已撤；`RenderCore`/`RHI` 保留（本机不能编译 UE，不为纯清洁动作冒险）。
- **派发入口（D45）**：**无 `-run=`** 的编辑器启动，只认 `-DoodleAiLightParams=<json>`；
  `FDoodleLightingEditorModule::StartupModule()` 看见这个参数就 `NewObject<UDoodleLightingRoundRunner>` +
  `AddToRoot()` + 挂 `FSlateApplication::Get().OnPreTick()`（先例 `DoodleEditor.cpp:180-184`）。
  批 9：`-run=DoodleLighting`（`UDoodleLightingCommandlet`）**已删除** —— 候选产物是视频，commandlet 里 MRQ 一帧都渲不出（见下条），且已无调用者。
- **渲染方式 = MRQ + PIE 执行器**（取代当初的 D36 `USceneCaptureComponent2D`）：
  `UMoviePipelineQueueSubsystem`（`GEditor->GetEditorSubsystem<>()`）+ `AllocateNewJob` 逐候选一个 job +
  `UMoviePipelinePIEExecutor` + 逐 job 钩子 `OnIndividualJobStarted()`（在 `UMoviePipeline::Initialize` 之前、
  PIE 已起）里把 rig 建进 `GEditor->PlayWorld` —— PIE 世界允许运行时 spawn 的 actor 参与渲染。
  序列里的灯光 spawnable 在内存里剥掉（`GetSpawnableCount/GetSpawnable/IsA<ALight>`，**倒着**遍历因为
  `GetSpawnable(i)` 按 guid 排序）；序列**永不落盘**。
- **为什么不能是 `-run=` commandlet**：① MRQ 靠 `FCoreDelegates::OnBeginFrame` 推进，全引擎只有
  `LaunchEngineLoop.cpp:5663`（`FEngineLoop::Tick`）广播它，`CommandletHelpers::TickEngine` 不广播 →
  管线建起来却一帧不渲；② `MoviePipeline::FindCurrentWorld()` 只认 `Game`/`PIE` 世界。
- **D37 = 不复制关卡、不落盘**（关卡由载荷 `render_map` 经 `LoadObject<UWorld>` 取来，交给编辑器跑 PIE：
  `DoodleLightingRoundRunner.cpp:324`；不 `DuplicateAsset`/不 `SaveLoadedAssets`/不 `MarkPackageDirty`。
  批 9 起**不再有「换 GWorld」那一步** —— `SwapToWorld` 已随旧路径删除）；
  **D31 = 以 `rig.lights_` 全量重建**（`rig.ops_` 只审计，`diff_rigs` 永不产生 add）；**D30 = UE 不算指标**（`metrics` 不写）。

### 测试（`src/test/core/ai_light_dispatch.cpp`，已登记 `src/test/core/CMakeLists.txt`）
- **24 用例**：解析组 8 例（严格/宽松分界逐条）、落库组 5 例（两条出口 + 部分失败仍前进）、
  归属组 5 例（404 / 400 / judged 拒回填 / 原因截断）、产物存在性 5 例（D41，含「海报在、成片缺」）、
  路径纯度 1 例（D44）、wire 契约冻结 1 例（`task_payload_json_keys_are_frozen`）。
- 夹具造**真文件**：每个候选三个探针（海报 / 成片 / EXR），走 `write_probe_file()` 与
  `candidate_png()` / `candidate_movie()` / `candidate_exr()` —— D41 判的是「算出来的落点上有没有文件」，
  海报与成片**缺一不可**。

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
  候选 `frames_dir = <候选 uuid>/`、海报 `<候选 uuid>.png`、成片 `<候选 uuid>.mp4`、可选 `<候选 uuid>.exr`）；
  命名服务端定、根工作机定。构造器在 `task_payload.h`：`make_iteration_relative_dir()` /
  `make_candidate_frames_dir()` / `make_candidate_file_name()` / `make_candidate_movie_file()`。
  候选行**不存路径**（D44）：要用时按 `candidate_render_path(ctx, 行, linear)` / `candidate_movie_path(ctx, 行)` 现算。
- **服务端新路由（D40）**：`candidate_file_route`（role 段是 scoped enum
  `ai_light_candidate_file_role{render, render_linear, render_movie}`，路由宏自动展开白名单正则）·
  注册 `kitsu.cpp:338` `/api/doodle/ai/light/candidate/{}/file/{}` · 实现 `ai_light.cpp`。
  要求 `application/octet-stream`；落点由候选行现算（海报/EXR 走 `candidate_render_path`、成片走 `candidate_movie_path`）；
  轮次 `judged` 拒绝、`awaiting_judgement` 允许重传；
  先写 `<目标>.uploading` 再 rename（前端不会读到半张图/半截 mp4）；成功回 **204**。
  **故意不复用 `up_file_base`**（那套是按项目派生目录的，且跨卷 rename 会抛）。
  取成片另有一条 GET：`candidate_movie_route`（`/api/doodle/ai/light/candidate/{}/movie`，`make_msg` 自带 Range）。
- **工作机**：`exe_warp/ai_light_render.cpp` 加 `make_ue_payload()`（相对 → 本地绝对）；
  **上传必须在回传报告之前**（服务端 D41 靠查文件在不在判可判断性）；海报与成片都必传、EXR 尽力而为（D42）；
  本地产物保留不删（D43）。客户端方法
  `kitsu_client::upload_ai_light_candidate_file(uuid, FSys::path, ai_light::ai_light_candidate_file_role)`
  （URL 段用 `magic_enum::enum_name(in_role)`）复用私有 `upload_asset_file`（`kitsu_client.cpp:261-290`，只认 204）。
  收产物时**位置从载荷取**（**D51**）：拿 `make_ue_payload()` 拼好的那份副本，按候选 **uuid** 反查候选，
  用它的 `frames_dir_` / `png_path_` / `movie_path_`。**不按命名规则重算**（那等于工作机再实现一遍服务端命名，
  版本偏斜会算到不存在的目录、白丢一轮），**也不按 `candidate_index_`**（弱键，错配会把 A 的成片挂到 B 的
  uuid 上 —— 美术对着别人的灯评判）。代价：多一个「报告里的候选不在载荷里」分支（该候选判 failed）。
- **UE 侧已改**：新增 `UDoodleLightingRoundRunner`（一轮的驱动者）+ `ApplyCandidateToWorld` /
  `PrepareAnchorCamera`（建灯与全局量，不抓帧不写文件）—— 出帧交给 MRQ。
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
  （D41 现算，D42 只 WARN）· 上传 / 取海报 / 取成片 / 定稿路由（同一族函数）· 工作机（三个构造器 + `out_dir_`）·
  `docs\ue_ai_lighting_frontend.md`（候选对象无路径键；海报与成片两条取用路由）。
- **迁移**：**不用加版本号** —— `sync_schema()` 对已存在的表只 `continue`（不加列不改列不删列），
  所以候选表没有路径列这件事只需 `reg_table` 里不写那两行；已在 v31 的开发库会留下两个没人引用的列，无害。
- **测试**：夹具新增 `server_ctx_`（**不能叫 `ctx_`**，基类那个是 asio `io_context`）+ `candidate_png()` /
  `candidate_movie()` / `candidate_exr()` 助手（真文件按**同一个函数**造）；`apply_render_report` 的 12 处调用带 ctx。
- 回归：全量 **245 用例 / 2110 断言 exit 0**；`ai_light_*` **128 用例 / 1357 断言**；
  `ai_light_dispatch` **24 用例**、`ai_light_candidates` **17 用例**（命令与铁律见 §五）。

## 三、剩余待办（按优先级）

1. **重新打包 UE 插件包**（真机验证的唯一前置动作）：
   `src\doodle_lib\exe_warp\ue_exe.cpp` 的 `installUePath` 每次跑 UE 前都会用
   `kitsu_client->get_ue_plugin(version)` 下载插件包覆盖 `Engine/Plugins/Doodle` ——
   **不打包新包，工作机上还是旧插件，`-DoodleAiLightParams` 没人认、`UDoodleLightingRoundRunner` 不存在**。
   打包流程不在本仓库里（由用户/美术侧执行）。
2. **第一次真机跑通一轮**（预期会有编译/运行期修补，因为 UE 代码没在本机编译过）：
   逐个盯 —— ① 两个模块能编译（MRQ 四个模块依赖、`MoviePipeline*` 头、`bEnableExceptions`）；
   ② 渲染进程起得来、`UDoodleLightingRoundRunner::Main` 真的被 `OnPreTick` 调到；
   ③ 交付机位 FBX 能导入并求出位姿（失败会给导入错误或「载荷里没有交付机位 FBX」）；
   ④ MRQ 6 个 job 逐个渲：每个候选的 `frames_dir` 下出现 `frame_*.png`（至少 2 张），
   合成出 `<候选 uuid>.mp4` + 首帧海报，落在**工作机本地** `get_cache_root("ai_light")/<session>/<轮次 uuid>/`；
   ⑤ 海报与 mp4 都上传成功、`render_report.json` 回传后会话真的进 `awaiting_judgement`
   （任一产物上传失败会看到候选被降级成 failed，全灭则只写 `failure_reason_`）。
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
| 回传客户端方法 | `src\doodle_lib\http_client\kitsu_client.{h,cpp}` `post_ai_light_render_report(uuid, const nlohmann::json&)` |
| 候选产物上传客户端方法 | `kitsu_client::upload_ai_light_candidate_file(uuid, FSys::path, ai_light::ai_light_candidate_file_role)`（角色枚举名进 URL 段；复用私有 `upload_asset_file` `:261-290`） |
| UE 启动设施 | `src\doodle_lib\exe_warp\import_and_render_ue.cpp:194-296`、`ue_exe.{h,cpp}` |
| 基线关卡来源 | `src\doodle_lib\http_method\kitsu\auto_task.cpp:768-812` |
| UE 侧现有 commandlet（照它写） | `script\uePlug\Doodle\Source\DoodleEditor\{Public,Private}\DoodleAutoAnimationCommandlet.{h,cpp}` |
| UE 侧插件清单 | `script\uePlug\Doodle\Doodle.uplugin`（引擎里的部署副本 `E:\UnrealEngine\Engine\Plugins\Doodle`） |
| **UE 侧 Runtime 模块（新）** | `script\uePlug\Doodle\Source\DoodleLighting\`（`DoodleLighting.Build.cs`；`Public\DoodleLighting{,Types,Json,Report,Ops}.h` + 同名 `Private\*.cpp`） |
| **UE 侧 Editor 模块（新）** | `script\uePlug\Doodle\Source\DoodleLightingEditor\`（`DoodleLightingEditor.Build.cs`；`Public\DoodleLighting{,Capture,Executor,RoundRunner}.h` + 同名 `Private\*.cpp`；派发入口是 `DoodleLightingRoundRunner`，`Executor` 提供 `PrepareAnchorCamera`/`ApplyCandidateToWorld`，`Capture` 只剩 `ApplyFrozenPostProcessBaseline` —— 批 9 已删 `DoodleLightingCommandlet.{h,cpp}`） |
| 工作机 UE 命令行 | `src\doodle_lib\exe_warp\ai_light_render.cpp`（**无 `-run=`**，`-DoodleAiLightParams=<json>`；帧→mp4 合成与两个上传也在这里） |
| 新单测 | `src\test\core\ai_light_dispatch.cpp` |
| 设计文档 | `docs\ue_ai_lighting_plan.md`（§7.x UE 契约、§15.12 服务端/工作机、§15.13 UE 侧、§15.14 本地上传 D38~D43、§15.15 路径现算 D44、§15.16 候选产物改视频 D45~D50） |
| 产物路径的**唯一真相** | `src\doodle_core\metadata\kitsu_ctx_t.h` 的 `get_ai_light_iteration_dir` / `get_ai_light_candidate_file` / `get_ai_light_candidate_frames_dir` / `get_ai_light_candidate_movie_file`；包装成 `candidate_render_path` / `candidate_movie_path` / `candidate_frames_dir`（`ai_light_orchestrator.h`，D44） |

## 五、构建 / 测试 / 铁律

```powershell
# 构建（每次先删掉 exe，否则 LNK1168：文件被上一次的 test_main.exe 占着）
Get-Process test_main -EA SilentlyContinue | Stop-Process -Force
Remove-Item 'E:\Doodle\build\Ninja_debug\bin\test_main.exe' -Force -EA SilentlyContinue
$env:TEMP = $env:TMP = 'E:\Doodle\build\vcpkg_tmp'; $env:VCPKG_KEEP_ENV_VARS = 'TEMP;TMP'
& 'E:\Doodle\build_script\build\set_env.ps1' cmake --build 'E:\Doodle\build\Ninja_debug' --target test_main

# 测试（**在仓库根目录**跑，不要 cd 到 bin）
$env:TEMP = $env:TMP = 'E:\Doodle\build\Ninja_debug\test_tmp'
.\build\Ninja_debug\bin\test_main.exe --run_test=ai_light_* --log_level=error --report_level=detailed
```

- **必须在仓库根目录运行**：夹具用 `FSys::temp_directory_path()` 建临时库，而 `app_base` 会在
  当前目录下解析自己的数据根 —— 用 `workdir=build\Ninja_debug\bin` 跑会让**每个**用例在夹具构造阶段
  就死在 `doodle::orm::sqlite_orm_exception: 14: unable to open database file`，
  看起来像「全部用例都坏了」，其实只是启动目录不对。另外记得先建好 `$env:TEMP` 指向的目录。
- **构建前必须先跑 `build_script\build\set_env.ps1`**（它内部 `vcvars64.bat` 灌 MSVC 环境再调 cmake）：
  否则会看到 `doodle_core_pch.h(7): fatal error C1083: Cannot open include file: 'chrono'` ——
  那是**环境缺失**，不是代码错误，别去改代码。
- **不要跑 `Ninja_release`**（只浪费编译时间）。
- 管道后的 exit code **不可信**：一律回看日志里的 `error C[0-9]+` / `error LNK` / `FAILED:` / `ninja: build stopped`。
- **`doodle_lib` 的 DLL 导出宏是 `test_main` 能否链接的前提**：`src\doodle_lib\core\http\` 下新增的
  自由函数若声明上没有 `DOODLELIB_API`，本模块内能编过、`doodle_lib.dll` 也能链出来，
  但 `test_main` 会报 `LNK2019: 无法解析的外部符号 doodle::http::xxx`。
  排查用：`dumpbin /SYMBOLS <obj>` 确认符号**已定义** → 看 `test_main.rsp` 确认 `.lib` **已传给链接器** →
  `dumpbin` 查 `.lib` 索引与 DLL 导出表（这才是真相）。修法：加
  `#include <doodle_lib/configure/doodle_lib_export.h>` 并在**声明**前加 `DOODLELIB_API`（定义处不用改）。
- 基线：全量 **245 用例 / 2110 断言全通过、exit 0**；`ai_light_*` **128 用例 / 1357 断言** exit 0；
  `ai_light_dispatch` **24 用例**（其中 5 个盯 D41 的产物存在性、1 个盯 D44 的路径纯度、1 个冻结 wire 键名）、
  `ai_light_candidates` **17 用例**（其中 1 个钉 `diff_rigs` 的全局量 op）。
- **`nlohmann::json{x}` 永远得到数组 `[x]`**（连字符串/枚举也一样）：要对象必须写拷贝初始化
  `nlohmann::json j = x;`。别处踩过的后果是静默的 —— 数组上 `contains("key")` 恒 false，
  所以「键没写进去」和「键名写错了」都表现为**悄悄什么都不做**：`ai_light_rig_ops.cpp` 的全局量 diff
  就是**两个错叠在一起**（花括号 + 键名多写了尾下划线）才让「调色改了却不下发 op」，所以现在有单测钉它
  （见 §15.16 坑 57）。
- 单测坑（见 §15.12 坑 36–40、§15.15 坑 47–50）：`ai_light_candidate::metrics_` 是 JSON 列要用 `get_metrics()`；
  纯同步用例不能用 `DOODLE_FIXTURE_TEST_CASE_CORO`（没有 `co_await` → C4716），要用 `BOOST_FIXTURE_TEST_CASE`；
  夹具的 asio `io_context` 成员叫 `ctx_`，别的上下文别用这个名字（会静默遮蔽）；
  `task_payload.h` 的类型都在 `doodle::ai_light`；`run_ue_assembly_arg` 在 `doodle::import_and_render_ue_ns`；
  `test_main` 里 `g_ctx()` 不可用，成功路径不要依赖它（所以产物落点由参数传 ctx 现算）。
- **D57 前置**：`http_listener_port_shm` 那三个既有缺陷（`offset_t` 命名空间、缺 `DOODLELIB_API`、
  测试里 `g_io_context` 少了限定）必须先修，否则 `test_main` 根本构建不出来。详见
  `docs/ue_ai_lighting_plan.md` §15.18。该套件本身在本机仍失败 5 例（运行时 `CreateFileMapping`
  一律「拒绝访问」，非本次改动引入，未修）。
- **UE 侧代码在本机无法编译**：`E:\Doodle\script` 下只有打包好的插件副本（`uePlug\5.7\Plugins\Doodle_*`），
  没有 HostProject、也没有插件打包脚本 —— 那 20 个文件只做了对照 `E:\UnrealEngine\Engine\Source` 的 API 级核对。
- 本轮所有改动 **未 git commit**（由用户自己提交）。
