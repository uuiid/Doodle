# AI 灯光：后端角色降级方案设计

> 状态：**方案设计 v1**（前置前提已由用户锁定，见 §1）
> 上游文档：[ue_ai_lighting_plan.md](ue_ai_lighting_plan.md)（v3.4，现状实现）、[ue_ai_lighting_frontend.md](ue_ai_lighting_frontend.md)（前端交接规格）
> 冲突处理：本文与上游两份文档冲突时**以本文为准**；上游文档描述的是降级前的形态，不因本文而失效（降级未落地前它们仍是权威）
> 起因：m00001 / m00602 —— 「后端角色发生改变，只负责发送和接收大模型的请求，前端通过大模型的回复来决定操作灯光的数据」
> 关键前提（m00602 原文）：**「现在的方案是没有到生产环境中的，不要考虑兼容性」**

---

## 1. 前提与动机

### 1.1 前提（用户已锁定）

| # | 前提 | 影响 |
|---|---|---|
| P1 | 后端角色降级 = **无状态 LLM 网关**，不是"加一个小接口" | 三条接缝的业务解析整体迁出服务端 |
| P2 | 前端通过大模型回复**决定操作灯光的数据** | `lighting_rig_t` 从"服务端生成"变为"服务端接收" |
| P3 | 后端保留 **openCV 参考图分析** | 统计层（`ai_light_stats.*`）原地不动 |
| P4 | **不考虑兼容性**，现有方案未进生产 | 允许删除接缝、改表结构、重写状态机，不留迁移分支 |

### 1.2 动机

现状的瓶颈不是正确性，而是**迭代速度**：提示词模板、JSON Schema、轴计划、候选展开全部编在 C++ 里（约 800 行），每改一次提示词都要走一次 `Ninja_debug` 构建 + 重启服务。而这一层恰恰是最需要高频试验的部分。

把"解释模型输出"移到前端后，调提示词变成改一个 TS 文件，这是本方案的全部收益来源。

### 1.3 一条不可让渡的边界

**凭据与计费必须留在服务端。** 依据：

- `src/doodle_core/metadata/ai_studio.h:14` —— `ai_studio.seedance2_key_` 是模型凭据的持有处
- `src/doodle_core/metadata/kitsu_ctx_t.h:22` —— `kitsu_ctx_t.deepseek_keys_` 是第二处
- `src/doodle_lib/http_method/kitsu/ai_light.h:8-10` —— 已写死的设计：「LLM/VLM 调用只在主服务器上做（只有它持有 `kitsu_ctx_t.deepseek_keys_`）」
- `src/doodle_lib/http_method/other/other.cpp:21-24` —— `/api/doodle/deepseek/key` 确实在无鉴权地返回 key 列表，但它是历史遗留口子，`docs/ue_ai_lighting_frontend.md:39-54` 已明令前端不得复用

因此本方案中**前端不持有任何模型 key**，也不直连 Ark。前端提交**提示词与参数**，后端代理转发，返回**原始模型回复**。

---

## 2. 现状盘点：后端到底做了什么

「把解析搬到前端」这句话需要先精确化。后端在拿到模型 JSON 之后做的事**不是"理解模型说了什么"，而是"把自由输出钉死成受限 IR"**。

### 2.1 三条接缝的职责拆解

| 接缝 | 入口 | 承接纯计算 | **契约强制**（本方案要迁出的部分） |
|---|---|---|---|
| 1 Spec | `POST /session/{sid}/spec` | 参考图聚合（openCV，**保留**） | 钉死 `space/palette/ownership/schema_version/reference_stats`/身份字段；LLM 只能给 `intent`/`targets` 标量区间/`grading_hints`/`conflicts`（`ai_light_seam_spec.h:95-98`） |
| 2 Candidates | `POST /session/{sid}/iterate` | 无（输入只有会话上下文） | `parse_axis_plan` 档位硬校验；`map_candidate_result` 的 `role=key` 判据、substance 差异判据、`rigs_.size() == g_candidate_count` 护栏；`expand_candidates`+`diff_rigs` 确定性展开 |
| 3 Feedback | `POST /session/{sid}/judge` | 无 | 轴名白名单、值非 0、最多 2 条；滑块优先 |

### 2.2 关键代码位置（搬迁清单的坐标）

**接缝 1（`ai_light_seam_spec.cpp` / `.h`）**

