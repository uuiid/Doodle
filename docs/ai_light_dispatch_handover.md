# AI 灯光「派发功能」交接文档

> 需求原话（用户）：**「安排 ai_light 工作的派发功能」**。
> 目标：把一轮 6 个候选真正派到工作机 → UE commandlet 渲染 → 回传落库，让会话从
> `generating` 走到 `awaiting_judgement`（人工判断闸门 2 之前的那一段）。
>
> **当前状态：方案已获批准；批 0（载荷契约）+ 批 1（回传落库）已落盘并编译通过；
> 单测与批 2–5 未做。**

## 一、已完成的工作

### 1. 派发/回传的 wire 契约（新文件，JSON 键已冻结）
`E:\Doodle\src\doodle_core\metadata\ai_light\task_payload.h`

- `g_ai_light_render_report_file_name{"render_report.json"}` —— commandlet 写、工作机读。
- `ai_light_task_payload_t`（派发，进 `server_task_info.command_`）：键
  `session_id / iteration / spec_id / out_dir / baseline_level / ue_project_path / camera_file_path / candidates`。
- `ai_light_task_candidate_t`：键 `candidate_index / candidate_id / png_path / exr_path / rig`（rig 已含 `ops_`）。
- `round_render_report_t`（回传）：键 `candidates / error`。
- `candidate_render_report_t`：键 `candidate_index / candidate_id / status / render_ms / light_count / metrics`
  —— **不带路径**（落库时服务端按 uuid 重算）。
- 已登记：`src/doodle_core/metadata/ai_light/CMakeLists.txt`、伞形头 `.../ai_light/ai_light.h`。

### 2. 服务端回传处理（编排器）
`E:\Doodle\src\doodle_lib\http_method\kitsu\ai_light_orchestrator.cpp`（924 行）

| 函数 | 位置 | 职责 |
| --- | --- | --- |
| `parse_round_render_report` | `:729` | 解析回传 JSON。**严格字段**（`candidates` / 每个元素的 `candidate_id` / `status`）缺失或值不认识 → 抛 502；**宽松字段**（`candidate_index` / `render_ms` / `light_count` / `metrics`）缺失取默认、形状不符丢弃 + WARN |
| `mark_iteration_failed` | `:797` | 整轮失败收尾：**只**写 `session.failure_reason_`（截断 512），状态机状态与候选行一概不动 |
| `apply_render_report` | `:821` | 至少一张 `rendered` → 服务端重算产物路径（不信任回传）→ 复用既有 `mark_iteration_rendered`（`:651`）落库并把轮次/会话推到 `awaiting_judgement` |

声明在 `ai_light_orchestrator.h:142 / :155 / :161`；新增截断助手 `brief_text`（`.cpp:83`，`brief_error` 改为调它）。

### 3. 前置重构（上一个任务，已完成）
三条推理接缝、rig 纯运算、共用部件、JSON 校验层各自成文件（`ai_light_seam_{spec,candidates,feedback,common}.*`、
`ai_light_rig_ops.*`、`ai_light_json_validation.*`），编排器 2734 → 924 行。文档见
`E:\Doodle\docs\ue_ai_lighting_plan.md` §15.11。

## 二、已定的设计要点

1. **只走一条载荷通道**：`server_task_info.command_` 装 payload；工作机**不查库**、不连数据库。
2. **路径只在服务端产生**：payload 里给 `out_dir` / `png_path` / `exr_path`；回传只带数值与状态，
   落库时用 `kitsu_ctx_t::get_ai_light_candidate_file()` 按 uuid 重算。
3. **失败不是终态**：整轮失败只写 `failure_reason_`，会话不换状态，美术可再点 `/iterate` 重派。
4. **回传文件**：commandlet 写 `<out_dir>/render_report.json`，工作机读它并 HTTP 回传给服务端。
5. `candidate_render_status` 只有 `pending` / `rendered` / `failed`（**没有** succeeded）。

## 三、下一步（按批）

