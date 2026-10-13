# AI 灯光 · C++ 侧角色降级实施计划

> 本文是 [ue_ai_lighting_role_downgrade.md](ue_ai_lighting_role_downgrade.md) 的**施工版**：
> 那份文档回答「降级可行吗、长什么样」，本文回答「C++ 侧改哪些文件、按什么顺序、怎么验」。
>
> 前端侧不在本文范围（用户 2026-02 决定：前端由实现方自行决定，服务端只出文档 —— 见
> [ue_ai_lighting_frontend.md](ue_ai_lighting_frontend.md) §14 交付边界）。
>
> 上游编号一并沿用：**决定 D-58** = `ops` 采定路线 A（`docs/ue_ai_lighting_frontend.md` §2 第 7 条 / §2.1）。
> 本文新增的决定编号从 **D-59** 起。

---

## 0. 一句话结论

**可行，且 C++ 侧的改动量比看起来小得多**：三条接缝（spec / candidates / feedback）本就是
「纯函数组信封 + 一次网络调用」的分法 —— 搬迁只搬**纯函数那一半**，网络那一半原地保留。
真正新增的只有一个网关路由，真正危险的只有一处（`judge_route` 的轴通道，删掉会静默降级反馈质量）。

---

## 1. 现状盘点（已核实，带行号）

### 1.1 三条接缝的「可搬 / 不可搬」分界

| 接缝 | 文件 | 纯函数（搬到前端） | 网络（C++ 保留） |
|---|---|---|---|
| 1 spec | `src/doodle_lib/http_method/kitsu/ai_light_seam_spec.{h,cpp}`（103/473 行） | `build_spec_instructions()` / `build_spec_json_schema()` / `build_spec_input_text()` / `build_spec_request()` / `map_spec_result()` | `generate_spec()` |
| 2 candidates | `ai_light_seam_candidates.{h,cpp}`（134/572 行） | `build_candidates_instructions()` / `build_candidates_json_schema()` / `build_candidates_input_text()` / `map_candidate_result()` | `generate_candidates()` |
| 3 feedback | `ai_light_seam_feedback.{h,cpp}`（105/352 行） | `build_feedback_instructions()` / `build_feedback_json_schema()` / `build_feedback_input_text()` / `build_feedback_request()` / `map_feedback_result()` | `structure_feedback()` |

**轴展开不在上面任何一处**：`apply_axis_step()` / `enumerate_variants()` 在
`ai_light_rig_ops.cpp`（233 行，`ai_light_rig_ops.h` 只 62 行），它**没有网络依赖**，
是纯粹的值运算 —— 整个文件可整体搬迁。

### 1.2 三条接缝共用的底座

`ai_light_seam_common.h`（152 行）/ `.cpp`（310 行）。逐项定性：

| 符号 | 行 | 定性 | 处置 |
|---|---|---|---|
| `seam_request_base_t`（`operator_id_` / `project_id_` / `task_id_` / `seam_`） | `.h:31` | 计费与凭据绑定 | **保留** |
| `light_t* first_light_with_role(lighting_rig_t&, light_role)` | `.h:42` | 纯 | 搬迁 |
| `bool rig_is_valid_candidate(const lighting_rig_t&)` | `.h:56` | 纯 | **保留**（服务端闸门要用，见 §3.2） |
| `bool candidates_are_substantively_different(...)` | `.h:70` | 纯 | **保留**（同上） |
| `std::string key_fill_ratio_of(...)` | 在 `ai_light_rig_ops.h` | 纯 | 搬迁 |
| `void chick_inference_ok(...)` | `.cpp:88` | 判定推理成败 | **保留** |
| `resolved_inference_t make_seam_client(sqlite_database&, uuid operator_id, seam_kind)` | `.cpp:51` | 解析 `operator_id_ → person.ai_studio_id_ → ai_studio` 取密钥 | **保留** |
| `run_seam_inference(sqlite_database&, seam_request_base_t&, std::string_view seam)` | `.cpp:116` | 建 client → 建任务行 → `run_inference_task`（自成一事务，落任务行 + 扣费）→ `chick_inference_ok` | **保留** |

`run_seam_inference` 是**整条降级的关键复用点**：它已经把「凭据 + 计费 + 成败判定」
收在一个函数里，网关路由直接调它即可，不必重写。

### 1.3 现在的编排调用形态

`advance_session_generating_branch()`（`ai_light_orchestrator.cpp:232-541`）里，
本体在 `:329` 调 `co_await generate_candidates(in_sql, l_req)`，
返回 `seam_inference_result_t`（`green_` = 模型 JSON，`task_id_` = 计费任务行 id）。