- `build_spec_instructions()` / `build_spec_json_schema()` / `build_spec_input_text()` / `build_spec_request()` / `map_spec_result()` —— `ai_light_seam_spec.h:79-98` 声明
- `g_spec_schema_version` 常量、`l_spec.degrade_level_` 的两级降级判定（`ai_light_seam_spec.cpp:346`）
- 冲突**只**由服务端聚合器判定（`ai_light_seam_spec.cpp:425-428`，决定 D7）

**接缝 2（`ai_light_seam_candidates.cpp`，572 行）**

- `preset_lexical_score` / `pick_presets`（`:104-135`）
- `parse_axis_plan`（`:143-194`）
- `build_candidates_instructions`（`:198-231`）
- `build_candidates_json_schema`（`:233-284`）—— 逐层 `additionalProperties:false` + 全字段必填
- `build_candidates_input_text` / `build_candidates_request`（`:286-342`）
- `default_axis_plan`（`:344-366`）—— 轴计划非法时的回退
- `build_base_rig`（`:368-452`）—— `light_t` 逐字段构造
- `map_candidate_result`（`:454-534`）—— 三条 502 判据 + provenance 钉死
- `generate_candidates`（`:536-570`）

**接缝 2 的展开算子（`ai_light_rig_ops.h` / 对应 `.cpp`）**

- `g_axis_step_budget = g_candidate_count - 1`（`ai_light_rig_ops.h:32`）、`g_axis_max_count = 2`（`:36`）
- `key_fill_ratio_of` / `apply_axis_step` / `expand_candidates` / `diff_rigs`（`:44-58`）
- 注释强调：**UE 执行器必须按同一份口径实现轴值语义**

**接缝 3（`ai_light_seam_feedback.cpp` / `.h`）**

- `build_feedback_instructions` / `build_feedback_json_schema`（`{understanding, axes[{axis,value,reason}]}`, `maxItems:2`） / `build_feedback_input_text` / `build_feedback_request` / `map_feedback_result`
- 调用点在 `src/doodle_lib/http_method/kitsu/ai_light.cpp:716-772`

**共享件（`ai_light_seam_common.cpp`，310 行）**

- `g_axis_value_semantics_text`（`:73-81`）—— 六轴增量语义的权威定义
- `build_common_lighting_section()`（`:202-229`）—— 职责边界 / 性格→灯光语言映射表 / 单位与格式口径**三段原文**
- `describe_targets` / `describe_rig_brief` / `describe_spec_brief` / `describe_preset_brief`（`:179-308`）
- `make_seam_client`（`:51-83`）—— operator→`ai_studio_id_`→`ai_studio` 的凭据解析（**保留**）
- `chick_inference_ok`（`:88-112`）—— 内容违规 400 / 其它 502 / 非 object 502
- `run_seam_inference`（`:116-154`）—— 落任务行 + 扣费（**保留**）

**校验层（`ai_light_json_validation.h`，211 行）**

- `require_object/array/number/string/bool/vec3/require_enum<E>` 严格族（抛 `bad_gateway`）
- `opt_*` 宽松族
- `check_schema_consistency` / `validate_schema_is_wellformed` / `validate_model_output`（pboettch/json-schema-validator，**只诊断写日志**，决定 D-1）

---

## 3. 目标架构

### 3.1 一句话

**后端 = 凭据代理 + openCV 统计器 + IR 准入闸门；前端 = 提示词组装 + 模型回复解释 + 灯光数据决策。**

### 3.2 边界重新划分

| 能力 | 现状 | 降级后 |
|---|---|---|
| 模型凭据 | 服务端 | **服务端**（不变） |
| 调 Ark / 落任务行 / 扣费 | 服务端（接缝内部） | **服务端**（网关内部，不变） |
| 提示词模板 | 服务端 C++ | **前端 TS**（后端降级为只读托管，见 §3.4） |
| JSON Schema | 服务端 C++ | **前端 TS**（同上，托管） |
| 模型回复 → 强类型 | 服务端接缝 | **前端 TS**（唯一的解释者） |
| 参考图 openCV 统计 | 服务端 | **服务端**（不变，P3） |
| 统计聚合 / 冲突判定 | 服务端 | **服务端**（不变） |
| IR 准入闸门 | 无（校验只写日志） | **服务端**（新增，见 §3.5） |
| 候选展开（轴 → 6 个 rig） | 服务端 | **前端** |
| 人工判断落库 / 状态机 | 服务端 | **服务端**（不变） |
| 派发 / 回传 / UE 执行 | 服务端 + 工作机 + UE | **不变** |

### 3.3 网关接口形状

