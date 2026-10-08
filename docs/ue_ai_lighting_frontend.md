# AI 灯光 · 前端页面需求与操作逻辑（交接文档）

> **这份文档是什么。** 服务端的 AI 灯光能力（方案 `docs/ue_ai_lighting_plan.md`，接缝见其 §15.4/§15.7/§15.8/§15.9）
> 已经落地：12 条 HTTP 路由、状态机、候选生成、文本反馈结构化全部可跑。**前端页面不由本方案实现方实现**，
> 本文把「前端要做什么、按什么顺序做、调哪个接口、拿到什么、怎么显示、错了怎么办」写清楚，交给前端实现方。
>
> **事实来源。** 本文的接口与字段全部来自服务端**已落地代码**（`src/doodle_lib/http_method/kitsu/ai_light.{h,cpp}`、
> `ai_light_orchestrator.{h,cpp}`、`src/doodle_core/metadata/ai_light/*.h`），不是设计意图的复述。
> 与代码冲突时**以代码为准**，并请回写本文。
>
> **文档边界。** 本文只描述前端职责与接口契约，不描述 LLM 提示词、UE 侧渲染、指标算法 ——
> 那些在方案文档里。凡本文写「服务端负责」的，前端一律**不要**自己实现。

---

## 1. 术语与业务对象

| 词 | 含义 | 前端关心什么 |
|---|---|---|
| **会话 session** | 一个镜头（shot/entity）的**一次** AI 灯光工作，从提示词走到定稿 | 页面主体；`state` 决定界面上能点什么 |
| **参考图 reference** | 美术上传的氛围参考（同一场景的不同角度，可能被PS/AI调过色），多张 | 上传、排序（`ordinal`）、人工排除、统计摘要 |
| **Spec** | 服务端把「提示词 + 参考图统计」蒸馏出的**灯光方案**：意图 + 数值目标区间 | **人工闸门 1**：美术必须确认它，才会开始出候选 |
| **轮次 iteration** | 一次「生成 6 个候选 → 渲染 → 交给美术判」的完整循环 | 时间线；`state` 表示这一轮走到哪 |
| **候选 candidate** | 一轮里的 6 个灯光设置之一，每个都会被真的渲染成一段视频（另配首帧海报） | 主工作台的核心：6 个候选并排（海报 + 播放器）+ 指标 + 判断 |
| **语义轴 axis** | 6 个候选之间**唯一允许变化的维度**（主光方向/主辅比/色温/软硬/环境光/调色强度） | 候选卡片上「这张和基线差在哪」的说明；也是美术的**反馈旋钮** |
| **人工闸门 1 / 2** | ①确认 Spec ②判断候选（挑选/继续/全否） | 两个必须由人点的按钮组 |
| **定稿 accept** | 美术挑中一个候选 → 该图注册成 Kitsu 的 preview file 挂到灯光任务上 | **不是终点**：会话随后回到 `generating`，可以继续调；再次定稿会把「当前定稿」指针移到新的一张 |
| **预设 preset** | 可复用的灯光配方（`rig_fragment` + 适用场景 + 目标区间） | 全局共享资产，有独立的库管理页 |

**角色**：美术（唯一操作者，做两次人工判断）；灯光指导/主管（复用 Kitsu 既有的任务状态、评论、审批流转，不由本特性新增）。
**外包角色（`outsource`）在所有 AI 灯光接口上都是 401** —— 权限不足不是一个可绕过的提示。

---

## 2. 六条硬约定（越界的实现要打回）

这六条是**架构性约束**，不是风格偏好。前五条来自方案 §14.4/§14.5，第六条来自实现事实。

1. **只有海报与成片下发浏览器（EXR 不下发）。** 每个候选的产物是**一段 mp4**（含相机运动的成片）加**一张首帧海报 PNG**；
   线性 EXR 是可选项（当前一轮不产），留服务端做工程检查。前端**只**拿海报 + 成片 + 数值指标。
   **候选对象里根本没有路径字段**（D44）：产物路径由服务端按
   「会话 uuid / 轮次 uuid / 候选 uuid」现算、不落库、不进 JSON —— 取海报一律用
   `GET /api/doodle/ai/light/candidate/{uuid}/image`，取成片用 `GET /api/doodle/ai/light/candidate/{uuid}/movie`，
   不要自己拼路径，也不要假设响应里有 `render_png_path` / `render_exr_path`。
2. **规划逻辑不要放到前端。** 现有 AIScript 模式是浏览器直连 DeepSeek —— 灯光规划**不能照抄**：
   提示词模板、Spec 词表、IR schema 校验一旦在 JS 与 C++ 各存一份，必然漂移。
   前端只调 `/api/doodle/ai/light/*`；**不要复用 `/api/doodle/deepseek/key`**，不要在前端拼提示词、不要在前端算轴。
3. **对比视图不做差分图。** 参考图是异视角、且被人工调过色，逐像素差分没有意义还会误导美术。
   用**并排 + 统计指标对照 + 目标区间标记**。
4. **`session_id` 只走路径。** 请求体里带 `session_id` 一律被忽略（避免「两个真相」）。
   路径里的 uuid 是**整串正则匹配**的：畸形 uuid 直接 404，不会 500。
5. **不要自己写死 6。** 每轮候选数用读模型给的 `candidate_count_per_iteration`（当前是 6）。
6. **不做逐 op 的接受/拒绝。** 判断粒度是**候选**（整张图），不是单个灯光操作。
   候选对象里的 `rig.ops` 是给执行器与服务端解释用的，**不是**让美术逐条勾选的清单。

---

## 3. 页面地图

方案 §14.6 定的 5 个新页面 + 1 个入口。路径是**前端路由**（浏览器地址），与服务端 API 路径无关。