`advance_session_spec_branch()`（`:147-231`）同形，本体在 `:168` 调 `co_await generate_spec(in_sql, l_req)`。

两个分支都遵守 `ai_light_orchestrator.h` 顶部的三条契约（抛出即本轮无法推进 /
抛之前不落库 / 返回值是纯值）。

### 1.4 路由现状

全部 16 条 AI 灯光路由注册在 `src/doodle_lib/http_method/kitsu.cpp:302-356`，
均在 `create_kitsu_route_2`（主服务器）内，链式 `.reg_t<类>("/path/{}"_url(&类::成员), ...)`。

其中与本文直接相关的三条：

| 注册行 | 路径 | 处理函数 |
|---|---|---|
| `kitsu.cpp:326-328` | `POST /api/doodle/ai/light/session/{}/iterate` | `ai_light.cpp:660` `iterate_route` |
| `kitsu.cpp:323-325` | `POST /api/doodle/ai/light/session/{}/spec/confirm` | `ai_light.cpp:586` `spec_confirm_route` |
| `kitsu.cpp:329-331` | `POST /api/doodle/ai/light/session/{}/judge` | `ai_light.cpp:697` `judge_route` |

### 1.5 OpenCV 现状（**用户前提已成立，无需新增依赖**）

- 顶层 `CMakeLists.txt:84` 已有 `find_package(OpenCV CONFIG REQUIRED)`。
- `src/doodle_lib/CMakeLists.txt:65-66` 已链接 `${OpenCV_LIBS}` + `opencv_ml opencv_dnn opencv_core opencv_flann`。
- 已有代码在用：`ai_light_stats.cpp:5-6`（`imgcodecs.hpp` / `imgproc.hpp`）、
  `ai_light_stats.h:35`（`core.hpp`）、`ai_light.cpp:37`（`opencv.hpp`）、
  `ai_light.cpp:151`（`cv::imread`）、`preview.cpp:35`、`pictures.cpp:17` 等。

**结论：用户说的「后端只用 OpenCV 分析图片」不需要任何构建改动** —— 统计层
（`ai_light_stats.{h,cpp}`，742 行 .cpp）本来就是 OpenCV 实现，且**已经和 HTTP 层解耦**
（`ai_light_stats.h:12` 明写「本模块不含任何 HTTP 依赖，单测可以直接调」）。

---

## 2. 目标形态（C++ 侧）

### 2.1 保留：服务端仍是「凭据方 + 计费方 + 落库方 + 闸门方」

```
前端                          后端（C++）                     火山方舟
 │                              │                              │
 │  ── 组装 input_text ────────▶│ (不参与)                     │
 │  ── POST /inference ────────▶│ make_seam_client(operator_id)│
 │     {seam, input_text,       │   ↓ 取 ai_studio.seedance2_key_
 │      json_schema, images?}   │ run_inference_task(...) ────▶│
 │                              │   ↑ 落 seam2_task 行 + 扣费   │
 │  ◀──── {result, task_id} ────│ ◀────────────────────────────│
 │                              │                              │
 │  ── 前端解释 result ────────▶│ (不参与)                     │
 │                                (轴展开 / rig 构造都在前端)   │
 │                              │                              │
 │  ── POST /iteration ────────▶│ 闸门校验候选 → 落库 → 派发    │
 │     {candidates:[{lights,    │                              │
 │       globals, ops?}]}       │                              │
```

### 2.2 服务端保留的责任（**一条都不能少**）

1. **凭据代持** —— `ai_studio.seedance2_key_` 永不出后端（`ai_light_seam_common.cpp:51` 的 `make_seam_client`）。
2. **计费** —— `run_inference_task` 落任务行 + 扣费（契约 2 的受控例外）。
3. **IR 准入闸门** —— 候选数 / `role=key` 主光 / 候选间 substance 差异（详见 §3.2）。
4. **落库与状态机** —— 现状不动。
5. **参考图统计** —— OpenCV 侧（`ai_light_stats`），现状不动。

---

## 3. 要改的东西（逐项）

### 3.1 【新增】推理网关路由

> **⚠️ 实现修正（已按落地代码回写）**：本条下列草稿形状有 **4 处**与最终落地不同，
> 落地代码为准（`http_method/kitsu/ai_light_gateway.{h,cpp}`）：
> 1. **路径挂到会话下**：`POST /api/doodle/ai/light/session/{session_id}/inference`，
>    不再是 `POST /api/doodle/ai/light/inference`。理由：权限校验的对象就是会话行，
>    放路径上则「URL 上操作的对象」与「鉴权查的对象」必然同一个。
> 2. **`session_id` / `operator_id` 都不在请求体里**。前者来自路径，后者来自 JWT。
> 3. **响应不带 `task_id`**（`seam_inference_result_t` 没有这个字段，见下方说明）。
> 4. **`seam` 的值是接缝名**（`generate_spec` / `generate_candidates` / `structure_feedback`），
>    不是草稿写的短名（`spec` / `candidates` / `feedback`）—— 与三条接缝实际调
>    `run_seam_inference` 时传的字面量逐字一致，否则日志与计费台账里会分成两套名字。

