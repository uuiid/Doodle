# AI 灯光「派发功能」交接文档

> 需求原话（用户）：**「安排 ai_light 工作的派发功能」**。
> 目标：把一轮 6 个候选真正派到工作机 → UE commandlet 渲染 → 回传落库，让会话从
> `generating` 走到 `awaiting_judgement`（人工判断闸门 2 之前的那一段）。
>
> **当前状态：批 0~6 全部落盘 —— 服务端 + 工作机编译通过、单测全绿（`ai_light_dispatch` 22 用例，
> 全量 240 用例 / 1944 断言 exit 0）；UE 侧两个插件模块（批 4）源码已写完，但
> **本机无法编译 UE 代码**（`E:\Doodle\script` 下只有打包好的插件副本，没有 HostProject 也没有打包脚本），
> 只做了 API 级核对 —— 真机验证前必须重新打包插件包。**
> 设计记录见 `docs\ue_ai_lighting_plan.md` **§15.12**（服务端/工作机）· **§15.13**（UE 侧）·
> **§15.14**（批 6：产物落工作机本地 + HTTP 上传，D38~D43）。

## 一、链路（现在的真实形状）

```
POST /api/doodle/ai/light/session/{id}/iterate
  └─ advance_session（generating 分支）
       ├─ generate_candidates(...)                      6 个 rig + 候选行产物路径（服务端绝对路径）
       ├─ shot_render_light(project_id, task_id_)        基线关卡 / .uproject / 相机文件
       ├─ ai_light_task_payload_t → server_task_info.command_（type_ = ai_light）
       │    载荷里 out_dir / png_path / exr_path 都是**相对路径**（D39）
       └─ run_next_task() + broadcast(server-task-info:new)
                    ↓ 工作机（报在线 + 无 running 任务）
http_work::run_task → ai_light_round_distributed::run()
       ├─ 把相对路径拼上工作机本地缓存根（get_cache_root("ai_light")）→ 写进 -Params 的 JSON   ← 批 6
       ├─ UnrealEditor-Cmd.exe <uproject> … -run=DoodleLighting -Params=<tmp>.json   ← 批 4
       ├─ 读 <本地 out_dir>/render_report.json
       ├─ 逐候选上传：POST /api/doodle/ai/light/candidate/{id}/file/{render|render_linear}   ← 批 6
       │    PNG 必传（失败 → 该候选判 failed）；EXR 尽力而为（D42）
       ├─ POST /api/doodle/ai/light/render_report/{task_id}（只带状态与数值，不带路径）
       └─ put_job_info（终态）→ 析构时 task_finished() 归还机器；本地产物保留不删（D43）
                    ↓
render_report_route → parse_round_render_report → apply_render_report
       ├─ 逐条查 D41：说 rendered 但 PNG 不在库里那条路径 → 降级 failed；EXR 悬空 → 清空路径（D42）
       ├─ 至少一张 rendered → mark_iteration_rendered（轮次 awaiting_judgement + 会话 await）
       └─ error_ 非空 / 一张都没 rendered / 全被降级 → mark_iteration_failed（只写会话 failure_reason_）
```

## 二、已完成的工作

### 批 0 · wire 契约（新文件，JSON 键已冻结）
`E:\Doodle\src\doodle_core\metadata\ai_light\task_payload.h`（157 行）

- `g_ai_light_render_report_file_name{"render_report.json"}` `:37` —— commandlet 写、工作机读。
- `ai_light_task_payload_t` `:73`（派发，进 `server_task_info.command_`）：键
  `session_id / iteration / spec_id / out_dir / baseline_level / ue_project_path / camera_file_path / candidates`。
- `ai_light_task_candidate_t` `:44`：键 `candidate_index / candidate_id / png_path / exr_path / rig`（rig 含 `ops_`）。
- `round_render_report_t` `:143`：键 `candidates / error`。
- `candidate_render_report_t` `:114`：键 `candidate_index / candidate_id / status / render_ms / light_count / metrics`
  —— **不带路径**（D27；落库沿用候选行里已有的路径）。
- 已登记：`src/doodle_core/metadata/ai_light/CMakeLists.txt`、伞形头 `.../ai_light/ai_light.h`。

### 批 1 · 服务端回传处理（编排器）
`E:\Doodle\src\doodle_lib\http_method\kitsu\ai_light_orchestrator.cpp`（949 行）