| 前端路由 | 目的 | 对应闸门 | 新增/复用 |
|---|---|---|---|
| `/lighting/new` | 提交：选镜头 → 提示词 → 多张参考图 → 建会话并出 Spec | — | 新增（复用现有 UE 装配提交页的上传组件与交互） |
| `/lighting/spec/:specId` | **人工闸门 1**：看 Spec（意图 + 目标区间 + 冲突维度）并确认/重出 | 闸门 1 | 新增 |
| `/lighting/session/:sessionId` | **人工闸门 2 + 主工作台**：6 个候选并排 + 指标 + 判断 + 轮次时间线 | 闸门 2 | 新增，**核心页** |
| `/lighting/presets` | 预设库管理（列表/新建/局部编辑/删/启停） | — | 新增，**直接照抄 `/api/doodle/model_library/*` 的页面模式** |
| `/lighting/jobs` | 批量任务与农场进度 | — | 新增（复用 `/api/doodle/task*` 的既有页面能力） |
| 镜头页（Shot）增加「AI 灯光」按钮 | 入口，与现有 `run-ue-assembly` 并列 | — | 改既有页面：显示该镜头的会话列表 + 「新建」 |

**导航流**（美术的心智模型只有三个页面）：镜头页 →（`/lighting/new`）→ `/lighting/spec/:specId` → `/lighting/session/:sessionId`。
presets 与 jobs 是旁路工具页，不进主流程。

> **一个细节**：`/lighting/spec/:specId` 的 `specId` 只是**前端路由参数**。
> 服务端**没有**「按 Spec id 取 Spec」的接口 —— Spec 永远依附于会话。
> 所以该页面加载时应当拿 `sessionId` 调 `GET /api/doodle/ai/light/session/{id}`，再从
> `spec_history` / `current_spec` 里挑出目标那一版。若前端只有一个 id，用 Spec 的话需要带上会话 id。

---

## 4. 一次会话的完整时间线

### 4.1 状态机（两层，别混）

**会话层 `session.state`**（**4 个值**，JSON 字符串 = 枚举名；**没有终态**）：

```
draft ──POST /spec──▶ awaiting_spec_confirm ──POST /spec/confirm──▶ generating
  ▲                          │                                        │
  └──── 接缝失败回滚 ────────┘                                        │ worker 回填渲染
                                                                      ▼
                    ┌──── judge（三个决定都一样）────▶ awaiting_judgement
                    ▼
                generating   ◀── 可以再按「继续迭代」出下一轮（无限轮）
```

三个判断决定（`accept` 定稿 / `continue_iteration` 继续 / `reject_all` 全否）**都**回到
`generating` —— 定稿不是终点，全否也不是终止，只表示「这一轮全部不要」。所以界面上**没有**
「会话已结束」这种状态，也就**不需要**为它做只读分支。

**轮次层 `iteration.state`**：`generated` → `dispatched` → `rendering` → `awaiting_judgement` → `judged`。
（实现里 `dispatched`/`rendering` 目前**没有赋值点** —— worker 侧派发未接线，前端应把它们当「可能被跳过」。）

**关键事实**：
- **`advance_session` 是唯一的推进入口**，`/spec`、`/spec/confirm`、`/iterate`、镜头页入口都走它。
  「同一个状态只能有一个后继步骤」是结构性保证 —— 所以前端**不需要**猜「这一步该调哪个接口」：状态决定动作。
- **`generating → awaiting_judgement` 没有 HTTP 入口**，是 worker 回填渲染结果时由服务端自己完成的。
  前端在 `generating` 期间只能**轮询**（见 §8）。
- **4 小时超时不是状态**（详见 4.3）。

### 4.2 状态 → 界面动作对照表（前端把这张表直接当成按钮可见性规则）

| `session.state` | 界面该显示什么 | 允许的动作 | 动作后的状态 |
|---|---|---|---|
| `draft` | 提示词 + 参考图墙；提示「还没有灯光方案」 | 传/删参考图；**生成方案**（`POST /spec`） | `awaiting_spec_confirm` |
| `awaiting_spec_confirm` | **闸门 1 卡片**：意图 + 目标区间 + 冲突维度 | **确认方案**（`POST /spec/confirm`）；**重新生成**（`POST /spec`，revision+1） | `generating` / 仍是 `awaiting_spec_confirm` |
| `generating` | 进度状态：候选生成中 / 已派发渲染中；显示已渲染几张 | 轮询 `GET /session/{id}`；**重试生成**（`POST /iterate`） | 仍是 `generating` |
| `awaiting_judgement` | **闸门 2 工作台**：6 张候选 + 指标 + 反馈面板 | **判断**（`POST /judge`，唯一动作） | `generating`（三个决定都一样） |

辅助布尔（读模型直接给，**不要自己算状态机**）：`is_waiting_human`（= `awaiting_spec_confirm || awaiting_judgement`）、
`is_judgement_overdue`。

**没有终态**：会话永远停在上面 4 个状态之一，任何状态下写动作（传图、删图、出 Spec、判断）都只受
「当前状态支不支持这个动作」约束，**不存在**「会话已结束所以只读」的分支。历史里的
`accepted`/`rejected`/`failed` 三个字符串**从未进过生产库**（30 之后的数据库升级没有推送过，那六张表是
随该次升级新建的，所以出生即空），因此既不存在需要迁移的历史数据、也不需要前端做兼容
（见方案文档 §15.10）—— 如果老接口/老文档里还提到这三个值，一律忽略。

### 4.3 4 小时判断时限（不是状态，别做成分支）

- 进入 `awaiting_judgement` 时服务端写 `judgement_due_at = now + 4h`。
- 超时后**状态不变**（仍是 `awaiting_judgement`，仍然可以判断），读模型的 `is_judgement_overdue` 从 `false` 变 `true`。
- **没有后台扫描器、没有「挂起」状态**。前端拿到 `is_judgement_overdue == true` 时做两件事：
  在列表页把该会话高亮成「已超时待判」，在工作台顶部挂一条催办 banner（连同 `judgement_due_at` 剩余时间）。