**路由**：`POST /api/doodle/ai/light/session/{session_id}/inference`

**请求体形状**（在 `docs/ue_ai_lighting_role_downgrade.md` §3.3 已给，此处落实为 C++ 侧契约）：

```jsonc
{
  "seam": "generate_spec" | "generate_candidates" | "structure_feedback",  // 必填
  "input_text": "...",                           // 必填；前端组装的完整用户消息
  "instructions": "...",                         // 可选；缺省时服务端取内置值
  "json_schema": { ... },                        // 可选；缺省时取内置 schema
  "images": [ "base64..." ],                     // 可选
  "model": "...", "temperature": 0.7, "top_p": 0.9, "max_output_tokens": 4096,
  "thinking": "...", "reasoning_effort": "...", "image_detail": "...",
  "json_schema_name": "ai_light_spec", "json_schema_strict": true
}

// 注意：**没有** session_id、**没有** operator_id —— 见下方「权限」段。
```

**响应体形状**：

```jsonc
{
  "result": { ... },         // 模型返回的原始 JSON，服务端**不做任何解释**
  "seam": "generate_spec",
  "input_tokens": 123, "output_tokens": 456, "total_tokens": 579
}

// **没有 task_id**：seam_inference_result_t 只有 json_ + 三项 token 计数
// （ai_light_seam_common.h:130-135），要带得改它 + run_inference_task 的返回，
// 不列入第一梯队（见 §7 待实测第 1 条）。计费留痕在 seedance2::task 行里。
```

**实现落点**：新增 `src/doodle_lib/http_method/kitsu/ai_light_gateway.{h,cpp}`，
类名 `inference_route`，在 `kitsu.cpp` 的 `create_kitsu_route_2` 内追加一行 `.reg_t<>`。

**核心实现**（直接复用 `run_seam_inference`，**约 60 行**）。

先看**真实签名**（`ai_light_seam_common.h:146-150`，此处逐字引用）：

```cpp
boost::asio::awaitable<seam_inference_result_t> run_seam_inference(
    sqlite_database& in_sql, const seam_request_base_t& in_request, std::string_view in_seam,
    std::string_view in_text_prompt, const http::seedance2::inference_request_t& in_data_request,
    const uuid& in_iteration_id = {}
);
```

以及 `seam_request_base_t` 的**真实**字段（同名文件 `:111-122`）—— 只有三个，**没有 `seam_`**：

```cpp
struct seam_request_base_t {   // 不给 DOODLELIB_API（内部件，见该处注释）
  uuid session_id_{};
  uuid project_id_{};   // 写 sd2::task.project_uuid_id_，只为归属可查
  uuid operator_id_{};  // 计费主体 + 签名 URL 的 uid；**无默认值**，nil 时由 make_seam_client 抛
};
```

所以网关**不需要**给 `seam_request_base_t` 加任何字段：信封（`in_data_request`）
与计费元数据（`in_request`）本来就是**两个独立入参**。

```cpp
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(inference_route, post) {
  auto l_json = get_json_body();
  auto l_seam = json_val::require_enum<seam_kind>(l_json, "inference", "seam");  // 认不出 → 400

  // operator_id 必须等于 JWT 本人 —— 否则任何一个登录用户都能拿别人的工作室密钥刷量。
  // 这是网关**唯一**的新权限点，必须在这里挡（测试见 ai_light_gateway.cpp）。
  const uuid l_operator = get_person().person_id_;

  seam_request_base_t l_base{};
  l_base.operator_id_ = l_operator;                                    // 显式传，不给默认值
  l_base.session_id_  = json_val::require_uuid(l_json, "inference", "session_id");
  l_base.project_id_  = load_detail_checked(get_person(), in_sql, l_base.session_id_).session_.project_id_;
  // ↑ 权限口径与 judge_route / iterate_route 完全一致（ai_light.cpp:185 load_detail_checked）

  // 信封：body 给了 json_schema 就用它，否则落到内置的那一份（§3.1.2）
  auto l_req = build_gateway_inference_request(l_json, l_seam);

  auto l_res = co_await run_seam_inference(
      in_sql, l_base, seam_name_of(l_seam), /*in_text_prompt=*/std::string{l_req.input_text_}, l_req
  );
  co_return nlohmann::json{
      {"result", l_res.json_}, {"seam", seam_name_of(l_seam)},
      {"input_tokens", l_res.input_tokens_}, {"output_tokens", l_res.output_tokens_},
      {"total_tokens", l_res.total_tokens_}
  };
}
```