- **批 2 · 服务端派发**
  - `advance_session_generating_branch` 组装 `ai_light_task_payload_t` 塞进 `command_`（`ai_light_orchestrator.cpp:203`）。
  - 补齐载荷里两个当前拿不到的值：`baseline_level_`（M2 阻塞项）与 `ue_project_path_`、`camera_file_path_`
    —— 复用 `auto_task::shot_render_light(project_id, task_id)`（`auto_task.cpp:768-812`），
    `session.project_id_` + `session.task_id_` 就是它的两个入参。
  - 新增工作机回传路由：注册位在 `src/doodle_lib/http_method/kitsu.cpp:296-335`，插在
    `judge_route` 之后、`preset_route` 之前；handler 里先 `person_` 鉴权（或按 task 的提交人 cookie 校验），
    再 `parse_round_render_report` → `apply_render_report`。
- **批 3 · 工作机**
  - `http_client/work.cpp:216-237` 的 `run_task()` 加 `server_task_info_type::ai_light` 分支（漏了会走 `report_task_unsupported` → failed）。
  - 新任务类（复用 `exe_warp/import_and_render_ue.cpp` 的 `create_logger()` / `create_kitsu_client()` /
    `async_run_ue()`；建议抽公共基类，但**不能**直接复用 `run_ue_assembly_base::run()` 的流程）。
  - **不要**把 `ai_light` 加进 `http_method/local/event.cpp:32-34` 的默认白名单（不是每台机器都有 UE/GPU）。
- **批 4 · UE**
  - `DoodleLighting` commandlet 模块：复制基线关卡 → 逐候选 `apply_ops` + 渲染 PNG/EXR → 写 `render_report.json`
    → 清理回基线；源资产永不修改；单个候选失败不拖垮整轮。
- **批 5 · 测试与文档**
  - 新套件 `src/test/core/ai_light_dispatch.cpp`（登记进 `src/test/core/CMakeLists.txt`）：冻结 JSON 键、
    严格/宽松分界、`apply_render_report` 两条出口、候选不属于本轮 → 400。
  - `docs/ue_ai_lighting_plan.md` 追加 §15.12；全量回归。

## 四、关键文件路径

| 作用 | 路径 |
| --- | --- |
| 载荷 wire 契约（新） | `src\doodle_core\metadata\ai_light\task_payload.h` |
| 编排器（状态机 + 回传处理） | `src\doodle_lib\http_method\kitsu\ai_light_orchestrator.{h,cpp}` |
| JSON 校验层（严格/宽松读取器） | `src\doodle_lib\http_method\kitsu\ai_light_json_validation.{h,cpp}` |
| 三条接缝 | `src\doodle_lib\http_method\kitsu\ai_light_seam_{spec,candidates,feedback}.{h,cpp}` |
| rig 纯运算 | `src\doodle_lib\http_method\kitsu\ai_light_rig_ops.{h,cpp}` |
| 共用部件 / 推理管道 | `src\doodle_lib\http_method\kitsu\ai_light_seam_common.{h,cpp}` |
| HTTP 路由与注册 | `src\doodle_lib\http_method\kitsu\ai_light.{h,cpp}`、`...\kitsu.cpp:296-335` |
| 工作机派发入口 | `src\doodle_lib\http_client\work.{h,cpp}`（`run_task` 在 `work.cpp:216-237`） |
| UE 启动设施 | `src\doodle_lib\exe_warp\import_and_render_ue.cpp:194-296`、`ue_exe.{h,cpp}` |
| 基线关卡来源 | `src\doodle_lib\http_method\kitsu\auto_task.cpp:768-812` |
| 候选/轮次/会话表实体 | `src\doodle_core\metadata\ai_light\{candidate,iteration,session,rig}.h` |
| 设计文档 | `docs\ue_ai_lighting_plan.md`（§15.11 重构、§15.12 待写） |
| 重构契约（历史） | `build\refactor_plan_v3.md` |

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
- 基线：批 0/1 落地前全量 **218 用例 / 1781 断言全通过**；`ai_light_*` **101 用例 / 1028 断言**。
  批 0/1 之后只验证过编译（BUILD OK），新用例尚未编写。
- 本轮所有改动 **未 git commit**（由用户自己提交）。