- 判断成功后 `judgement_due_at` 会被清空（三条 decision 都会清）。
- **不要**因为超时禁用判断按钮 —— 那正是删掉 `suspended` 状态要避免的 bug（超时后候选再也判不了）。

---

## 5. 逐页需求与操作逻辑

### 5.1 镜头页（Shot）的「AI 灯光」入口

- 在既有镜头页加一个按钮，与 `run-ue-assembly` 并列。点击进入该镜头的 AI 灯光会话列表：
  `GET /api/doodle/ai/light/shot/{entity_id}/session` → `{data: [session...]}`（含历史，**新的在前**）。
- 列表每行显示：`title` / `state`（用 §4.2 的中文名）/ `current_iteration`（第几轮）/ `created_at` /
  待办高亮：`is_waiting_human`（轮到我）与超时（列表响应里**没有** `is_judgement_overdue`，
  它只在会话详情里；列表页要么进详情判断、要么用 `judgement_due_at` 自己比时间——推荐前者，避免两套判据）。
- 「新建」按钮 → `/lighting/new?entity_id=...`；`draft` 状态的会话行也应能点进去继续。
- 建会话要求该镜头**有灯光任务**（按 task_type 精确匹配），否则 400
  `镜头 {} 没有灯光任务，无法建立 AI 灯光会话` —— 前端要在提交前/报错后引导美术去建任务。

### 5.2 `/lighting/new` —— 提交页

**字段**：镜头（默认带入，可换）× 提示词（**必填非空**）× 参考图（**≥1 张**，多张）× 可选预设。

**操作顺序被服务端强制**，前端必须按这个顺序串起来（这是全流程最容易做错的地方）：

1. `POST /api/doodle/ai/light/shot/{entity_id}/session`，体 `{prompt_text, title?}` → **201** + 会话详情（`state = draft`，`references` 为空）。
   - 服务端覆盖 `submitter`（= 当前人）、`entity_id`（= 路径）、`project_id`（从实体取）、`task_id`（镜头灯光任务）、
     `state = draft`、`current_iteration = 0`。**体里只有 `prompt_text` 与 `title` 生效**，别传别的。
   - `title` 留空则取镜头名。
2. `POST /api/doodle/ai/light/session/{session_id}/reference`，**multipart/form-data**，一次可传多张
   → **201** + `{data: [新增的 reference...]}`。
   - 表单**字段名不参与解析**：凡是 file part 都会收（所以「一次传多张」天然支持）。
   - 服务端会统一转成 PNG、铸造 uuid、落盘、算统计（亮度分位/对比/色温/调色板/削波率/痕迹）。
   - **为什么不能把图塞进第 1 步**：参考图必须挂在已存在的会话上（产物目录按会话 uuid 组织），
     所以「先建会话、再传图」不是交互习惯问题，是硬顺序。
3. 可选第 2.5 步：如果想排除某张调色过度的图，在 `POST .../spec` 时用 `excluded_reference_ids` 排除（见 5.3）。
4. `POST /api/doodle/ai/light/session/{session_id}/spec`，体 `{}` → **201** + 会话详情（`state = awaiting_spec_confirm`，`current_spec` 有了）。
   - 这一步会真的调 LLM 蒸馏，**耗时数秒到数十秒**：按钮必须进 loading、禁用重复点击、允许失败重试。
   - **没有参考图必然 400**：`会话 {} 还没有参考图，无法蒸馏 Spec（决策 1：参考必有多张）`。
     这条错误**不写 `failure_reason`、不回滚状态** —— 前端不要把它当「AI 失败」渲染，它就是「你还没传图」。
5. 成功后跳到 `/lighting/spec/...`（闸门 1）。

**交互细节**：
- 参考图上传要显示：缩略图（用 `GET /api/doodle/ai/light/reference/{id}/image`）、尺寸、`stats_confidence`（置信度低要提示「这张图参考价值低」）、
  `is_outlier`/`excluded_reason`（人工排除过的）。
- 提示词是**权威意图**（方案 §6.3：提示词优先于参考图）。placeholder 应给出可用的中文示例，
  并说明「参考图提供场景的数值结构，提示词提供意图；两者冲突时以提示词为准」。
- 失败重试：`POST /spec` 失败后会话回到进入时的状态（`draft`），**已传的参考图保留**，可以直接重试。

### 5.3 `/lighting/spec/:specId` —— 人工闸门 1（Spec 确认与冲突裁决）

**显示什么**（数据来自会话详情的 `current_spec`；要对照 `spec_history` 看历史版本）：

| 区块 | 字段 | 怎么显示 |
|---|---|---|
| 意图 | `intent.atmosphere[]`、`time_of_day`、`weather`、`source_motivation[]`、`setting`、`story_context` | 标签组 + 文本 |
| 角色 | `intent.character_personality[]` → `{subject, trait, lighting_language}` | 表格：角色 / 性格 / 对应的灯光语言（`hardness`、`position_bias`、`kelvin_bias`、`key_fill_ratio`） |
| 目标区间 | `targets.luma_percentiles.{p10,p50,p90}`、`contrast`、`white_balance_kelvin`、`clip_rate.{high,low}`、`palette[].share` | **每个都是一对 `[low, high]`**，显示成区间条；`palette[].color` 用色块 |
| 调色提示 | `targets.grading_hints.{vignette, lifted_blacks, bloom}` | 枚举徽标（`none/subtle/moderate/strong`、是/否） |
| 责任划分 | `targets.ownership.{lighting, grading, shared}` | 三列维度名，说明「哪些指标归灯光、哪些归调色」（用于解释「为什么这张图的差异主要来自调色」） |
| 证据 | `evidence.{字段路径: {source, confidence, note?}}` | 每个目标的来源徽标：`prompt`/`reference_stats`/`preset`/`scene_facts`/`default_value` + 置信度条；**这是「这个数字凭什么」的唯一窗口** |
| 参考统计 | `reference_stats.{image_count, aggregation, per_dimension_confidence, conflicts[], retouch_detected, usable_for_physical_solve}` | 几张图、怎么聚合的、哪些维度冲突 |
| 降级 | `degrade_level` | `stats_only` = 正常；`prompt_only` = **必须显眼提示**「参考图不可用/完全冲突，本方案只依据提示词与预设库」，否则美术会误以为参考图起了作用 |
| 冲突 | `reference_stats.conflicts[]` | **要人工裁决**的维度列表（见下） |