#### 3.3.1 主接口

```
POST /api/doodle/ai/light/inference
Content-Type: application/json
```

**请求体**（`inference_request_t` 的 JSON 形状，`src/doodle_lib/http_client/seedance2_inference_request.h` 是唯一权威）

```jsonc
{
  "input_text": "……",          // 必填
  "instructions": "……",         // 可选，系统指令
  "images": ["https://…"],      // 可选，公网可达 URL
  "image_detail": "high",       // 可选
  "model": "deepseek-v4-1-flash-260910",  // 可选，缺省用 g_ark_inference_model
  "temperature": 0.2,           // 可选
  "top_p": 0.9,                 // 可选
  "max_output_tokens": 4096,    // 可选，缺省 g_ark_inference_default_max_output_tokens
  "thinking": "disabled",       // 可选
  "reasoning_effort": "medium", // 可选
  "json_schema": { … },         // 可选，结构化输出契约
  "json_schema_name": "lighting_candidates",
  "json_schema_strict": true
}
```

**响应体**（**关键：后端不解释 `result_json_`，原样透传**）

```jsonc
{
  "task_id": "uuid…",           // seedance2::task 行 uuid，审计与计费对账用
  "result_text": "……",          // 模型原始文本回复
  "result_json": { … },         // 模型回复是合法 JSON object 时原样给出，否则 null
  "usage": {
    "input_tokens": 1234,
    "output_tokens": 567,
    "total_tokens": 1801,
    "consumed_tokens": 1801      // 实际扣减的 person_token
  },
  "model": "deepseek-v4-1-flash-260910",
  "request_id": "…"             // 方舟侧 request id，排障用
}
```

**后端在网关里**：

1. `person_.check_not_outsourcer()` + `check_project_access(project_id_)`（与现有 AI 灯光路由同一套鉴权）
2. `make_seam_client(in_sql, person_.person_.uuid_id_, "inference")` 解析凭据（复用 `ai_light_seam_common.cpp:51-83`）
3. 用 `inference_request_t::from_json` 反序列化请求体 → `run_inference_task`（`src/doodle_lib/http_method/seedance2/inference_task.h:46`）落任务行 + 扣费
4. 原样返回 `result_text_` / `result_json_` / 用量

**后端在网关里绝对不做**：不读 `result_json_` 的任何字段、不校验 schema、不构造任何 `lighting_*` 类型、不落任何 `ai_light_*` 业务表（任务行除外，那是计费凭据）。

#### 3.3.2 模板与 Schema 托管接口

为消掉「双份 schema 必然漂移」的风险，模板与 schema 仍由服务端托管，前端**读取**而非重写：

```
GET /api/doodle/ai/light/prompt_assets
GET /api/doodle/ai/light/prompt_assets/{name}
```

```jsonc
// GET /prompt_assets/{name} —— name ∈ {spec, candidates, feedback, common}
{
  "name": "candidates",
  "revision": 3,                 // 服务端常量版本号，模板改动必须递增
  "instructions": "……",           // build_candidates_instructions() 的输出
  "json_schema": { … },          // build_candidates_json_schema() 的输出
  "json_schema_name": "lighting_candidates",
  "json_schema_strict": true
}
```

**为什么要保留托管**：`build_common_lighting_section()`（`ai_light_seam_common.cpp:202-229`）里那段「§5.5 性格→灯光语言映射表 + §6.1 职责边界 + §6.3 冲突规则 + 单位与格式口径」是**行为等价性的唯一来源**。逐字搬运到 TS 一次是允许的，但让它变成两份可独立演化的副本是不允许的。

前端用法：拿到 `instructions` 与 `json_schema` 后原封不动塞进 §3.3.1 的信封。前端**只负责填 `input_text`**（把 openCV 统计 + 用户提示词拼成用户消息）。

> 若最终选择"模板彻底搬到前端"，则本接口退化为一条文档导出，`revision` 改由前端仓库管理。这是一个可逆的局部决定，不阻塞主线。

#### 3.3.3 参考图统计接口（P3 的落点）

```
GET /api/doodle/ai/light/session/{sid}/stats
```

```jsonc
{
  "references": [
    { "reference_id": "uuid…", "is_outlier": false, "stats_confidence": 0.82,
      "stats": { "space": "display_srgb", "luma_percentiles": {…}, "palette": […],
                 "contrast": 0.61, "white_balance_kelvin": 5200, "clip_rate": {…},
                 "retouch_score": 0.3 } }
  ],
  "aggregated": {              // aggregate_reference_stats() 的输出，服务端算
    "stats": { …, "conflicts": […], "per_dimension_confidence": {…} },
    "targets": { … },          // lighting_targets_t，服务端按中位数推导
    "dimension_spread": { … },
    "suspected_outlier_ids": [ … ]
  }
}
```