**落地代码**（**已实现**，见 `http_method/kitsu/ai_light_gateway.cpp`，约 214 行）：

```cpp
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(inference_route, post) {
  person_.check_not_outsourcer();          // 与 judge_route / iterate_route 同口径
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  // 权限：路径里的 {session_id} → check_project_access。
  // 会话不存在时 get_by_uuid → to_single 抛 sqlite_orm_exception{404}。
  auto l_session = l_sql.get_by_uuid<ai_light_session>(session_id_);
  person_.check_project_access(l_session.project_id_);

  const auto l_seam_name = resolve_seam_name(l_json);          // 白名单，未知 → 400
  auto       l_request   = build_gateway_inference_request(l_json, l_seam_name);

  // operator_id **只从 JWT 取**：请求体里若有 operator_id 键会被彻底忽略。
  // 这是结构性保证 —— 没有任何代码路径能让外部值流进来。
  seam_request_base_t l_base{};
  l_base.operator_id_ = person_.person_.uuid_id_;
  l_base.session_id_  = l_session.uuid_id_;
  l_base.project_id_  = l_session.project_id_;

  auto l_result = co_await run_seam_inference(
      l_sql, l_base, l_seam_name, l_request.input_text_, l_request
  );
  co_return in_handle->make_msg(nlohmann::json{
      {"result", l_result.json_}, {"seam", l_seam_name},
      {"input_tokens", l_result.input_tokens_}, {"output_tokens", l_result.output_tokens_},
      {"total_tokens", l_result.total_tokens_}
  });
}
```

**注意四处与旧草稿的差异**（已按真实代码修正）：

1. `run_seam_inference` **没有** `task_id_` 返回值可用 —— `seam_inference_result_t`（`:130-135`）
   只有 `json_` + 三项 token 计数。所以响应体里**不带 `task_id`**，
   计费留痕在 `seedance2::task` 行里（`l_task.text_prompt_` = 前端传来的 `input_text`，可按它反查）。
   若前端确实要 `task_id`，得改 `seam_inference_result_t` **并**改 `run_inference_task` 的返回，
   **不列入第一梯队**（见 §7 待实测第 1 条）。
2. `seam_request_base_t` **不加字段**。原草稿提议的 `prebuilt_` 字段是多余的 ——
   信封本来就是独立入参，不需要往计费元数据里塞。
3. **不另建 `seam_kind` 枚举**：用一个三项的白名单常量数组 + `resolve_seam_name()`
   直接得到服务端内部 seam 名（就是接缝名本身）。枚举会让「接缝名字面量」与「枚举名」
   之间多一层需要人工对齐的翻译，而白名单只有三项、且这张表就是唯一真源。
   （`json_val::require_enum` 也不能直接用 —— 它抛 `bad_gateway`，那是给「模型输出」
   用的口径；请求体给错了是调用方的问题，应当 400。）
4. **不调 `load_detail_checked`**：那是 `ai_light.cpp` 里为详情页设计的（带 `detail` 结构）。
   网关只要会话行本身，`get_by_uuid<ai_light_session>` + `check_project_access` 就是
   它内部做的事，且 404 口径完全一致。

**信封组装函数** `build_gateway_inference_request(const nlohmann::json&, std::string_view)`
（**已实现**，放在 `ai_light_gateway.cpp` 的匿名 namespace 里 —— 它只服务这一个路由，
不外传就不需要进头文件）：

```cpp
// 从网关 body 组装信封。instructions / json_schema 缺省时落到内置的那一份 ——
// 内置值是**编译期常量**，前端从 /prompt_assets 读到的就是它，两者必须字节相同（§6 风险 6）。
http::seedance2::inference_request_t build_gateway_inference_request(
    const nlohmann::json& in_json, std::string_view in_seam
);
```

**一处静默降级要点明**：`json_schema` 给的不是 object 时，`json_val::opt_object` 返回
`nullopt`，于是**静默落回内置 schema**。这是有意的（宁可跑内置契约，也不发一个必然被
火山拒掉的 schema），但它必须可发现 —— 调用点在 `l_json.contains("json_schema") &&
!l_json.at("json_schema").is_object()` 时记一条 WARN。前端传错 schema 却「看起来成功了」
是最难查的一类问题。

**为什么不另开一个 `run_gateway_inference()`**：那会把「计费 + 成败判定」复制第二份，
而这两件事现在只定义在 `run_seam_inference` 一处 —— 复制必然慢慢分叉。