**操作**：
- **确认方案** → `POST /api/doodle/ai/light/session/{session_id}/spec/confirm`，体可带冲突裁决：
  ```json
  { "conflict_resolution": { "resolved": { "<维度名>": <任意 JSON> }, "note": "为什么这么定" } }
  ```
  成功 → `state = generating`，服务端**随即触发首轮候选生成**（不用前端再调 `/iterate`）。
  该接口**幂等**：已确认过会直接返回 `{spec, session_state, already_confirmed: true}`，前端据此提示「已确认过」而不是报错。
- **重新生成** → `POST .../spec`，体可带 `{prompt_text, excluded_reference_ids}`：
  - `prompt_text` 会**覆盖**会话提示词并落库；`excluded_reference_ids` 把那些图标为人工排除
    （写 `is_outlier=true`、`excluded_reason="人工排除"`）。
  - 重出即 revision+1，**旧版留在 `spec_history` 里**（前端要提供「查看上一版」的入口，否则美术无法回头）。
  - 该接口**只在 `draft` 或 `awaiting_spec_confirm` 状态可用**。
- **冲突裁决的交互**：`reference_stats.conflicts` 里列出的维度，前端给一个「按提示词 / 按参考图 / 手工填一个区间」的三选一，
  结果写进 `conflict_resolution.resolved`。服务端**只记录不改统计**（统计是事实，裁决是意图）。
- 确认按钮是**人工闸门**：不要自动确认、不要因为「置信度高」就替美术点。

**注意**（服务端行为，前端要如实呈现）：
- 若 `current_spec.human_confirmed == true`，界面应显示确认人与确认时间（`confirmed_by`/`confirmed_at`）。
- `POST /spec/confirm` 若在候选生成阶段失败，会话回滚到 **`generating`**（Spec 的确认**保留**），
  读模型的 `failure_reason` 会有文本 —— 前端应显示它并给「重试生成」（`POST /iterate`）按钮。

### 5.4 `/lighting/session/:sessionId` —— 主工作台（人工闸门 2）★核心页

数据源：`GET /api/doodle/ai/light/session/{id}` **一个接口给全**（六表聚合，刻意不分页、不懒加载；
一次会话的量级是 6 × 轮数，实际收敛在 3 轮内）。

**布局三区**：

```
┌── 顶部：会话标题 / 状态 / 第几轮 / 超时 banner（is_judgement_overdue）────────────────┐
├── 左：参考图墙（references，按 ordinal）────┬── 中：本轮 6 个候选（接触表）──────────┤
│  缩略图 + 置信度 + 排除标记                  │  #0 基线  #1 轴…  #2 轴…  #3 轴… #4 #5  │
│  「对比视图」入口（并排，不做差分图）          │  每张：图 + 轴/档位标签 + 渲染状态      │
├───────────────────────────────────────────┴── 右/下：指标看板 + 判断面板 ──────────────┤
│  指标看板：区间满足率 + 每项落点（在区间内/偏高/偏低）                                  │
│  判断面板：定稿 / 继续迭代 / 全否 + 反馈（滑块默认 + 文本可选）                          │
│  轮次时间线：每轮的轴计划 strategy_、判断结果、渲染耗时                                  │
└──────────────────────────────────────────────────────────────────────────────────┘
```

**候选卡片（每张）显示**：
- 海报：`GET /api/doodle/ai/light/candidate/{id}/image`（**只发 PNG**；`pending` → 404「尚未渲染完成」，`failed` → 404「渲染失败」）。
- 成片：`GET /api/doodle/ai/light/candidate/{id}/movie`（`video/mp4`，**支持 Range** —— 用原生 `<video>` 播、让它自己分段拉，
  不要一次请求整段）。同一个候选的海报与成片**要么都在、要么都不在**：服务端只在两者都取得到时才算这个候选 `rendered`。
- `candidate_index`（0 = 基线）、`variant_axis` + `variant_value` + `variant_label`
  —— 这是「这 6 张差在哪」的唯一解释字段，**必须显示**（美术看不懂轴名就用中文映射，见 §6 表格）。
- `render_status`（`pending`/`rendered`/`failed`）、`render_ms`、`light_count`。
- `metrics.satisfaction`（0..1 的加权满足率）与 `metrics.landings[]` 的落点。
- `is_selected`（历史轮里被我选作基准的）、`is_accepted`（定稿的）、`preview_file_id`（只有定稿非 nil）。

**指标看板（决策辅助，不是判据；见 §7）**：
- 顶部一个大数：`metrics.satisfaction` × 100% = 区间满足率。
- 逐项 `metrics.landings[]`：`dimension`（如 `luma_percentiles.p50`）、`achieved`（实际值）、
  `target`（`[low, high]`）、`in_interval`、`distance`。
  显示成「实际值 ●———[区间]」，落点在区间内显示绿色、偏高/偏低用方向箭头 + `distance`。
- `metrics.palette` 与 `spec.targets.palette` 并排做色块对照（这是「调色对没对上」最直观的视图）。
- **一致性要求**：主指标都在**显示空间**（`metrics.space` 恒 `display_srgb`）算，与参考图同一空间 ——
  前端不要自己拿渲染图重算一遍指标。
- `vlm_score` / `vlm_delta.{verdict, differences}` 若有值，作为「语义判分」单独一栏展示（辅助意见）。

**判断面板（`POST /api/doodle/ai/light/session/{session_id}/judge`）**：

请求体字段（`decision` 缺省是 `continue_iteration`，**前端应当显式传**，不要依赖默认值）：