前端把 `aggregated` 当作**输入**拼进 `input_text`，把 `references` 当作展示面。**冲突判定不由前端做**（决定 D7 不变）。

#### 3.3.4 提交灯光数据的接口

```
POST /api/doodle/ai/light/session/{sid}/iteration
```

```jsonc
{
  "iteration": 3,                       // 前端自增；服务端校验 == current_iteration + 1
  "strategy": "…",                      // 轴计划策略文字，仅供展示
  "axes": [ {"axis":"key_direction","steps":[ -20, 20 ], "label":"…"} ],
  "candidates": [                       // 长度必须 == g_candidate_count
    {
      "candidate_index": 0,
      "variant_axis": "none", "variant_value": 0.0, "variant_label": "基线",
      "globals": { … },                 // globals_t 全字段
      "lights": [ { "id":"L01", "role":"key", … } ],   // light_t 全字段
      "ops": [ { "op":"modify", "light":"L01", "patch": {…}, "rationale":"…" } ]
                                          // 可选，审计影子；权威状态是上面的 lights+globals（决定 D-58，见 §3.3.4.1）
    }
  ]
}
```

服务端在此**唯一一次**做 IR 准入（§3.5），通过后 `sync_from_rig` 落 `ai_light_candidate.rig_`，写入 `axes_`，置轮次 `generating`，然后走**现有的派发链路**（不变）。

##### 3.3.4.1 `ops` 的语义定位（决定 D-58，路线 A —— 已采定）

**背景。** UE 侧 `DoodleLightingExecutor.cpp:626-628` 的 `BuildRigLights` 头注释就是当前设计的官方表态：

```cpp
/// 按 rig 全量建灯（`rig.ops` 只做审计：`diff_rigs` 永不产生 add/remove，§6.6 同轮 6 个候选灯集合完全一致）。
```