#### 3.1.2 schema 校验从哪来

现状：三条接缝的 `map_*_result()` 里各自调 `json_val::validate(...)`（用 `ai_light_json_validation.h` 的建造器，
`.h:76` `schema_object(nlohmann::json in_props)` / `schema_enum<...>` 等）。

**网关路径的处置**：body 里带了 `json_schema` 就用它，没带就用**服务端内置的那一份**
（即现有 `build_spec_json_schema()` / `build_candidates_json_schema()` / `build_feedback_json_schema()` 的返回值）
—— 这三份保留在 C++ 侧不删，正是为了它，也为了 `/prompt_assets`（见 3.3）。

**关键约束**：`json_val` 的枚举一律 `magic_enum::enum_cast` 显式校验，
**绝不能**用 `NLOHMANN_JSON_SERIALIZE_ENUM` 的裸 `get<>`（认不出的字符串会静默落到第一个枚举值）——
这条在 `ai_light_json_validation.h` 顶部有明确约定，网关校验沿用同一份实现即可，不要另写。

### 3.2 【保留 + 加强】IR 准入闸门

**这是降级后服务端唯一还有「判断力」的地方，必须比现在更严。**

现有判据（`docs/ue_ai_lighting_role_downgrade.md` §3.5）三条：
候选数 == `g_candidate_count` / 每候选至少一个 `role=key` 主光 / 相邻候选 substance 不同。

对应实现已存在于 `ai_light_seam_common.h`：
- `rig_is_valid_candidate(const lighting_rig_t&)`（`:56`）—— 判 `role=key`
- `candidates_are_substantively_different(...)`（`:70`）—— 判 substance 差异

**新增第 4 条判据（D-58 路线 A 的直接后果）**：

> **有 `ops` 就必须同时有非空 `lights`。** 只有 `ops` 的候选一律 400。

理由已在 `ue_ai_lighting_role_downgrade.md` §3.3.4.1 写清：`ops` 是审计影子，
`BuildRigLights`（`script/uePlug/Doodle/.../DoodleLightingExecutor.cpp:628-661`，
头注释 `:626-628` 原文「`rig.ops` 只做审计：`diff_rigs` 永不产生 add/remove」）
只读 `InRig.Lights`，`InRig.Ops` 从未被读。

**同时 `apply_axis_step` 的搬迁是这一条的前提**：轴展开搬到前端后，前端拿到的是
「基线 rig + 各候选的轴档位」，提交时必须**已经折叠成 `lights` + `globals`**。

### 3.3 【新增】`GET /api/doodle/ai/light/prompt_assets[/{name}]`

把三条接缝的 `build_*_instructions()` / `build_*_json_schema()` 以**只读**方式暴露，
供前端「读取后原样使用，不要重写」。

**实现**（**已落地**，`ai_light_prompt_assets.{h,cpp}`，两个路由）：

- `GET /api/doodle/ai/light/prompt_assets`（无路径参数）
  → `{"assets": [{"name", "has_schema", "instructions_chars", "schema_chars"}]}`
- `GET /api/doodle/ai/light/prompt_assets/{name}`
  → `{"name", "instructions", "json_schema", "json_schema_name", "json_schema_strict"}`

**三处与旧草稿的差异**（已按落地代码回写）：

1. **`{name}` 用接缝名本身**（`generate_spec` / `generate_candidates` / `structure_feedback`），
   不是草稿写的短名（`spec` / `candidates` / `feedback`）。理由同 §3.1 第 4 点：
   前端要拿 `name` 去当网关的 `seam` 用，两套名字之间的映射表就是下一个漂移点。
2. **多了第四项 `common`**（= `build_common_lighting_section()`，即被 spec 与 candidates
   的 instructions **包含**的那三段公共文本）。它只在 `/prompt_assets` 里合法，
   **不是**网关的合法 `seam` —— 没有「只跑公共段」这种推理。所以两个白名单是
   「前三个重叠、第四个不重叠」的关系，**不能合并成一个常量**。
3. **`{name}` 的类型必须是 `FSys::path` 而不是 `std::string`**：`_url(...)` 的路径参数
   只接受 uuid / number / 枚举 / `FSys::path`（`http_function.h:159-176` 的 `static_assert`
   会拒掉 `std::string`），`FSys::path` 那一支用的正则就是「一个路径段」。
   与 `kitsu_reg_url.h:479` 的 `data_attachment_files_file::file_name_` 同一写法。
   取值时用 `.generic_string()` 收成字符串。