| 键 | 类型 | 何时必填 | 说明 |
|---|---|---|---|
| `decision` | `accept` / `continue_iteration` / `reject_all` | 建议总是显式传 | 注意字符串是 `continue_iteration`，**不是** `continue` |
| `selected_candidate_id` | uuid | `accept` 与 `continue_iteration` **必填** | `reject_all` 必须为空 |
| `feedback_axes` | `{"<轴名>": number}` | `continue_iteration` 可选 | **滑块通道（默认通道）**，服务端直接使用，不经 LLM |
| `feedback_text` | string | `continue_iteration` 可选 | 自由文本，服务端会尝试结构化；失败也**不会**让判断失败 |
| `by` / `at` | — | — | 服务端覆盖为当前人与当前时间，别传 |

三个动作：

1. **定稿（`accept`）**：必须先选一个候选。服务端会把它渲染成 Kitsu 的 preview file
   （`original_name = "ai_light_{会话 uuid}_cand{序号02}"`，挂在**灯光任务**上），同事务把候选标成 `is_accepted`。
   成功后会广播 `preview-file:new`。定稿**不再锁住会话**：会话回到 `generating`，可以接着调、
   再次定稿（会把「当前定稿」指针移到新的一张；旧的那张 `preview_file` 仍然留在灯光任务上，不会被删）。
2. **继续迭代（`continue_iteration`）**：必须选一个候选作为下一轮基准（它成为下一轮的 warm start），
   然后给**反馈**：滑块（默认）和/或文本。会话回到 `generating`，服务端据 `feedback_axes` 生成下一轮 6 个候选。
   - 反馈**可以为空**（「再调调看」是合法输入）；此时下一轮由模型自行选轴。前端应给出明显提示
     「没有指定方向时，下一轮由 AI 自己选方向」，避免美术误以为自己的意见被采纳了。
   - 服务端把滑块轴的数值连同文本推导的轴一起写进本轮 judgement 的 `feedback_axes`，下一轮生成时作为**约束**
     （「这些轴必须出现在你的轴计划里」）。
3. **全否（`reject_all`）**：不选候选，会话回到 `generating` —— 它表示「**这一轮全部不要**」，
   可以再出一轮（下一轮没有 warm start，从 LLM 的基线重新开始）。既然不是终止，它在语义上就
   不是不可逆操作；仍建议做二次确认（免得误点白扔一轮渲染）。

**轮次时间线**：`iterations[]` 按 `iteration` 升序，每轮显示：
- `axes.strategy`（服务端记录的本轮生成策略，含「上一轮反馈如何被采纳」——这是美术理解「AI 有没有听我的」的窗口）；
- `axes.axes[]`（轴 + 档位表 `steps` + `label`）；
- `judgement.{decision, feedback_text, feedback_axes, judged_by, judged_at}`；
- `render_ms_total`、`state`、`failure_reason`。
- 历史轮的候选图仍可取（`candidates[]` 是**跨轮**的）。

**`generating` 期间**：候选已生成、任务已派发，正在渲染。前端轮询会话详情，直到
`iterations[last].state` 进入 `awaiting_judgement`（或 `session.state` 变成 `awaiting_judgement`）。
逐张图可用 `render_status` 显示「已渲染 x/6」。渲染是分钟级的（见 §8）。

**已经定稿过的会话**：详情里 `accepted_candidate` 非 null 时显示定稿图（走 `preview_file` 的图片接口，
见 §9），并把「已挂到灯光任务的预览文件上」说清楚 —— 这是这个特性的**交付物**：定稿图进了 Kitsu 的
预览/评论/审批流程。但会话**不会**因此变成只读：美术可以接着调、再出下一轮、再次定稿。
界面上把「当前定稿」当成一个已交付的里程碑标记，而不是一个终态。

### 5.5 `/lighting/presets` —— 预设库管理

照抄 `/api/doodle/model_library/*` 的页面模式即可（列表 + 表单 + 启停 + 删除）。

- 列表：`GET /api/doodle/ai/light/preset`（默认只返回启用的）。
  **取全部要显式传 `?enabled_only=false`** —— 该参数的解析很特别：
  值 `true` 或 `1` → true；**其它任何值（含 `false`、`0`）→ false**；参数缺失 → 默认 true。
- 新建：`POST /api/doodle/ai/light/preset`，体 **至少要 `name` 非空**；
  其余可传 `tags`、`rig_fragment`（`{globals, lights}` 片段）、`applicable`（`scene_type[]`/`subject_count`/`interior`）、
  `stats_target`（无参考图时的兜底目标区间）、`enabled`。
  **`source` 与 `created_by` 由服务端钉死**（= `artist` / 当前人），传了也无效。
- 编辑：`PUT /api/doodle/ai/light/preset/{id}` 是**局部更新** —— 只有体里出现的键会被覆盖，
  白名单是 `name|tags|embedding|embedding_dim|rig_fragment|applicable|stats_target|enabled`；
  `source`/`created_by` **不可改**（那是「谁贡献的、是不是内置」的事实）。
- 删除：`DELETE /api/doodle/ai/light/preset/{id}` 是**真删**（预设无外键引用，因为候选存的是 rig 全量副本）。
  给二次确认。
- 表单里的 `rig_fragment` 建议给「从某个候选复制」的入口（美术不会手写 JSON）。

### 5.6 `/lighting/jobs` —— 批量任务与农场进度

复用既有的 `/api/doodle/task`、`/api/doodle/task/{id}`、`/api/doodle/task/{id}/log`、`/api/doodle/task/{id}/inspect`。
AI 灯光派发的任务类型是 `ai_light`（`server_task_info.type`），任务载荷里带
`{session_id, iteration, spec_id, candidates[], baseline_level}`。这一页的目标是「这一批渲到哪了、哪台机在跑、失败的原因是什么」。

### 5.7 复用的既有能力（不要重写）