`ApplyCandidateToWorld`（`:703-733`）的执行路径是：`DestroyAiSpawnedActors()` → 全局量落 PostProcessVolume / SkyLight / Fog → `BuildRigLights(L_Rig, ...)`（**只读 `InRig.Lights``）。**`InRig.Ops` 从头到尾没有被读过一次。**

`ops` 是「派生审计」而非「指令」：`diff_rigs`（`src/doodle_lib/http_method/kitsu/ai_light_rig_ops.cpp:168-231`）的输入是 `(基线 rig, 目标 rig)` 两个**已经完整的** rig，`patch_` 用**目标值整块**（语义是 set，不是增量），且注释明说「展开器只改变量、不增删灯，所以两边 id 集合必然相同，这里也不会产生 add / remove」。
**`lights` 是权威状态，`ops` 是它的差分影子**；影子再生效一次就是重复应用。

**采定路线 A。**

| 项 | 约定 |
|---|---|
| **权威状态** | `candidates[i].lights` + `candidates[i].globals`。服务端与 UE **只消费它们** |
| **`ops` 的地位** | 可选**编辑说明**，仅供展示、审计与回归对照；服务端原样落库，**任何执行路径都不得解释它** |
| **UE 侧改动** | **零**。`BuildRigLights` 保持「按 `lights` 全量建灯」 |
| **前端职责** | 维护 `reduce(ops, baseRig) → {lights, globals}` 折叠器。该折叠器编辑 UI 本来就要有（撤销栈、实时预览都需要完整状态），不是额外负担 |
| **服务端闸门** | 只提交 `ops` 而缺折叠后的 `lights`/`globals` → IR 闸门 400：`候选 N：ops 是审计字段，lights/globals 才是权威状态（D-58）` |

**为何不走路 B（让 UE 解释 `ops`）。** 那会引入「两套状态表示都能生效」的长期复杂性，且必须真正实现 `light_op_kind` 的 `add` / `remove` —— 它们**目前从未被 `diff_rigs` 产生过**（`rig.h:521-541` 的 `light_op_t`，`ai_light_rig_ops.cpp:184-189` 只用到四个 set 类），语义是「已声明、未实现」；实现它们要回答「新灯 id 谁分配」「语义 id 与 UE actor 如何对应」，还要上真机验证（方案 §15.17.5 记录「仍未落到真机」）。路线 A 用前端一个 `reduce` 就拿到了同等表达力。

#### 3.3.5 删除的路由

| 路由 | 处置 |
|---|---|
| `POST /session/{sid}/iterate` | **删除**。它现在"刻意不读请求体"、由服务端跑完整接缝；降级后这个语义不存在 |
| `POST /session/{sid}/judge` 的文本→轴通道 | **删除**（`ai_light.cpp:716-772` 整段）。前端提交结构化 `feedback_axes`，不再需要服务端调 LLM 做结构化 |
| `POST /session/{sid}/spec` | **删除**（第三梯队落地后）。它最后只剩「读会话 + 校验提示词非空 + 返回会话详情」，一个字节都不写；`GET` 的 Spec 版本列表也一并删除（与会话详情的 `spec_history` 重复）。**改提示词**改走 `PUT /session/{sid}`。**Spec 的落库入口目前不存在** —— 见 §3.3 第三梯队的形态 A |
| `POST /session/{sid}/spec/confirm` | **保留**（人工闸门，与 LLM 无关） |

---

### 3.4 状态机变化

现状（`src/doodle_core/metadata/ai_light/session.h:65-83`）：`draft → awaiting_spec_confirm → generating → awaiting_judgement`，**无终态**。推进由 `advance_session` / `advance_session_local`（`ai_light_orchestrator.cpp:522-800`）内的 `advance_session_spec_branch` / `advance_session_generating_branch` 驱动。

降级后的变化是**去掉服务端内部的 `co_await` 模型**，状态集合本身不变：

| 状态 | 现在怎么进 | 降级后怎么进 |
|---|---|---|
| `generating` | 服务端自己调 `generate_candidates` 后进入 | 前端 `POST /iteration` 提交候选后进入 |
| `awaiting_judgement` | 服务端收到工作机回传后进入 | **不变** |
| `awaiting_spec_confirm` | 服务端自己调 `generate_spec` 后进入 | 前端提交 Spec 后进入 |
| `draft` | 建会话 | **不变** |

`advance_session_spec_branch` / `advance_session_generating_branch` 里所有 `co_await generate_spec/generate_candidates` 调用点删除，逻辑变成"读库 → 校验入口状态 → 返回"。§15.21 / §15.23 已经把这个方向推了一半（回滚已撤、同步失败不落库），本方案是它的延续。

---

### 3.5 IR 准入闸门（新增，必须做）

前端算错的 rig 一旦下发，第一个发现问题的是 **UE 侧的 `DoodleLightingExecutor`**（`ParseLightingRig` / `ParseGlobals` 逐字段强校验），而那时错误位置离责任人已经很远——美术看到的是黑屏或崩溃，不是"候选 2 的 `elevation_deg` 超范围"。

所以在 `POST /iteration` 里加**一次**服务端校验，复用现有的 `ai_light_json_validation.h` 严格族（`require_*` / `require_enum<E>`），判据取现状 `map_candidate_result` 已有的三条：

1. 候选数 `== g_candidate_count`（`ai_light_rig_ops.h` 的常量）
2. 每个候选**至少有一个 `role=key` 的主光** —— 现状的 502 文案：「候选基线 rig 里没有 role=key 的主光：key_direction / key_fill_ratio 两个轴都会变成空操作，§9.1 要求的「候选之间真的在目标上有差异」无法成立」
3. 相邻候选 **substance 必须不同** —— 现状的 502 文案：「候选 {} 沿轴 {} = {} 展开后与基线完全相同（这条轴在当前 rig 上不生效）」

外加决定 D-1 的升级路径：`validate_model_output` 现在是"只写日志"，在本方案里**网关侧不启用**（网关不解释结果），闸门只在 `POST /iteration` 生效。

---

## 4. 模板搬迁清单

**这一节是全部风险所在。** 迁移的不是代码结构，是**行为等价性**。

### 4.1 必须逐字保留的原文段落

| 来源 | 内容 | 落地位置 |
|---|---|---|
| `ai_light_seam_common.cpp:202-229` | `build_common_lighting_section()` —— 「职责边界」「性格→灯光语言映射表」「单位与格式口径」三段 | 托管模板 `common` |
| `ai_light_seam_common.cpp:73-81` | `g_axis_value_semantics_text` —— 六轴增量语义（`key_direction` 度、`key_fill_ratio` 绝对值、`kelvin` K、`hardness` −1..+1、`ambient_level` −1..+1、`grading_strength` −1..+1） | 托管模板 `common` |
| `docs/ue_ai_lighting_plan.md` §5.5（`L419-431`） | 性格→灯光语言映射表六行（阴郁→高 key:fill 6:1–10:1 硬侧高偏冷 等） | 同上，与 C++ 原文比对一致 |
| `docs/ue_ai_lighting_plan.md` §6.1（`L451-460`） | LLM 职责边界 | 同上 |
| `docs/ue_ai_lighting_plan.md` §6.3（`L484-494`） | 提示词与参考的冲突规则（**提示词优先于参考**） | 同上 |
| `docs/ue_ai_lighting_plan.md` §6.6（`L539-552`） | 护栏（硬约束） | 同上 |

### 4.2 类型级搬迁

| C++ 位置 | 前端职责 |
|---|---|
| `ai_light_rig_ops.h:44-58` `key_fill_ratio_of` / `apply_axis_step` / `expand_candidates` / `diff_rigs` | **重写为 TS**。这些是确定性算子，必须与 UE 侧同口径（`ai_light_rig_ops.h` 注释点明） |
| `ai_light_seam_candidates.cpp:143-194` `parse_axis_plan` | **重写为 TS**（档位总数校验、轴名合法性、不含 0 档） |
| `ai_light_seam_candidates.cpp:344-366` `default_axis_plan` | **重写为 TS**（硬编码 `key_fill_ratio {3,6,10}` + `kelvin {-800,800}`） |
| `ai_light_seam_candidates.cpp:368-452` `build_base_rig` | **重写为 TS**（`light_t` 逐字段构造，含 `exposure_.method_="manual"`、`tint_=0.0`、颜色按 `color_mode` 分支） |
| `ai_light_seam_candidates.cpp:104-135` `pick_presets` | **重写为 TS**（词法匹配 name+tags，同分按名字排序保证可复现）。预设库数据仍来自 `GET /api/doodle/ai/light/preset` |
| `ai_light_seam_spec.cpp` `map_spec_result` | **重写为 TS**，但**保留服务端钉死**：`space` / `palette` / `ownership` / `schema_version` / `reference_stats` / 身份字段仍由服务端在 `POST /spec` 时覆盖 |
| `ai_light_json_validation.h` 严格族 | 前端保留一份**宽松**读取器（只为给出人话错误）；**权威校验在服务端** §3.5 |

### 4.3 JSON Schema 的处置

`build_candidates_json_schema()`（`ai_light_seam_candidates.cpp:233-284`）与 `build_spec_json_schema()` / `build_feedback_json_schema()` 里的 `schema_object` 建造器（`ai_light_json_validation.h`）**全部保留在 C++**，通过 §3.3.2 托管接口暴露。

理由：它们是 `json_schema_strict=true` 发给 Ark 的**契约本身**，不是"解析方案"。让它有唯一权威来源，比让它跟着前端发布节奏漂移更安全。

**字段清单必须完整保留**：`{strategy, axes[{axis,steps,label}], globals{9 个数值字段}, lights[{role,type,azimuth_deg,elevation_deg,distance_m,intensity_unit,intensity,color_mode,kelvin,rgb,filter,softness,source_radius_cm,cast_shadows,note}]}`。刻意**不含** `variant_axis` / `baseline_level` / `ops` / `rig_id` / `candidate_index` / `schema_version` —— 这些是服务端（降级后是前端）在解释阶段补的，不是模型该给的。

---

## 5. 三梯队落地顺序

原则：**按"搬错的代价"从小到大排，每一梯队都必须能独立跑通并验证。**

### 第一梯队：网关 + feedback（低风险，验证方向）

**为什么先做这个**：feedback 的文本→轴是纯函数、无数据库依赖、现状本就"任何失败都静默降级"（`ai_light.cpp:765-771`），搬错的代价最小。同时它能在最小成本下证明"前端解释模型输出"这条路跑得通。

**改动**

1. 新增 `POST /api/doodle/ai/light/inference`（§3.3.1）
2. 新增 `GET /api/doodle/ai/light/prompt_assets[/{name}]`（§3.3.2）
3. 删除 `ai_light.cpp:716-772` 的文本→轴通道；`judge` 只接受结构化 `feedback_axes`
4. 删除 `ai_light_seam_feedback.{h,cpp}` 的调用；`structure_feedback` 可整体删除（无兼容性负担）
5. 前端实现：拉 `feedback` 模板 → 拼 `input_text` → 调网关 → 解释 `{understanding, axes[{axis,value,reason}]}` → 塞进 `feedback_axes` 提交 `/judge`

**验收**：同一段反馈文本，前端解释出的轴与服务端旧实现一致（可用现有单测的用例做对照）；轴名非法/值 0/超过 2 条仍被服务端丢弃。

### 第二梯队：candidates（高风险高收益，主体工作）—— **已落地**

**为什么第二批**：契约强制最重（`role=key`、档位总数、substance 差异），但收益也最大——提示词与轴计划迭代速度提升最明显。

**改动**

1. ✅ 新增 `POST /session/{sid}/iteration`（§3.3.4）+ IR 准入闸门（§3.5）—— 实现为 `ai_light_submit.{h,cpp}`
2. ✅ 删除 `POST /session/{sid}/iterate`（服务端生成候选的入口）
3. ✅ 删除 `ai_light_seam_candidates.{h,cpp}`（572 行）与 `generate_candidates` 的调用
4. ✅ `advance_session_generating_branch` 里 `co_await generate_candidates` 调用点删除（连"跑接缝 + 护栏"整段与 `g_candidate_count` 个数护栏一起去掉：候选数由前端决定，服务端不再数）
5. ✅ `ai_light_rig_ops.{h,cpp}` 的 `expand_candidates` / `diff_rigs` / `apply_axis_step`：服务端已整体删除，保留常量 `g_axis_step_budget` / `g_axis_max_count` / `g_candidate_count`（前端通过 `session_detail` 读，`docs/ue_ai_lighting_frontend.md:39-54` 第 5 条已要求"不要写死 6"）。
   注：`diff_rigs` 的**生产**调用方随接缝一起没了，但 `ai_light_submit.cpp:45-83` 为 IR 准入闸门重写了一份等价的「只比灯怎么打」的子集比较，测试里仍需要它们做候选自检，故保留实现。
6. ⏳ 前端实现：§4.2 全部类型级搬迁 + `build_candidates_input_text` + `map_candidate_result` 的三条判据（**不在本仓库**）
7. ✅ ~~同步处理 §6.1 的 `ops` 阻塞点~~ —— **已定：路线 A**（决定 D-58，§3.3.4.1）。UE 侧零改动，前端提交前折叠 `ops → lights`+`globals`；服务端闸门拒绝只有 `ops` 的候选

**落地后服务端剩下的**：`advance_session_generating_branch` 只做闸门校验（Spec 存在 / `human_confirmed_` / iteration 递增 / operator 非 nil）→ 推装配给 UE（`shot_render_light_sync_only`）→ 写 submitted iteration；候选与基线 rig 全由前端提交，服务端既不排轴也不展开。

### 第三梯队：spec（语义最纠结，建议最后或保持）

**为什么最后**：它和 openCV 统计是一体的。`map_spec_result` 钉死的 `reference_stats` / `space` / `ownership` / `targets` 既是**输入**也是**输出**。拆开会让"谁钉死 reference_stats"变成一个没有好答案的问题。

**两个可选形态**

- **形态 A（推荐，成本低）**：前端拉 `spec` 模板 + `GET /session/{sid}/stats` 的 `aggregated` → 调网关 → 解释出 `intent` / `targets` 标量区间 / `grading_hints` → **提交给一个尚待新建的 Spec 落库接口**，服务端仍覆盖 `space`/`palette`/`ownership`/`schema_version`/`reference_stats`/身份字段，并保留 `stats_only`/`prompt_only` 两级降级判定
- **形态 B（彻底）**：Spec 也完全由前端产出，服务端只校验形状。代价是 `reference_stats_t` 的字段语义要完整暴露给前端，且 §12.1 两级降级由前端决定

> **现状（第三梯队只做了一半）**：老接口 `POST /session/{sid}/spec` 已删除（它退化成空写），但形态 A 的**替代入口还没建** —— 于是生产代码里**没有任何地方**会 insert 一行 `ai_light_spec`。目前能跑通的全靠测试直接插行，真机上 Spec 无法产生。这是本方案**唯一未闭合的缺口**，须在形态 A 落地时补上（`ai_light_spec` 表、`get_ai_light_spec(s)` 读取方法、`spec_confirm_route` 与 `ai_light_submit.cpp` 的消费端都仍然健在，缺的只是写入方）。

**建议先不做第三梯队。** 第一、二梯队落地后，spec 的瓶颈是否还在服务端会变得清楚——`build_spec_*` 的改动频率远低于 candidates。

---

## 6. 遗留阻塞点

### 6.1 UE 侧 `ops` 从未被 apply —— **已采定路线 A**（决定 D-58）

`DoodleLightingJson.cpp:410-434` 把 `ops` 解析进 `FDoodleLightingRig::Ops`（定义在 `DoodleLightingTypes.h:314-329`），但 `DoodleLightingExecutor.cpp` 的 `BuildRigLights`（`:628-661`）**只消费 `rig.lights` 与 `globals`，从不 apply `ops`**。

**这不是缺陷，是既有设计**：`BuildRigLights` 头注释（`:626-628`）写明「`rig.ops` 只做审计：`diff_rigs` 永不产生 add/remove」。`ops` 是 `lights` 的**差分影子**，不是指令。

**采定：路线 A。** `ops` 保持为前端的本地编辑记录（撤销栈 / 展示 / 排障），前端**折叠进 `lights` + `globals` 后再提交**；UE 侧**零改动**。完整契约与「为何不走路 B」见 **§3.3.4.1**，前端交接口径见 `docs/ue_ai_lighting_frontend.md` §2.1。

路 B（在 UE 执行器实现 op 应用语义）已否决：引入双生效路径，且必须实现从未产生过的 `add` / `remove`（`rig.h:521-541` 的 `light_op_t`），并上真机验证。

### 6.2 计费与审计的粒度变化

现状一次候选生成 = 一次推理 = 一条任务行。降级后前端可能**试多次**才提交一份候选，每次试都产生一条任务行与一次扣费。这是特征不是缺陷（试错本来就该计费），但需要：

- 前端在 UI 上明确"本次提交前已消耗 N 次推理"
- 给网关加**频率上限**，防止误循环烧 token

### 6.3 前端工程现状

`docs/ue_ai_lighting_frontend.md:537-545` 记录：生产前端是 Kitsu fork 部署在 `D:\kitsu\dist`（**只有构建产物、没有源码**）；`E:\doodle_vue\doodle` 是近乎空白的 Vuetify 3 脚手架。

**本方案要求前端持有实质业务逻辑**（提示词组装、模型回复解释、轴展开、rig 构造），这与"只有一个空脚手架、生产前端无源码"的现状是冲突的。§4.2 的搬迁工作量全部落在前端，**前端工程必须先可用**。这是本方案最大的非技术风险。

---

## 7. 与上游文档的关系

| 上游位置 | 处置 |
|---|---|
| `docs/ue_ai_lighting_plan.md` §3.1、§6、§8.4、§9.1、§13.1、§14.5 | 描述降级前形态；本方案落地后需按 §5 逐条修订 |
| `docs/ue_ai_lighting_plan.md` §14.5「规划逻辑不要放到前端」 | **本方案直接推翻这一条**（P1/P2）。落地后必须改写，否则文档自相矛盾 |
| `docs/ue_ai_lighting_plan.md` §5.1–5.5（感知层） | **完全保留**（P3），仅新增 §3.3.3 的暴露接口 |
| `docs/ue_ai_lighting_plan.md` §15.7–15.9（三条接缝的落地记录） | 变成历史记录；落地后加"已被 §降级 取代"标注 |
| `docs/ue_ai_lighting_frontend.md` §2「规划逻辑不要放到前端」、§6、§12 | 同上，需重写第 2 条与 §12 的「提示词模板/轴计算/Spec 词表/IR schema 校验」一行 |
| `docs/ue_ai_lighting_plan.md` §15.21 / §15.23 | **本方案的前置**（回滚已撤、同步失败不落库），保持不变 |
| 决定 D-1（校验只写日志） | 网关侧不启用；`POST /iteration` 的闸门（§3.5）是它的第一次真正启用 |
| 决定 D7（冲突只由服务端判定） | **不变** |
| 决定 D34（全局量落点） | **不变** |

---

## 8. 未决问题

1. **模板归属**：托管（§3.3.2，消漂移）还是彻底搬到前端（迭代最快）？本文默认为托管，可随时反转。
2. **`ops` 路线**：§6.1 的路 A 还是路 B？
3. **`POST /iteration` 的校验强度**：只做 §3.5 三条，还是把 `light_t` 全字段范围（`azimuth_deg ∈ [0,360)`、`kelvin ∈ [1500,15000]` 等）也校验？
4. **第三梯队做不做**：形态 A / 形态 B / 保持服务端。
5. **前端工程**：§6.3 的阻塞如何解决——先把 `E:\doodle_vue\doodle` 建起来，还是拿到 Kitsu fork 源码？