**为什么要 `json_schema_name` / `json_schema_strict`**：它们是「往网关发这次推理时该带
什么」的一部分。前端若只拿 schema 不带这两个，就得自己在 TS 里写死
（`ai_light_<seam>` 与 `true`），那又是一处会与 C++ 漂移的常量。

**`name` 未知 → 400，不是 404**。`{name}` 是一个四值白名单，不在白名单内是「参数非法」
（调用方拼错了），而不是「这个资源不存在」—— 后者的语义会让人以为将来可能多出一个接缝，
于是前端写出一个动态探测的分支。白名单即全部可能。

**要点**：这三个 `build_*_instructions()` 现在**不在 `DOODLELIB_API` 之外可见**
（它们声明在 seam 头里，伞形头 `ai_light_orchestrator.h` 会 include）——
所以从路由层直接调是可行的，无需额外导出。**但必须逐字保留**（模板搬迁清单见
`ue_ai_lighting_role_downgrade.md` §4.1），这条是前端视觉一致性的唯一保障。

**命名遮蔽的坑**（落地时踩到）：这些函数在 `doodle::ai_light` 里，而路由类在
`doodle::http::ai_light` —— 同名内层命名空间会**遮蔽**外层，直接写 `ai_light::build_spec_instructions()`
会命中 `doodle::http::ai_light` 自己（找不到）。两个新 .cpp 都在匿名 namespace 里加了
`using namespace doodle::ai_light;`（与 `ai_light.cpp:50` 同一写法）。

### 3.4 【删】规划层的服务端调用点

这是「角色降级」四个字在 C++ 侧的实际含义。**建议保留旧路由到前端切换完成**（见 §5 顺序），
切换完成后删：

| 要删的东西 | 位置 | 备注 |
|---|---|---|
| `iterate_route`（`POST /session/{}/iterate`） | `ai_light.cpp:660-696` + `kitsu.cpp:326-328` + `ai_light.h` 里的类声明 | 由 3.1 网关 + 3.5 提交路由取代 |
| `advance_session_generating_branch()` 的接缝调用 | `ai_light_orchestrator.cpp:329`（`co_await generate_candidates`） | 改为「从落库的候选直接进 `awaiting_judgement`」 |
| `advance_session_spec_branch()` 的接缝调用 | `ai_light_orchestrator.cpp:168`（`co_await generate_spec`） | 改为「从落库的 Spec 直接进 `awaiting_spec_confirm`」 |
| `spec_confirm_route` 里的重出分支 | `ai_light.cpp:586-659` | 若「重出」也移到前端 |

**注意**：删掉 `:329` 后，`advance_session_generating_branch` 的返回类型
`generating_branch_result` 里 `dispatch_` 的语义会变 —— 它现在承载「要派发渲染」，
降级后仍然要派发（渲染还是 UE 做），所以**那条链路不动**，只把「候选从哪来」换掉。

### 3.5 【新增】`POST /api/doodle/ai/light/session/{}/iteration`

前端解释完模型回复后，把**已经折叠好的候选**提交上来。

**请求体**：`{ "candidates": [ { "lights": [...], "globals": {...}, "ops": [...] } ], "spec_id": "uuid" }`

**处理**：过 §3.2 的四条闸门 → 落 `ai_light_candidate` 行 → 建 iteration 行 → 派发渲染。

**与现有 `judge_route` 的关系**：`judge_route`（`ai_light.cpp:697`）是「美术看完图给判断」，
本路由是「前端给候选」—— **两者不同，不要合并**。前者用 `apply_judgement`（`:639`），
后者是新函数，可以复用 `mark_iteration_rendered`（`:802`）的落库形状。

### 3.6 【危险项】`judge_route` 的轴通道

**这是整个降级里唯一会「静默变差」的地方，必须显式决策。**

现状（`ai_light.cpp:697-819`）：`judge_route` 收到美术的自由文本后，
调 `co_await structure_feedback(...)`（接缝 3）把文本变成 `std::vector<axis_step_t>`，
再 `apply_axis_step` 进下一轮中心值。这条链路是**自动的**。

降级后如果只保留网关，前端得自己调 `seam=feedback` 再自己 `apply_axis_step`。
**两种走法**：

| 走法 | 后果 |
|---|---|
| **A（推荐）**：前端调 `seam=feedback` 网关，拿回轴后自己展开 | 与「规划层全在前端」一致；前端要多做一次往返 |
| B：`judge_route` 内部继续调 `structure_feedback` | 反馈质量立刻可用，但服务端留了一处规划逻辑，「角色降级」不彻底 |

**建议 A，但排在最后做**（第三梯队，见 §5）—— 因为反馈链路的验证成本最高，
先让 spec / candidates 两条跑通。

### 3.7 【保留不动】明确不碰的部分