| 能力 | 复用点 |
|---|---|
| 任务提交页与多图上传组件 | 现有 `run-ue-assembly` 提交页（`/api/actions/projects/{project_id}/shots/{id}/run-ue-assembly`） |
| 参考素材浏览 | `/api/data/entities/{entity_id}/preview-files`、`/api/doodle/pictures/*` |
| 定稿图的浏览/评论/审批 | Kitsu 既有 preview file + comment + task status（`/api/actions/projects/{}/tasks/comment-many`） |
| 农场任务进度 | `/api/doodle/task*`、`/api/doodle/computing_time/*` |
| 通知 | Kitsu 既有通知机制（本特性**没有**新增通知类型） |
| 模型库页面模式 | `/api/doodle/model_library/*`（预设库照抄它） |

---

## 6. 反馈通道规格：滑块 ↔ 六条轴（**最关键的一节**）

美术的反馈有两种通道，**滑块是默认通道**，自然语言是补充。两者最终汇合成同一个
`feedback_axes: {"<轴名>": <数值>}`。

**六条轴的语义**（这张表与服务端 `apply_axis_step()`、提示词里的轴说明是**同一份口径**；
两边不一致的症状是「轴变了但图没变」）：

| JSON 键（轴名） | 语义 | 单位/范围 | 建议控件 | 中文标签 |
|---|---|---|---|---|
| `key_direction` | 主光**方位角增量** | 度，方向为增/减 | Slider −60 … +60，step 5，0 = 不变 | 「主光方向（左右）」 |
| `key_fill_ratio` | 主辅比（key:fill）**的目标绝对值**（**不是增量**） | 比值 > 0，常见 3 / 6 / 10 | 档位按钮 3:1 / 6:1 / 10:1，或 Slider 2…12 | 「明暗对比（主辅比）」 |
| `kelvin` | 色温**增量** | K | Slider −800 … +800，step 100 | 「冷暖」 |
| `hardness` | 硬软**增量**（正 = 更硬） | −1 … +1 | Slider −1 … +1，step 0.1 | 「阴影软硬」 |
| `ambient_level` | 环境光**相对增减** | −1 … +1 | Slider −1 … +1，step 0.1 | 「环境光/暗部」 |
| `grading_strength` | 调色强度**增量**（只放大/缩小「偏离中性的量」） | −1 … +1 | Slider −1 … +1，step 0.1 | 「调色强度」 |

**必须守住的规则**（服务端会按这些规则处理，前端提前约束能避免「点了没反应」）：

1. **键必须是上面的枚举名**。没有 `key_elevation`、没有 `_delta` 后缀。
   方案 §4.4 早期样例里的 `key_elevation_delta` / `kelvin_delta` 是**错的**，已按实现修正。
2. **只有 `key_fill_ratio` 是绝对值，其余五条都是增量。** 这是最容易做错的一条：
   滑块旁边必须写清「目标 6:1」而不是「+6」。
3. **值不能是 0**：`0` 表示「没有变化」，会被服务端丢弃（等于这条轴没给）。
   所以滑块回到中点时前端应当**不提交该轴**，而不是提交 0。
4. **一次最多 2 条轴**（`g_axis_max_count = 2`）。这是硬约束：下一轮候选生成只接受 1–2 条轴的计划，
   文本通道给 3 条会让整份轴计划被丢弃、文本通道静默失效。
   前端建议：**同时最多允许启用 2 个滑块**，第 3 个要么禁用要么替换。
5. **滑块优先**：同一轴上既有滑块值又有文本时，**滑块胜**（服务端合并时跳过文本推导的同名轴）。
   前端应把这条规则显示出来，例如「冷暖已由滑块指定（+400K），文字里再写『冷一点』不会生效」。
6. **未知轴名会被丢弃**（不报错）。所以前端不要发明轴名。
7. **当前轮各候选沿哪条轴变了多少**是只读信息，来自候选的 `variant_axis`/`variant_value`/`variant_label`；
   `candidate_index == 0` 是基线（`variant_axis == "none"`）。

**提交形状**（`/judge` 的 `continue_iteration`）：

```json
{
  "decision": "continue_iteration",
  "selected_candidate_id": "…",
  "feedback_axes": { "key_fill_ratio": 10, "kelvin": 400 },
  "feedback_text": "主光再低一点，色温再冷些"
}
```

**文本通道的行为**（前端要知道，才能正确提示）：
- 只在 `decision == continue_iteration` 且 `feedback_text` 非空时，服务端才调一次 LLM 把文本翻成轴；
- **任何失败都降级**：文本原样留存在 judgement 里，判断照常成功 —— 所以前端**不要**因为「AI 没听懂」而报错；
- 服务端只把**语义有效**的轴留下来（丢弃 `none`、0、未知轴名、非正的主辅比、重复轴）。

---

## 7. 指标看板的呈现规格

**判据层级（务必在 UI 上传达）**：人工判断是**唯一收敛判据**；目标区间满足率（A）与 VLM 语义判分（B）都只是**决策辅助**，
**没有自动接受**。前端不要把「满足率 100%」渲染成「推荐接受」。

**呈现方式**：**区间满足率 + 每项落点**。

- 满足率：`metrics.satisfaction`（按 `targets.ownership` 与各维度置信度加权）。
- 每项落点：`metrics.landings[]`，`dimension` 是维度名（如 `luma_percentiles.p50`、`contrast`、
  `white_balance_kelvin`、`clip_rate.high`、`palette`），
  `achieved` 是实际值，`target` 是 `[low, high]`，`in_interval` 是布尔，`distance` 是超出区间的距离（0 = 在区间内）。
- 视觉：区间条 + 实际值游标；在区间内 = 中性色（不要用「成功绿」暗示可以自动接受），偏离用方向 + 距离。
- `dimension_distances`（`{维度名: 距离}`）用于排序「差得最多的三项」——**这正是下一轮该调的轴**，
  建议直接在指标看板里给出「差得最多的维度」提示。