| 函数 | 位置 | 职责 |
| --- | --- | --- |
| `brief_text` | `.cpp:85` | 截断到 512 + `"...(已截断)"`（`brief_error` `:92` 改为调它） |
| `parse_round_render_report` | `.cpp:760` | **严格**：`candidates` / 元素的 `candidate_id` / `status`（`json_val::require_enum<candidate_render_status>`，不许静默回落）→ 502；**宽松**：`candidate_index` / `render_ms` / `light_count` / `metrics` / 轮级 `error` 缺失取默认、形状不符丢该字段 + WARN |
| `mark_iteration_failed` | `.cpp:828` | 整轮失败收尾：**只**写 `session.failure_reason_`，轮次状态与候选行一概不动（D28） |
| `apply_render_report` | `.cpp:852` | 两条出口（`:864`）：有 `rendered` → 沿用库里路径 → 复用既有 `mark_iteration_rendered` 落库；否则 → 失败 |

声明在 `ai_light_orchestrator.h:142 / :155 / :161`（都带 `DOODLELIB_API`）。

### 批 2 · 服务端派发 + 回传路由
- 基线关卡推导（D8 从「只透传」改成「真的去问」）：`ai_light_orchestrator.cpp:282-295`
  调 `http::auto_task::shot_render_light(io_session.project_id_, io_session.task_id_)`（`auto_task.cpp:768-812`），
  取 `original_map_` / `ue_main_project_path_` / `camera_file_path_`。失败只 WARN + 空串，不让整轮失败。
- 载荷组装 `:339-346`、进 `command_` `:395`。
- 回传路由：声明 `ai_light.h:158` · 注册 `kitsu.cpp:333`（`/api/doodle/ai/light/render_report/{}`）·
  实现 `ai_light.cpp:790`（先查任务类型必须是 `ai_light`，再解析、落库，回执带 `iteration_state` /
  `session_state` / `failure_reason`）。**不做项目权限检查** —— 调用方是工作机的机器 JWT。
- 客户端方法 `kitsu_client.h:174` / `.cpp:694` `post_ai_light_render_report(uuid, const nlohmann::json&)`
  （接受 200/201，回执非严格解析）。
- 顺手修掉 `ai_light.h` 里 `iterate_route` 上方过时的「本轮返回 501」注释。

### 批 3 · 工作机
- 新任务类 `src/doodle_lib/exe_warp/ai_light_render.{h,cpp}`（41 / 96 行）：
  `ai_light_round_distributed : base_distributed_task`，成员只有 `ai_light::ai_light_task_payload_t payload_` + `run()`。
  - 三条硬前置：`baseline_level_` / `ue_project_path_` / `candidates_` 非空，否则抛 `doodle_error`。
  - **起 UE 前先删 `<out_dir>/render_report.json` 残留**（否则 commandlet 崩在写报告前会回传上一轮结果）。
  - `async_run_ue({... "-run=DoodleLighting", "-Params=<tmp>.json"}, logger, kitsu_client, /*create_lock=*/true, run_time_info)`。
  - 异常也回传「整轮失败」报告（`l_report.error_`），**再** `put_job_info` 落终态（§10.4 契约）。
- 派发分支 `src/doodle_lib/http_client/work.cpp:237`。
- 默认允许类型 `src/doodle_lib/http_method/local/event.cpp:31-38` 加入 `ai_light`。
- CMake：`src/doodle_lib/exe_warp/CMakeLists.txt`（PUBLIC `.h` / PRIVATE `.cpp`）。