- `ai_light_stats.{h,cpp}` —— OpenCV 统计层，本就不含规划逻辑，**一行不动**。
- `ai_light_rig_ops.cpp` 的 `diff_rigs()`（`:168-231`）—— 服务端仍要算基线差分用于展示，
  **保留**；只把 `apply_axis_step` / `enumerate_variants` 搬走。
- `ai_light_json_validation.{h,cpp}`（211/416 行）—— 闸门与网关都用它，**保留**。
- `ai_light_stats.cpp` 的 742 行指标计算、`metrics_` 落库 —— **保留**（§8.1 的「A 目标区间满足」要用）。

---

## 4. 文件级改动清单

| 文件 | 动作 | 估计行数 |
|---|---|---|
| `http_method/kitsu/ai_light_gateway.h` | **新增** | ~40 |
| `http_method/kitsu/ai_light_gateway.cpp` | **新增** | ~180（含 `build_gateway_inference_request`） |
| `http_method/kitsu/ai_light_prompt_assets.h` | **新增** | ~30 |
| `http_method/kitsu/ai_light_prompt_assets.cpp` | **新增** | ~120 |
| `http_method/kitsu/ai_light_submit.{h,cpp}` | **新增**（3.5 的提交路由） | ~200 |
| `http_method/kitsu/ai_light.cpp` | 改：删 `iterate_route`（`:660-696`）、`spec_confirm_route` 重出分支 | −90 |
| `http_method/kitsu/ai_light_orchestrator.cpp` | 改：删 `:168` / `:329` 两处接缝调用 | −60 |
| `http_method/kitsu/ai_light.h` | 改：删 `iterate_route` 类，加 3 个新路由类 | ±30 |
| `http_method/kitsu.cpp` | 改：删 1 行注册，加 4 行 | ±5 |
| `http_method/kitsu/CMakeLists.txt` | 改：加 6 个新文件 | +6 |
| `src/test/core/ai_light_gateway.cpp` | **新增测试** | ~150 |
| `src/test/core/ai_light_submit.cpp` | **新增测试** | ~180 |
| `src/test/core/CMakeLists.txt` | 改：加 2 个测试文件 | +2 |

**第一梯队不动 `seam_request_base_t`**（原草稿提议的 `prebuilt_` 字段已撤销 —— 信封是
`run_seam_inference` 的独立入参，不需要往计费元数据里塞东西）。

**搬迁到前端的文件**（C++ 侧删除或降级为「仅供网关缺省值使用」）：

| 文件 | 行数 | 处置 |
|---|---|---|
| `ai_light_rig_ops.{h,cpp}` 的轴部分 | ~120 | 整体搬到前端；`diff_rigs` 留 C++ |
| `ai_light_seam_spec.{h,cpp}` 的纯函数部分 | ~350 | 搬到前端；`generate_spec` 留 C++ |
| `ai_light_seam_candidates.{h,cpp}` 的纯函数部分 | ~400 | 搬到前端；`generate_candidates` 留 C++ |
| `ai_light_seam_feedback.{h,cpp}` 的纯函数部分 | ~300 | 搬到前端；`structure_feedback` 留 C++ |

**注意**：纯函数「搬到前端」不等于 C++ 侧删干净 —— `build_*_json_schema()` 与
`build_*_instructions()` 要**保留**给 `/prompt_assets`（§3.3）和网关缺省值（§3.1.2）。

---

## 5. 落地顺序（三梯队，每梯队可独立验证）

### 第一梯队：网关 + prompt_assets（**不动任何现有逻辑**）

1. 新增 `ai_light_gateway.{h,cpp}`，注册 `POST /inference`（含 `build_gateway_inference_request`）。
2. 新增 `ai_light_prompt_assets.{h,cpp}`，注册两条 `GET`。
3. 写 `src/test/core/ai_light_gateway.cpp`：
   - `build_gateway_inference_request` 在 `json_schema` 缺省时落到**内置 schema**，
     且与 `build_*_json_schema()` 的输出**字节相同**（防模板漂移，§6 风险 6）
   - 未知 `seam` → 400；未知 `session_id` → 404（走 `load_detail_checked`）
   - `operator_id` 不来自 body 而来自 JWT —— 断言 body 里塞任何 operator 都无效
   - `input_text` 逐字透传到任务的 `text_prompt_`（计费留痕可反查）

**验收**：`--run_test="ai_light_*"` 全绿，且**现有 9 个 ai_light 测试文件一个不红**。
此时前端可以开始用网关，服务端行为零变化。

**本梯队不碰的东西**：`seam_request_base_t`、三条接缝的 `build_*_request()`、
`ai_light_orchestrator.cpp`、`iterate_route`。网关是**纯增量**。