- **空间一致性**：`metrics.space` 恒为 `display_srgb`（显示空间）。参考图的 `stats.space` 也是同一空间。
  唯一例外是色温：服务端已按「sRGB 线性化 → XYZ → CCT」算好，前端**不要**自己从 RGB 反推。
- 参考图对照：把参考图的 `stats`（亮度分位/对比/色温/调色板/削波率）与候选的 `metrics` 并排 ——
  这就是「参考 vs 候选」的定量对照（**不做差分图**）。

---

## 8. 实时通知与轮询

**服务端只广播两个事件**（namespace `/events`），**没有**会话级/轮次级/候选级的专用事件：

| 事件名 | 何时 | 前端该做什么 |
|---|---|---|
| `preview-file:new`（字段 `preview_file_id`、`comment_id`、`project_id`） | `/judge` 定稿成功、preview file 落库后 | 刷新该项目的预览文件列表；会话页显示「已定稿」 |
| `server-task-info:new`（字段 `server_task_info_id`） | 候选生成成功、渲染任务派发后 | 任务/进度页插入新任务 |

**其余一律靠轮询** `GET /api/doodle/ai/light/session/{id}`：

- `generating` 期间：轮询直到 `awaiting_judgement`（渲染是**分钟级**）。建议起始 5 s、指数退避到 30 s，页面失焦时更慢。
- 用 `render_status` 显示逐张进度（`pending` → `rendered`/`failed`）。
- `awaiting_judgement` 期间不需要高频轮询（等人，分钟到小时级）：可退到 60 s，或只在窗口聚焦时拉一次。
- **不要**对图片接口做轮询：候选图在 `pending` 时是 404，等 `rendered` 后再取即可（`<img>` 加载失败不要弹错误提示，静默重试）。

---

## 9. 图片与缓存

| 用途 | 接口 | 说明 |
|---|---|---|
| 参考图 | `GET /api/doodle/ai/light/reference/{id}/image` | 上传时已转 PNG；不要用 `file_path`（服务端磁盘路径） |
| 中间候选海报 | `GET /api/doodle/ai/light/candidate/{id}/image` | `pending`/`failed` → 404（含中文原因）；磁盘缺文件 → 500 |
| 中间候选成片 | `GET /api/doodle/ai/light/candidate/{id}/movie` | `video/mp4`，带 Range（原生播放器可直接拖进度）；缺失同样 404/500 |
| 定稿图 | 走 preview file 的既有图片接口（按 `preview_file_id`） | 定稿候选的 `preview_file_id` 非 nil；也可从 `/api/data/entities/{entity_id}/preview-files` 拿到 |

- **签名 URL**：取图的两条路由同时接受「会话 JWT」**或**「短时效签名 URL」
  （`?exp=<unix秒>&uid=<人员uuid>&sig=<hex(hmac-sha256)>`，默认有效期 **10 分钟**，**只对 GET 生效**）。
  **站点内正常带 cookie 请求即可，前端一般不需要签名** —— 签名主要是给外部 VLM 取图用的，
  且签名路径下项目权限校验**照常生效**（换的是凭据，不是绕过授权）。
- 服务端这两个取图接口**没有设置** `Cache-Control`/`ETag`。前端对同一张已渲染的候选图可以自己加缓存
  （候选图内容不会变：`candidate.id` 不变则图不变）。
- 删会话时产物目录**先备份再删**（进 `…/ai_light/backup/<日期>/<会话uuid>/`），但记录随 cascade 删除、**不再有任何 HTTP 入口**。
  所以「删了还能从备份捞图」是运维行为，不是前端能力。

---

## 10. 错误处理与降级清单

**通用事实（先记住四条，能省掉一半的调试时间）**：

1. **所有 POST 的成功状态码是 `201 Created`**（不是 200）；GET/PUT 是 200；DELETE 是 `204 No Content`。
   前端别把 201 当异常。
2. **权限失败是 `401 Unauthorized`（不是 403）**，消息 `权限不足`。
   **外包角色（`outsource`）在所有 AI 灯光接口上都是 401**；项目无权访问也是 401。
3. 请求体里的 `session_id` 一律忽略；路径里的 uuid 畸形 → **404**（不会 500）。
4. **仓库里已经没有 501 端点**。三条 LLM 接缝（`/spec`、`/iterate`、`/judge` 的文本通道）全部落地，
   失败时给的是 **400（内容安全/参数）** 或 **502（推理失败）**，不是 501。

**常见错误 → 前端动作**：

| 状态 | 消息（节选） | 含义 | 前端动作 |
|---|---|---|---|
| 400 | `缺少 prompt_text —— 提示词是权威意图，不允许空着建会话` | 建会话没给提示词 | 表单校验，聚焦提示词 |
| 400 | `镜头 {} 没有灯光任务，无法建立 AI 灯光会话` | 该镜头没有灯光任务 | 引导去建任务 |
| 400 | `会话 {} 还没有参考图，无法蒸馏 Spec（决策 1：参考必有多张）` | 顺序错 | 回到上传步骤；**不要**显示成 AI 失败（不写 `failure_reason`） |
| 400 | `会话 {} 处于状态 {}，当前没有可推进的步骤` | 状态不对 | 刷新详情，按 §4.2 重新渲染按钮 |
| 400 | `定稿必须指定候选` / `候选 {} 不属于第 {} 轮` | 参数与服务端不一致 | 强制「先选候选再点定稿」 |
| 400 | `轮次 {} 已经判断过了` | 重复提交 | 幂等处理：刷新详情，不报错 |
| 400 | `参考图 {} 无法解析为图片` / `落盘后无法解码，无法统计` | 上传的文件不是有效图片 | 单张失败提示，不中断其余 |
| 400 | `{} 被内容安全策略拒绝：{}` | LLM 拒答 | 提示改提示词/换参考图后重试 |
| 502 | `{} 推理失败（status={}, error_code={}, 超时={}）：{}` | 上游推理失败 | 可重试；带 `error_code` 便于反馈 |
| 502 | `{} 的模型输出不是合法 JSON 对象` | 模型输出不合 schema | 可重试（偶发） |
| 500 | `候选渲染图不存在 文件 {}` / `候选视频不存在 文件 {}` | 服务端产物缺失（海报或成片） | 提示联系维护，不要重试 |
| 404 | 候选 `尚未渲染完成` / `渲染失败` | 图还没好/坏了 | 用 `render_status` 决定显示占位图还是失败标记 |
| 404 | `AI 灯光会话 {} 不存在` | 会话被删/不存在 | 回列表页 |