### 批 4 · UE 侧（两个插件模块，20 个新文件）
`script\uePlug\Doodle\Source\DoodleLighting\`（11 个文件）· `script\uePlug\Doodle\Source\DoodleLightingEditor\`（9 个文件）

- 开工前用户拍板两件事：预览用**交付机位**（载荷 `camera_file_path_` 的 FBX 经临时 level sequence 导入）、
  预览分辨率 **1920×1080 固定常量**（不进 wire 契约）。
- `Doodle.uplugin` 加两条 Module：`DoodleLighting`（Runtime/Default/Win64）、
  `DoodleLightingEditor`（**Editor/Default**/Win64 —— 必须是 `Default`，`-run=X` 的类查找在插件加载之后）。
- `DoodleLightingEditor.Build.cs`：去掉 MRQ 四件，加 `Sequencer`/`MovieSceneTools`/`FBX`/`Imath`/`UEOpenExr`/`UEOpenExrRTTI` + `bEnableExceptions = true`。
- 入口：`-run=DoodleLighting` → `UDoodleLightingCommandlet`（引擎按名字找，无需注册），只认 `-Params=<json>`（D29）。
- **渲染方式拍板 D36 = `USceneCaptureComponent2D` 而非 MRQ**（MRQ 要第二个进程驱动、无单帧/同步 API；
  LDR 通道 `RTF_RGBA8_SRGB`+`SCS_FinalColorLDR` → PNG，HDR 通道 `RTF_RGBA16f`+`SCS_FinalColorHDR` → EXR，
  8 次丢弃式预热抓帧让 Lumen 收敛）。
- **D37 = 不复制关卡**（`LoadObject<UWorld>` 后换 `GWorld`，不 `DuplicateAsset`/不 `SaveLoadedAssets`/不 `MarkPackageDirty`）；
  **D31 = 以 `rig.lights_` 全量重建**（`rig.ops_` 只审计，`diff_rigs` 永不产生 add）；**D30 = UE 不算指标**（`metrics` 不写）。
- 工作机命令行补 `-AllowCommandletRendering`（`src\doodle_lib\exe_warp\ai_light_render.cpp:53-58`）——
  `CommandletHelpers::TickEngine` 少这个开关不 tick 渲染。

### 批 5 · 测试
- 新套件 `src/test/core/ai_light_dispatch.cpp`（530 行，**18 用例**；已登记 `src/test/core/CMakeLists.txt`）：
  解析组 8 例（严格/宽松分界逐条）、落库组 5 例（两条出口 + 部分失败仍前进 + 路径来自库）、归属组 5 例
  （404 / 400 / judged 拒回填 / 原因截断）。
- 回归：全量 **236 用例 / 1847 断言 exit 0**（上一轮 218 / 1781）；`ai_light_*` **119 用例 / 1094 断言**。
- 文档：`docs\ue_ai_lighting_plan.md` §15.12。

### 批 6 · 产物落工作机本地 + HTTP 上传（用户改方案）
> 用户原话：「修改方案, 这里的输出目录改为工作机本地目录, 在输出后, 使用 http 上传, 并记录 使用
> `http::kitsu_ctx_t` 类中, 生成的目录不在 网络盘/UNC 挂载, 使用 http 上传, 而在
> `src/doodle_lib/core/entity_path.h` `entity_path.cpp` 文件中, 生成的目录才会使用 网络盘/UNC 挂载」

- **目录归属（D38，全局规则）**：`http::kitsu_ctx_t` 派生的目录（`root_` 底下这一片）只保证
  **同一台机器同卷**、**不是**共享盘 —— 那份 `root_` 是**服务端**的 `D:/kitsu_data`
  （`launch/kitsu_supplement.cpp:150`），工作机对它的可见性纯属偶然；工作机产物一律写
  `core_set::get_set().get_cache_root("ai_light")`。只有 `core::entity_path` 派生的项目/资产目录才在网络盘/UNC
  （判定 `FSys::is_windows_remote_path`，`file_sys.cpp:209-211`）。
- **契约（D39）**：载荷里的路径一律**相对**（`out_dir = <session_uuid>/it_<NNN>`，
  候选 `<candidate_uuid>.png|.exr`）；命名服务端定、根工作机定。构造器在
  `task_payload.h`：`make_iteration_relative_dir()` / `make_candidate_file_name()`。
  库里的候选行仍然存**服务端绝对路径**（`kitsu_ctx_t::get_ai_light_candidate_file`）。
- **服务端新路由（D40）**：`candidate_file_route`（`ai_light.h:166/:187`，role 段是 scoped enum
  `ai_light_candidate_file_role{render, render_linear}`，路由宏自动展开白名单正则）·
  注册 `kitsu.cpp:338` `/api/doodle/ai/light/candidate/{}/file/{}` · 实现 `ai_light.cpp:828`。
  要求 `application/octet-stream`；落点取自**库里**候选行；轮次 `judged` 拒绝、`awaiting_judgement` 允许重传；
  先写 `<目标>.uploading` 再 rename（前端不会读到半张图）；成功回 **204**。
  **故意不复用 `up_file_base`**（那套是按项目派生目录的，且跨卷 rename 会抛）。
- **工作机**：`exe_warp/ai_light_render.cpp` 加 `make_ue_payload()`（相对 → 本地绝对）；
  **上传必须在回传报告之前**（服务端 D41 靠查文件在不在判可判断性）；PNG 必传、EXR 尽力而为（D42）；
  本地产物保留不删（D43）。客户端方法 `kitsu_client::upload_ai_light_candidate_render(uuid, path, bool linear)`
  复用私有 `upload_asset_file`（`kitsu_client.cpp:261-290`，只认 204）。
- **UE 侧一行未改**：UE 只按载荷给的 `PngPath`/`ExrPath` 写文件、按 `OutDir` 写报告 —— 工作机把绝对路径
  塞进载荷即可，批 4 那 20 个文件不动（也就没有加重「本机无法编译 UE」这个缺口）。
- **服务端回传增强**：`apply_render_report` 加 D41（PNG 不在库里那条路径 → 该候选降级 `failed`，
  全被降级 → `failure_reason_ = "轮次 {} 的 {} 个候选渲染成功但产物文件缺失，本轮无可判断结果"`，
  轮次留在 `rendering`）与 D42（EXR 悬空 → 清空库路径）。计数只数**被降级的**，回传里本来就 failed 的不算。
- **测试**：`ai_light_dispatch.cpp` 新增 4 个 D41 用例（13 / 10 / 12 / 11 断言）；夹具改成造**真文件**
  （`media_dir_` + `write_probe_file()`），否则所有「推进到 `awaiting_judgement`」的旧用例会因降级而红。
- 回归：全量 **240 用例 / 1944 断言 exit 0**（上轮 236 / 1847）。文档：§15.14。

## 三、剩余待办（按优先级）

1. **重新打包 UE 插件包**（真机验证的唯一前置动作）：
   `src\doodle_lib\exe_warp\ue_exe.cpp:84-128` 的 `installUePath` 每次跑 UE 前都会用
   `kitsu_client->get_ue_plugin(version)` 下载插件包覆盖 `Engine/Plugins/Doodle` ——
   **不打包新包，工作机上还是旧插件，`-run=DoodleLighting` 会找不到类**。打包流程不在本仓库里（由用户/美术侧执行）。
2. **第一次真机跑通一轮**（预期会有编译/运行期修补，因为 UE 代码没在本机编译过）：
   逐个盯 —— ① 两个模块能编译（`bEnableExceptions`、OpenEXR 头、`MovieSceneTools`/`FBX` 依赖）；
   ② commandlet 能起来（`-run=DoodleLighting` + `-AllowCommandletRendering`）；
   ③ 交付机位 FBX 能导入并求出位姿（`GetCameraView` 报错就退化成「相机在原点」）；
   ④ 6 张 PNG + 6 张 EXR 落在**工作机本地** `get_cache_root("ai_light")/<session>/it_001/` 下（批 6 起），
   并逐张上传成功落在服务端库路径上（`kitsu_ctx_t::get_ai_light_candidate_file`）；
   ⑤ `render_report.json` 回传后会话真的进 `awaiting_judgement`（若 PNG 上传失败会看到候选被降级成 failed）。
3. **服务端按 D30 对回传 PNG 复算显示空间指标**（`ai_light_stats` 已有实现，但 `apply_render_report`
   目前只沿用回传里的 `metrics`）—— 这是「工作机不算指标」的另一半，缺了它候选的 `metrics_` 就一直是空。
4. 与派发无关、但仍在同一里程碑上的后续：前端页面（§14 + `docs\ue_ai_lighting_frontend.md`）、
   `/judge` 之后的下一轮梯度、`preview_file` 登记（只有被接受的候选会登记）、VLM 评分（`vlm_score_`）。
5. `baseline_level_` 的来源**已经在批 2 落地**（D8 从「只透传」改成真的调
   `auto_task::shot_render_light` 取 `original_map_`/`ue_main_project_path_`/`camera_file_path_`），
   §15.12 里那条「M2 阻断项」可以划掉；只剩真机确认这些字段在真实工程里非空。

## 四、关键文件路径

| 作用 | 路径 |
| --- | --- |
| 载荷 / 回传 wire 契约 | `src\doodle_core\metadata\ai_light\task_payload.h` |
| 编排器（状态机 + 回传处理） | `src\doodle_lib\http_method\kitsu\ai_light_orchestrator.{h,cpp}` |
| JSON 校验层（严格/宽松读取器） | `src\doodle_lib\http_method\kitsu\ai_light_json_validation.{h,cpp}` |
| 三条接缝 | `src\doodle_lib\http_method\kitsu\ai_light_seam_{spec,candidates,feedback}.{h,cpp}` |
| rig 纯运算 / 共用部件 | `ai_light_rig_ops.{h,cpp}` / `ai_light_seam_common.{h,cpp}` |
| HTTP 路由与注册 | `src\doodle_lib\http_method\kitsu\ai_light.{h,cpp}`、`kitsu.cpp:333`（回传）与 `:338`（候选产物上传） |
| 工作机任务类 | `src\doodle_lib\exe_warp\ai_light_render.{h,cpp}`（批 6：相对→本地绝对、上传、保留产物） |
| 工作机派发分支 | `src\doodle_lib\http_client\work.cpp:237` |
| 回传客户端方法 | `src\doodle_lib\http_client\kitsu_client.{h,cpp}`（`.h:174` / `.cpp:694`） |
| 候选产物上传客户端方法（批 6） | `src\doodle_lib\http_client\kitsu_client.cpp` `upload_ai_light_candidate_render`（复用私有 `upload_asset_file` `:261-290`） |
| UE 启动设施 | `src\doodle_lib\exe_warp\import_and_render_ue.cpp:194-296`、`ue_exe.{h,cpp}` |
| 基线关卡来源 | `src\doodle_lib\http_method\kitsu\auto_task.cpp:768-812` |
| UE 侧现有 commandlet（照它写） | `script\uePlug\Doodle\Source\DoodleEditor\{Public,Private}\DoodleAutoAnimationCommandlet.{h,cpp}` |
| UE 侧插件清单 | `script\uePlug\Doodle\Doodle.uplugin`（引擎里的部署副本 `E:\UnrealEngine\Engine\Plugins\Doodle`） |
| **UE 侧 Runtime 模块（新）** | `script\uePlug\Doodle\Source\DoodleLighting\`（`DoodleLighting.Build.cs`；`Public\DoodleLighting{,Types,Json,Report,Ops}.h` + 同名 `Private\*.cpp`） |
| **UE 侧 Editor 模块（新）** | `script\uePlug\Doodle\Source\DoodleLightingEditor\`（`DoodleLightingEditor.Build.cs`；`Public\DoodleLighting{,Capture,Executor,Commandlet}.h` + 同名 `Private\*.cpp`） |
| 工作机 UE 命令行 | `src\doodle_lib\exe_warp\ai_light_render.cpp:53-58`（`-AllowCommandletRendering` 在这里） |
| 新单测 | `src\test\core\ai_light_dispatch.cpp` |
| 设计文档 | `docs\ue_ai_lighting_plan.md`（§7.x UE 契约、§15.12 服务端/工作机、§15.13 UE 侧、§15.14 批 6 本地上传） |

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
- 基线：全量 **240 用例 / 1944 断言全通过、exit 0**；`ai_light_*` exit 0；
  `ai_light_dispatch` **22 用例**（批 5 的 18 + 批 6 的 4 个 D41 用例）。
- 单测坑（本轮踩过，见 §15.12 坑 36–40）：`ai_light_candidate::metrics_` 是 JSON 列要用 `get_metrics()`；
  纯同步用例不能用 `DOODLE_FIXTURE_TEST_CASE_CORO`（没有 `co_await` → C4716），要用 `BOOST_FIXTURE_TEST_CASE`；
  `task_payload.h` 的类型都在 `doodle::ai_light`；`run_ue_assembly_arg` 在 `doodle::import_and_render_ue_ns`；
  `test_main` 里 `g_ctx()` 不可用，成功路径不要依赖它。
- **UE 侧代码在本机无法编译**：`E:\Doodle\script` 下只有打包好的插件副本（`uePlug\5.7\Plugins\Doodle_*`），
  没有 HostProject、也没有插件打包脚本 —— 那 20 个文件只做了对照 `E:\UnrealEngine\Engine\Source` 的 API 级核对。
- 本轮所有改动 **未 git commit**（由用户自己提交）。