### 第二梯队：candidates 走新路（**第一次真正搬家**）

1. 写 `ai_light_submit.{h,cpp}` + `POST /session/{}/iteration`。
2. 前端组装 input_text → 调网关 → 解释结果 → 折叠 `ops` → 调 submit。
3. 服务端对照运行：旧 `iterate` 与新 `submit` **并行**，比对两者落库的候选是否一致。
4. 比对通过后删 `iterate_route` 与 `advance_session_generating_branch:329`。

**验收**：同一个提示词走两条路，落库候选的 `lights` / `globals` 逐字段相同；
`ops` 允许不同（审计影子）。

### 第三梯队：spec 与 feedback

1. spec 同第二梯队做法（搬 `generate_spec` 的调用点 `:168`）。
2. feedback 按 §3.6 的走法 A。

**验收**：完整跑一轮「提交 → 出 Spec → 出 6 候选 → 渲染 → 判断 → 迭代」，
前端全程不依赖服务端的任何规划逻辑。

---

## 6. 风险与对策

| # | 风险 | 对策 |
|---|---|---|
| 1 | **计费口径分叉** | 网关与三条接缝**共用** `run_seam_inference`，不复制。测试断言网关路径落下的 `seedance2::task` 行 `type_=inference` / `user_id_=operator` / `text_prompt_=input_text` |
| 2 | **schema 漂移** | 前端从 `/prompt_assets` 读，不自己写。服务端内置 schema 是唯一真源 |
| 3 | **`ops` 只带不带 `lights`** | §3.2 第 4 条闸门硬拒 400 |
| 4 | **越权刷量** | `operator_id` 只从 JWT 取、**不读 body**（§3.1）—— 这一条靠「不读」而不是「校验后拒绝」来保证，比 403 判定更彻底 |
| 5 | **反馈质量静默下降** | §3.6 显式决策走法 A 或 B，不允许「删了 `structure_feedback` 调用但没人在前端补上」 |
| 6 | **模板搬迁走样** | 逐字比对：`/prompt_assets` 返回的 `instructions` 与搬迁前 `build_*_instructions()` 的输出必须**字节相同** |
| 7 | **枚举静默落值** | 网关校验沿用 `json_val` 的 `magic_enum::enum_cast`，**不得**改用 `NLOHMANN_JSON_SERIALIZE_ENUM` 裸 `get<>`（`ai_light_json_validation.h` 顶部已有约定） |

---

## 7. 待实测确认（不阻塞开工）

1. **`seam_inference_result_t` 要不要带任务 id**：现在它只有 `json_` + 三项 token 计数
   （`ai_light_seam_common.h:130-135`）。若前端需要「这一次花了多少 / 哪个任务」，
   要改 `seam_inference_result_t` **并**看 `run_inference_task` 能否回传 `task.uuid_id_`。
   —— **第一梯队不阻塞**：先不带，前端的计费展示可以后加。
2. **网关要不要校验 session 归属**：`load_detail_checked()`（`ai_light.cpp:185`）做
   `check_project_access`。§3.1 已按同口径写，但要确认前端在 spec 阶段就已有 `session_id`。
3. **`/prompt_assets` 的缓存策略**：模板是编译期常量，可加 `Cache-Control`，
   但前端调试期可能希望不缓存 —— 建议先不加头，留待实测。
4. **`seam_kind` 枚举是否已存在**：§3.1 用了 `json_val::require_enum<seam_kind>`。
   若工程里没有这个枚举（现在 `run_seam_inference` 收的是 `std::string_view in_seam`），
   则在 `ai_light_gateway.h` 里新增一个三值的 `enum class seam_kind { spec, candidates, feedback }`
   + `seam_name_of()`，**不要**复用字符串字面量散落各处。

---

## 附：本文与其他文档的对应

| 本文 | 上游文档 |
|---|---|
| §1 现状盘点 | [ue_ai_lighting_plan.md](ue_ai_lighting_plan.md) §15.7/§15.8/§15.9 |
| §2 目标形态、§3.1 网关 | [ue_ai_lighting_role_downgrade.md](ue_ai_lighting_role_downgrade.md) §3.3 |
| §3.2 闸门四条判据 | 同上 §3.5 + [ue_ai_lighting_frontend.md](ue_ai_lighting_frontend.md) §2.1 |
| §3.4 搬迁清单 | 同上 §4.2 |
| §3.6 反馈通道 | 同上 §5 第一梯队 |
| `ops` 路线 A（决定 D-58） | [ue_ai_lighting_frontend.md](ue_ai_lighting_frontend.md) §2 第 7 条 / §2.1 |