**降级路径（前端要能走下去，不能卡死）**：
- 会话 `failure_reason` 非空时，在工作台顶部显示它 + 「重试生成」（`POST /iterate`，要求状态是 `generating`）。
- 文本反馈结构化失败时，服务端已降级为「只用滑块轴」；前端只需照常显示判断结果，不需要额外提示（最多在日志级提示）。
- 指标缺失（`metrics` 为 null，M2 之前可能发生）时，指标看板显示「本轮暂无指标」，**不要**用 0 冒充。

---

## 11. 验收标准（可勾选）

**流程闭环**
- [ ] 从镜头页能建会话、传多张参考图、出 Spec、确认 Spec、看到一轮 6 个候选渲染完成（每个候选有首帧海报 + 可播放的成片）、判断、看到下一轮，最终定稿。
- [ ] 定稿后能在 Kitsu 既有的预览文件流程里看到那张图（挂在灯光任务上），并能评论/审批。
- [ ] 全否后会话仍可继续出一轮（不是终止，界面不变只读）。
- [ ] 定稿后会话仍可继续调、可再次定稿；再次定稿后任务上能看到新的预览文件，旧的仍在。

**闸门**
- [ ] 闸门 1 未确认前无法进入候选生成（按钮不存在或服务端 400 有明确提示）。
- [ ] 闸门 2 只能三选一，且 `accept`/`continue_iteration` 必须先选候选；`reject_all` 有二次确认。
- [ ] 判断重复提交不会报错（幂等提示）。

**反馈通道**
- [ ] 6 条轴的中文标签、单位、方向与 §6 表格一致；`key_fill_ratio` 显示为**目标比值**而非增量。
- [ ] 值为 0 的轴不会被提交；同时启用超过 2 个滑块时给出明确限制提示。
- [ ] 同一轴既有滑块又有文本时，界面提示「以滑块为准」。
- [ ] 纯文本反馈（无滑块）能走通，且 AI 没听懂时判断依然成功。

**呈现**
- [ ] 候选卡片显示 `candidate_index`（0 = 基线）+ 轴/档位标签；6 张图的**灯集合与角色一致**（只有轴参数不同）。
- [ ] 指标看板显示区间满足率 + 每项落点（在区间内/偏高/偏低），并标出「差得最多的维度」。
- [ ] 参考 vs 候选是**并排 + 统计对照**，没有差分图。
- [ ] 页面上**没有**任何 EXR 入口；候选图/参考图都走 `/image` 接口，不出现磁盘路径。

**健壮性**
- [ ] `generating` 期间轮询能自动推进到 `awaiting_judgement`；渲染失败的单张图有明确标记。
- [ ] 4 小时超时会话有催办提示，且**仍然可以判断**。
- [ ] 401 的展示是「权限不足」而不是「服务器错误」；外包账号看不到 AI 灯光操作（或操作后明确报 401）。
- [ ] 所有 POST 按 201 处理，DELETE 按 204 处理。

---

## 12. 明确不属于前端

1. **实时视口反馈** —— 已取消（UE 侧不做实时交互设置）。
2. **逐 op 的接受/拒绝** —— 判断粒度是**候选**。
3. **SceneFacts 采集** —— 由 UE commandlet 的 `inventory` 做。
4. **提示词模板、轴计算、Spec 词表、IR schema 校验** —— 全在服务端（§2 第 2 条）。
5. **指标计算** —— 服务端（显示空间）；前端只呈现。
6. **LLM 直连** —— 前端不持有任何模型 key。

---

## 13. 待前端/产品确认的问题

1. **宿主工程**。本机事实：生产前端是 Kitsu 的 fork，部署在 `D:\kitsu\dist`（**只有构建产物，没有源码**）；
   `E:\doodle_vue\doodle` 是一个近乎空白的 Vuetify 3 脚手架（Vue3 + Vuetify + Pinia + vue-router + TS + Vite，
   目前只有 `views/AboutView.vue`、`views/HomeView.vue`，全工程**没有任何 `/api/` 调用**）。
   这 5 个页面落在哪个工程、能否复用 Kitsu 的 preview file / 评论 / 任务状态组件，需要前端实现方先明确。
   本文按**需求与契约**写，与宿主无关。
2. **认证来源**：站点内用现有会话 cookie 即可；若前端要独立部署（跨域），需要确认是否改用签名 URL。
3. **滑块的档位与默认值**：本文给的是**建议范围**，最终以美术试用为准（方案 §6.2 给的是档位示例 3:1/6:1/10:1 等）。
4. **批量派发的范围**：`/lighting/jobs` 是「看进度」还是「批量建会话并派发」，需要产品定义（当前服务端**没有**批量建会话的接口）。
5. **Spec 页的路由参数**：服务端没有「按 Spec id 取 Spec」的接口，见 §3 末尾的说明。

---

## 附：本文与方案文档的对应

| 本文 | 方案文档 |
|---|---|
| §2 硬约定 | §14.4 / §14.5 |
| §4 时间线、状态机 | §10.3 / §15.4 |
| §5.4 主工作台、§7 指标看板 | §8.1 / §8.2 / §8.3 |
| §6 六条轴与滑块规格 | §6.2 / §15.8 / §15.9 |
| §3 页面地图 | §14.3 / §14.6 |
| §8/§9 通知与图片 | §15.4（图片变体导出）/ §15.6 |
| 接口契约细节 | 服务端代码 `ai_light.{h,cpp}` / `ai_light_orchestrator.{h,cpp}` |
