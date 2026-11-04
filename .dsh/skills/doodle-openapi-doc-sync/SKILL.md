---
name: doodle-openapi-doc-sync
description: 在 Doodle 仓库同步 docs/api 下的 OpenAPI 文档——后端 HTTP 路由（新增/删除/迁移 URL）、结构体或响应字段（新增 kmt_key、canvas_ids、person_links、backend 等）、同一路径的 get 与 post 返回同一结构体、列表响应体整体改形（裸 array 改为 {data,count,sum} 对象）、查询过滤参数（新增 in query 的 backend / task_type / time_beg / time_end / episode）、枚举取值（如 TaskBackend 加 kmt_image）、或按既有 .cpp/.h 行号与 git diff 核对，或把（UE 插件 / 后端）实现代码里的运行时行为逻辑（如 DoodleLightingExecutor.cpp 的 ApplyCandidateToWorld 灯光方案落地步骤）提取成描述性说明写进既有分片 description；当用户说“检查这几个 .cpp 的 git 改动并同步文档”“根据 git diff 同步 docs/api”“把 ai_studio 的字段更改同步到 docs/api/openapi.yaml”“路由改了同步文档”“新增 backend 查询参数同步文档”“补齐 time_beg/time_end 查询参数”“列表响应改成 data/count/sum 同步文档”“新增后端枚举值同步文档”“新增了 task_backend backend_{task_backend::seedance2} 同步文档”“get 与 post 让两个 URL 返回结构一致”“openapi 引用校验”“把这段实现逻辑提取为文档写进这个 URL 的 OpenAPI 描述里”，或直接贴出 src\doodle_lib\http_method 的 .cpp / .h / reg.h 行号范围与 UE 插件 .cpp 函数（如 task.cpp:610-722、ai_light.h:109-117、DoodleLightingExecutor.cpp:636-677 的 ApplyCandidateToWorld）要求同步文档时使用。
version: 8
---

# Doodle OpenAPI 文档同步（git 改动 / 行号入口 + 路由 + 字段 + 查询参数/枚举 + 响应体改形 + 多方法一致性 + 实现逻辑说明）

`docs/api/` 是接口文档的唯一来源，结构是 `openapi.yaml`（`paths` 里每条只是 `$ref: paths/xxx.yaml`）+ `paths/*.yaml` 分片 + `components/schemas/*.yaml` 分片。本 skill 覆盖七类改动，都要“先全量定位、再改文件、最后校验”：

- **G. git 改动入口**：用户只给 .cpp 文件（或目录），要求“按 git 更改同步文档”，先划定改动范围再分流到 A–E。
- **A. 路由级**：新增 / 删除 / 迁移 URL（`git mv` 改名、去掉路径层级参数）。
- **B. 字段级**：后端结构体或响应新增/改名一个字段，需要同步可写 schema、响应 schema 描述、以及 path 分片里硬编码的字段清单。若新增字段是枚举型（如 `task_backend backend_`），还要决定“复用既有枚举 schema 还是内联手抄”，并把代码里的默认值与可选性写进 description。
- **C. 查询参数级 / 枚举取值级**：给 GET 等接口新增 `in: query` 过滤参数（可能一次缺多个，如 `backend`、`time_beg`、`time_end`、`episode`），或给枚举 schema（如 `TaskBackend.yaml`）追加取值，需要同步分片 `parameters`、枚举 schema 的 `enum` 与各取值 `description`、分片 `description` 里硬编码的过滤字段与“非法枚举值”清单。
- **D. 响应体改形级**：整个响应体形状变了（如列表接口从裸 `type: array` 改为 `{data, count, sum}` 对象），需要新建组合 schema、把分片 `responses.200` 的 `$ref` 改指新 schema，并核对 `description` 里对 `size` / `offset` 等参数作用范围的硬编码说明。
- **E. 多方法一致性级**：同一分片下 `get` 与 `post`（或 `put` / `delete`）共用一条 URL、且现在要返回同一个结构体时，要把两个方法的 `responses` 统一指向同一个 schema，并回读聚合读模型 schema 核对字段不漂移。
- **F. 实现逻辑说明级**：用户给的不是“结构变了”，而是“某段实现 / UE 插件代码怎么执行”的运行时逻辑，要求写进既有分片的 `description`（如“把 `DoodleLightingExecutor.cpp:636-677` 的灯光方案应用方式提取为文档，放到 `/api/doodle/ai/light/session/{}/iterate` 的描述中”）。本类**不改 `$ref`、不加字段、不改 `parameters`**，只写描述性说明，且必须先把口语 URL 反查到真实分片（见 F 第 1–3 步）。

每类都要用 `pyyaml` 校验；`build/tmp/check_openapi.py` **在本工程已存在（已实测 `Test-Path` 为 True）**，先探测再运行，只有确认不存在才按 F.1 原样重建。本工作区里 `git mv`、`Remove-Item` 常被沙箱拒绝：迁移只能走“write 新建 + edit 旧文件 + 改 $ref”，删除旧分片也只能写占位说明。`git status` / `git diff` **是可用的**（G.1 就是靠它们），被拒绝时不要反复重试同一条命令。

分节编号沿用下表（A–E 为格式类改动，F 为“实现逻辑说明”改动、F.0 为校验）：

| 节 | 内容 |
| --- | --- |
| G | 从 git 改动出发 |
| 0 | 从代码行号出发 |
| A | 路由级 |
| B | 字段级 |
| C | 查询参数级 / 枚举取值级 |
| D | 响应体改形级 |
| E | 多方法一致性 |
| F | 从实现代码提取行为逻辑写进既有分片 description |
| V | 校验（所有类型都做，必须贴真实输出） |

## Steps

### G. 从 git 改动出发（用户只给 .cpp 文件或目录时，先做这一步）

1. **先划定范围，不要一上来读整份文件**。在仓库根目录跑：
   ```powershell
   git diff --stat -- <文件1> <文件2>; git status --porcelain
   ```
   `--stat` 给出每个文件的增删行数，`git status --porcelain` 给出改动文件全集（含未跟踪的 `??` 新文件）。若某个文件在 `--stat` 里没出现，再看 `git status --porcelain` 是否把它列为 `??` 或 `M`，**不要只凭空 diff 输出断言“无改动”**。
2. **读完整 diff**：`git --no-pager diff -- <文件1> <文件2>`。带分页器的 `git diff` 在沙箱里会卡住，一律加 `--no-pager`。
3. **按 `@@` 函数头定位改动落在哪个函数**（diff 的 hunk 头通常带 `struct xxx` / `void xxx::get()` 一类上下文行），得到「文件:函数 → 语义」清单。
4. **区分格式化噪音与语义改动**，逐 hunk 判定：
   - 纯 `clang-format` 重排：只是 `update(...)` 参数换行、`json{...}` 块位置移动、`delete_` 之类超长调用断行，键名与取值集合不变。这类对文档**无影响**，但在最终报告里要**显式列为“纯 clang-format 重排”**，不要静默省略。
   - 语义改动：新增 `j["xxx"] = ...` 键、新增 `if (j.contains("xxx"))` 分支、新增/改动注册宏或 URL、结构体新增成员。这些才需要同步文档。
5. **新键要反查文档是否缺失**：对每个新键（如 `ji_du2`）和新类名，在 `docs/api` 上 `esearch` 一次（如 `pattern=ji_du2|sequences_with_tasks path=E:\Doodle\docs\api content=true`）。`total: 0` 就是“文档缺失”，需要新建 schema 或补字段；再用更宽的关键字（如 `with-tasks`）确认该 URL 的分片是否已存在——已存在则**不加 `$ref`**，只改分片与 schema。
6. 拿着这份清单走 Step 0（用类名反查真实注册 URL）与 A–F 对应分支；报告里保留「git 改动 → 代码位置 → 注册 URL → 生效文档 → 是否新增 $ref」的完整链条。

### 0. 从代码行号出发（用户给了 .cpp / .h / reg.h 路径 + 行号，或其口语 URL 时）

1. `read` 用户给的行号范围：
   - `.cpp` 里的 `DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(iteration_submit_route, get)`：第二个实参就是 HTTP 方法（`get` / `post` / `put` / `delete`）。
   - `.h` 里的声明区（如 `ai_light.h:109-117`）：拿到类名与 `DOODLE_HTTP_FUN` / `DOODLE_HTTP_ROUTE` 宏里的 URL 片段，**仍然要按第 2 步反查注册表**，头文件里的 URL 只是声明。
   - `reg.h` 里的路由声明块：读出类名与 URL，再到注册表确认 `reg_c<类名>()` / `reg_t<>` 真实存在。
2. **用 handler 类名反查真实注册路由**，不要从行号猜 URL：在 `src/doodle_lib/http_method` 的 `kitsu.cpp` 等注册表里搜该类名与 `reg_t<` / `reg_c<`，拿到实际 URL（如 `/api/doodle/ai/light/session/{session_id}/iteration`、`/api/seedance2/task`）。报告里给出「行号 → 类名 → URL → 分片」的对照，**每一项都是读到的真实值**，不允许写“待反查”“待定”一类占位。
3. **同一个类名要搜两次**：先限定在 `src/doodle_lib/http_method/<域>`（如 `kitsu`）目录下定位实现（步骤 5 的 grep 就找到了 `ai_light.cpp:987` 的 `get` 与 `:1008` 的 `post`），再放宽到 `src/doodle_lib/http_method` 甚至 `src/doodle_lib` 才能找到 `kitsu.cpp:240` 的 `.reg_c<iteration_submit_route>()`——`reg_c` 注册点和实现不在同一目录。只用第一次 grep 的结果会漏掉注册点，只有第二次才能写出真实注册 URL。报告里**写出 `reg_c` 所处的文件名与行号**。
4. 由 URL 反推分片文件名（规则见 A.1），再在 `docs/api/openapi.yaml` 上 `grep` 该 URL 片段（如 `ai/light/session`）或分片名，确认它**已被引用**。已存在 = 这是改字段/参数/响应形状/描述说明，**不要在 `openapi.yaml` 里新增 `$ref`**；只有代码里出现全新 URL 才走 A 类。在报告里先写出“实际注册 URL + 对应分片 + 是否新增 $ref”的结论。
5. 需要参照既有分片写法时，用 `esearch` 通配（如 `*ai_light*`、`*seedance2*`）限定在 `docs/api/paths` 下枚举同域分片，挑一个结构最接近的 `read` 作模板。
6. `read` 该分片的完整内容作为改动底稿：逐块确认它是否已经有 `get` / `post` 两段，哪些方法的 `responses` 还指向旧形状，再按 B / C / D / E / F 类分支落地。
7. **代码与文档可能已经一致**：逐个比对后若无差异，结论就写“文档已与代码一致”，**不要为了交差编造改动**；仍然要把 V 节校验跑一遍并在报告里贴出真实输出，用执行结果而非“假设脚本可用”来证明一致。

### A. 路由级同步

1. 用 `grep`（pattern 取路由前缀，如 `canvas`）在 `docs/api/openapi.yaml` 上定位所有相关 `paths` 条目，记录每条 path 对应的 `$ref: paths/....yaml`。分片命名规则是**路径转文件名**：`/api/a/{id}/b` → `api_a_{id}_b.yaml`，参数保留花括号（如 `GET /api/seedance2/task/{task_}` → `paths/api_seedance2_task_{task}.yaml`）。
2. 确定新路径后反推新分片文件名，用 `read` 读取每个旧分片的完整内容与它引用的 schema，作为新文件底稿。
3. 用 `write` 在新文件名下创建分片：保留原 `tags` / `summary` / `responses`，按新路径补齐路径参数（新增层级参数如 `subproject_id` 放在 `canvas_id` 前，`in: path`、`required: true`、类型与同级参数保持一致），`description` 改成新语义，`responses.200` 明确说明“响应是对象而非裸数组”一类形状。
4. 用 `edit` 更新 `docs/api/openapi.yaml`：把旧 path 条目替换为新 path 与对应 `$ref`（或在新位置追加 `$ref: paths/<新分片>.yaml`），保持同一缩进与相邻条目顺序风格。若 `paths` 下引用的 schema 也要换，一并改分片里的 `$ref`。
5. 删除 / 迁移旧分片时，旧文件不删（删不掉）：用 `edit` 把旧文件内容整体替换为两行迁移说明，例如：
   ```yaml
   # 已迁移至 api_seedance2_subproject_{subproject_id}_canvas.yaml
   # 画布现在必须归属于子项目：/api/seedance2/subproject/{subproject_id}/canvas
   ```
   接口被删除而非迁移时，同样写成 `# 已删除：对应硬删` 之类的注释占位，让旧文件不再是有效 YAML 定义。但**必须在最终报告里明确写出“这些旧文件需你手工 `git rm`”，不要静默保留**。从 `openapi.yaml` 里删掉 `$ref` 条目这一步必须真的做，否则 `check_openapi.py` 仍会检查旧分片。
6. 若有多个分片的同一参数块（如 `- name: subproject_id`）需要统一删除，不要手改逐个冒风险：写一个一次性 python 脚本（或逐文件 `edit`），处理后在 `docs/api/paths` 上 `grep 'name: subproject_id'` 确认目标分片无残留，再逐个 `yaml.safe_load` 确认仍可解析。
7. 组合 schema（如任务详情 `TaskDetail`）优先用 `allOf` 复用既有响应 schema，再追加关联对象，不要内联手抄主 schema 的字段：
   ```yaml
   allOf:
     - $ref: ./Task.yaml
     - type: object
       properties:
         entity:
           $ref: ./Entity.yaml
           description: <外键来源，见下条>
   ```
   关联对象的**外键必须读实现确认**（如 `entity` 取任务行里的 `ai_episode_id` 还是 `entity_id`），不凭字段名猜测；`read` 对应 `.cpp` 实现里真正做 `find` / `get` 的那一行再写，并在 `description` 里写明取值来源（文件名、字段名、以及“取该外键对应的行”这一逻辑）。

### B. 字段级同步

1. **先定位，再动手**：用 `grep`（pattern 取字段名，如 `apipy_key`、`kmt_key`、`backend`）在 `docs/api` 上**全量搜索**，一次性列出所有出现该字段的分片文件。字段级改动通常不止一处。
2. 再 `grep`（pattern 取业务域，如 `AI工作室|ai_studio`）在 `docs/api/openapi.yaml` 上确认该业务域的 `paths` `$ref` 与 `tags` 列表，检查是否还有没被分片覆盖的引用。
3. `read` 读后端源头确认语义。**结构体路径别猜**：实体常定义在 `src/doodle_core/metadata/<域>/<x>.h`（如 `src/doodle_core/metadata/seedance2/task.h`、`src/doodle_core/metadata/ai_light/iteration_view.h`），`src/doodle_lib/metadata/...` 可能根本不存在（`grep` 会报 `exit 2: os error 2`）。先从代码里 include 的头文件名或 `esearch` 的文件名搜索定位，再用 `include=*task*.h` 一类过滤在正确目录上 `grep` 字段名。读 `put_property_list()`、`from_json` / `to_json` 的读取方式（是否 `if (j.contains("xxx"))`、成员默认值是什么）、返回体应取哪些字段（密钥类字段是否只写不读）。
4. 按三处联动改（缺一处文档就自相矛盾）：
   - **可写 schema**（如 `components/schemas/AiStudioWrite.yaml`）：新增字段的 `type` 与 `description`，放在同类字段后面。POST 与 PUT 共用的 schema，新字段必须在这里可写。
   - **响应 schema 描述**（如 `components/schemas/AiStudio.yaml`）：若描述里硬编码了“响应中不包含 `a` / `b` / `c`”这类密钥清单，把新字段名补进去。响应里实际带出该字段时，在 `properties` 里加属性。
   - **path 分片里的字段清单**：分片的 `description` 常写成“可写字段与 `xxx::put_property_list()` 一致：`name`、`color`……”，硬编码清单，必须同步追加，否则文档与 schema 互相矛盾。
5. **后端差异说明类文字也要全量对齐**：当某个请求体/响应形状被多个后端复用（如 `TransferStationTaskDataRequest.yaml` 原写“只有 transfer_station 使用”，后来 `kmt_image` / `apipy_image` 也复用），`grep` 该 schema 名与「只有 / 仅 / 使用该形状」等措辞，把所有列出后端清单的位置（含枚举 schema 的 `description`、后端对照表行、path 分片 `description`）改成包含全部复用后端的统一表述，不留旧说法。
6. **新增枚举型字段时复用既有枚举 schema**：字段类型是既有枚举（如代码里的 `task_backend`，文档里已有 `components/schemas/TaskBackend.yaml`）就 `$ref` 复用，**不要内联手抄 `type: string` + `enum` 值列表**，否则枚举一改文档就漂移：
   - 先 `grep` 该枚举 schema 名（如 `TaskBackend`）在 `docs/api` 上找既有引用写法，并 `read` 一个典型引用点（如 `components/schemas/Task.yaml` 的 `backend:` 属性）。
   - **以仓库既有惯例为准**：若既有写法是 `backend:\n    $ref: ./TaskBackend.yaml`（`Task.yaml` 就是这样），新字段照抄这个写法；不要因为“枚举 schema 的 `description` 偏任务后端语义”就临时改判成内联。这条描述里讲后端语义、轮询、计费的说明本就是该枚举的权威说明，各引用点共用是对的。
   - 响应 schema 里引用路径是 `./TaskBackend.yaml`；path 分片里引用路径是 `../components/schemas/TaskBackend.yaml`。
   - 只在确实需要针对本字段单独说明（且说明不能写进字段的父级 `description`）时才考虑内联，并在报告里说明理由。
7. **可选字段的默认值与可选性写进 description**：若代码用 `if (j.contains("xxx"))` 读取、成员带默认值（如 `task_backend backend_{task_backend::seedance2}`），说明该字段**可省略**：
   - 在 requestBody schema 里**不要**把这个字段加进 `required` 列表（只列真正必填的字段，如 `model_name`、`resolution`）。
   - 在 path 分片的 `description`（接口级说明）或该字段的 `description` 里写明“省略时默认为 `seedance2`”一类语义，让调用方知道省略后的行为；同时可在接口 `description` 里补一句该字段的业务含义（如“指定该组合所属的任务后端”）。
   - 默认值必须来自代码里真实看到的成员默认值 / `from_json` 分支，不要凭猜测编一个默认值。
8. 用户说“我自己已经改过一部分文档”时：先 `read` 读现有文档，把它当作当前事实，只补不一致的地方，**绝不把用户已做的改名回滚成旧名**。最终报告同时列出代码位置（文件 + 键名）和文档位置（文件 + 属性名）。
9. **投影型响应（只返回 entity 本体部分字段 + 扩展数据）单独建 schema**：当分片响应是“实体部分字段投影 + `entity_episode_extend` 里的某个键”（如 `ji_du2`）时，新建独立响应 schema（如 `components/schemas/SequencesWithTasks.yaml`），`description` 必须写明：
   - 它是 `entity` 本体的**部分字段投影**，并**列出未返回的字段**（如 `code` / `project_id`），调用方不要照 `Entity.yaml` 取全量字段。
   - 扩展部分（如 `entity_episode_extend.yaml`）只在请求体带对应键时整体写入，并指向写入它的接口（如 `PUT /api/data/entities/{id}`）。
   - 扩展 schema 的 `description` 同样写清“仅在请求体含该键时整体写入该表，写入入口是 `PUT /api/data/entities/{id}`”。
10. **可选字段的序列化语义先 grep 确认再写**：代码里出现 `j["frame_in"] = p.frame_in_ ? nlohmann::json{} = p.frame_in_ : nlohmann::json::object();` 这类写法时，`grep` 一次 `nlohmann::json\{\} =`（限定该 .cpp）确认这是“有值则原样写出、无值写空对象”，再决定 description 怎么写；不要凭直觉写成 `null`。

### C. 查询参数级 / 枚举取值级同步

1. **先全量定位**：用 `grep` 在 `docs/api` 上搜枚举 schema 名（如 `TaskBackend`）与要新增的参数名（如 `backend`），拿到：引用该枚举的 `components/schemas/*.yaml`（响应 schema）、各 `paths/*.yaml` 里 `$ref` 到它的位置、以及分片 `description` 里硬编码列出的过滤字段 / 枚举值清单。
2. `read` 后端源头确认语义：查询参数名、是否能空、对应响应字段名（如 `backend` 过滤响应里的 `backend` 字段）、新枚举值在代码里代表什么（同步还是异步、计费方式、是否已下线）。分片里现有参数缺哪些再看一遍 handler 的 `get_param` 一类调用——**一次可能缺好几个**（如 `time_beg` / `time_end` / `episode`），别只补用户点名的那一个。
3. 改分片 `parameters`：在同类参数（如 `task_type`）后面追加 `- name: backend` / `in: query` / `required: false`，`schema` 用 `$ref: ../components/schemas/TaskBackend.yaml` 复用枚举而不是重抄一遍值，`description` 写“按任务后端过滤（seedance2 / transfer_station / ...）”。时间范围类参数写明格式（`time_beg` / `time_end` 的日期或时间格式）与是否闭区间，取值必须来自代码。
4. 改枚举 schema（如 `components/schemas/TaskBackend.yaml`）：在 `enum` **末尾按既有顺序追加**新值（如 `kmt_image`），不要打乱原有值顺序；同时在 `description` 里补该取值的中文语义（图片中转站、异步任务、轮询地址、计费方式；已下线的写“已下线，仅为存量任务兼容保留”）。
5. 改分片 `description` 里硬编码的约束清单，两处都要同步：过滤语义行（“`task_type` 过滤响应中的 `type`，`backend` 过滤响应中的 `backend`”）与非法值清单（“非法枚举值（`status` / `task_type` / `backend`）、非法 UUID……会导致请求失败”）。
6. 若该枚举还被别的 schema（如 `Task.yaml`）引用，顺着 `$ref` 检查这些 schema 的描述里是否也列了取值清单，一并核对；不要再新建一份枚举副本。

### D. 响应体改形级同步（裸 array → 组合对象）

1. **先比对形状**：`read` 代码里实际写出的响应体（如 `j["data"] = ...; j["count"] = ...; j["sum"] = ...;`）与分片 `responses.200.content.application/json.schema`。若分片仍是 `type: array` + `items: $ref: ./X.yaml`，就是整体改形，不是加字段。
2. **新建组合 schema**：`write` 到 `docs/api/components/schemas/<X>ListPage.yaml`（命名 `./TaskListPage.yaml`，与既有 schema 命名风格一致），内容为：
   ```yaml
   type: object
   properties:
     data:
       type: array
       description: 当前页列表，按 `created_at` 倒序
       items:
         $ref: ./Task.yaml
     count:
       type: integer
       format: int64
       description: 满足过滤条件的总数，不受 `size` / `offset` 影响
     sum:
       type: integer
       format: int64
       nullable: true
       description: <代码里真实含义，如 completion_tokens 之和>
   ```
   字段名、类型（`int64`）、可空性（`nullable: true`）都必须来自代码；`items.$ref` 指向**已存在**的 `components/schemas/*.yaml`（响应 schema 内用 `./X.yaml`）。
3. **改分片 `$ref`**：把 `paths/<分片>.yaml` 的 `responses.200.content.application/json.schema.$ref` 从 `../components/schemas/Task.yaml` 改为 `../components/schemas/TaskListPage.yaml`；同一 `$ref` 若在 2xx 其他状态码或 `default` 里重复出现，一起改。
4. **改分片 `description`**：把分片顶部 `description` 里关于响应体的说明改写为对象语义（如“响应体是对象而非裸数组，`size` / `offset` 只作用于 `data`”），并列出 `data` / `count` / `sum` 各自的含义。
5. 只在“原 schema 需要复用给其他接口”时才考虑 `allOf` 包裹原 schema；整个响应形状都变了就新建组合 schema，不要给列表接口继续挂裸 array。

### E. 多方法一致性同步（同一 URL 的 get / post 返回同一结构体）

1. **先确认两个方法都存在**：`grep` 类名在实现目录下（如 `src/doodle_lib/http_method/kitsu`），确认 `DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(<类名>, get)` 与 `(<类名>, post)` 各自的行号；报告里写出这两个行号，别只改看到的那一个。
2. `read` 两个实现各自写出的 JSON 键，确认它们现在真的返回同一形状、同一键名；若实现里有差异，先在报告里点明差异，再决定 schema 取并集还是以其中一个为准。
3. 在分的**读模型结构体**定义（如 `src/doodle_core/metadata/ai_light/iteration_view.h` 的 `ai_light_current_iteration_t`）中确认字段清单、类型与可空性，作为新 schema 的权威来源。
4. `write` 新 schema（如 `components/schemas/AiLightIterationView.yaml`），`description` 里写明“`<URL>` 的 `get` 与 `post` 都返回本形状”，并注明它是哪个结构体的精简子集、刻意不带哪些字段。
5. `write`（整体重写）该分片，让 **`get` 与 `post` 的 `responses.200` 都 `$ref` 同一个 `../components/schemas/<新schema>.yaml`**；其他方法的 `responses`、`tags` / `summary` / 路径参数保持原样。若两个方法在分片里原本合并写在同一个 `responses` 块下，拆分时不要重复定义 `tags` / `parameters`。
6. **交叉核对聚合读模型**：`read` 同类聚合读模型 schema（如 `components/schemas/AiLightSessionDetail.yaml`，其 `iterations` 数组项多半就是同一结构体），逐字段比对属性名、类型、`nullable`；不一致时以代码为准改新 schema，并在报告里说明“聚合读模型那边是否需要跟着改”。
7. 同一 URL 还有其他方法（`put` / `delete`）时同样检查：要么也指向同一 schema，要么在分片 `description` 里写明它们的响应不同。
8. 若本次同时要给这两个方法补“实现怎么执行”的叙述，在 E 完成后接着走 F，**get 与 post 两段 description 都要写**（合成后仍各自 `$ref` 同一 schema）。

### F. 实现逻辑说明级（从实现代码提取行为逻辑写进既有分片 description）

前提：用户给的是一段**实现代码的执行逻辑**（UE 插件 `.cpp` 函数、后端 handler 内部流程），要求写进某个既有 URL 的描述里。本类不改 `openapi.yaml` 的 `$ref`、不加 schema 属性、不动 `parameters`、不改 `responses`；`responses` 必须保持改动前的 `$ref` 不变。

1. **别用用户口语的 URL 去搜分片**。用户说 `iterate`，真实分片名却是 `_iteration`。在 `docs/api/paths` 上 `esearch pattern=*iterate*`、`content=/iterate` 与 `grep pattern=iterate` 都返回 0 命中，这是**已知现象，不代表文档缺失**。拿到 0 命中时按第 2 步反推，**不要因此新建分片或断言“文档里没有这个接口”**。
2. **从后端注册路由反推真实 URL 与分片名**：在 `esearch` 上限定 `path=E:\Doodle\src\doodle_lib\http_method`、`pattern=<URL 关键字>`、`content=true`，命中 `.h`（如 `kitsu\ai_light.h`，其中 `// /api/doodle/ai/light/session/{}/iterate` 只是注释里的口语写法）；再 `grep` 对应 `.h` / `.cpp`（如 `ai_light.cpp`）拿到真实类名与实现行号（如 `iteration_submit_route` 在 `ai_light.cpp:988` 的 `get`、`:1007` 的 `post`）。
3. **由真实路由反推分片文件名**（A.1 规则）：`/api/doodle/ai/light/session/{session_id}/iteration` → `docs/api/paths/api_doodle_ai_light_session_{session_id}_iteration.yaml`。`read` 它确认已经存在、且 `get` / `post` 两段都在。报告里写出「口语 URL → 真实注册 URL → 分片文件」的对照与依据（`.h` 注释行、`.cpp` 实现行、`reg_c` 行）。
4. **从实现代码提取逻辑，先定位再读**：用户给的 `.cpp` 行号范围（如 `DoodleLightingExecutor.cpp:636-677` 的 `ApplyCandidateToWorld`）先 `read`；它调用的辅助函数（如 `ResolveLightPlacement` / `ResolveIntensityUnits`）用 `esearch` 限定到该模块目录（如 `path=E:\Doodle\script\uePlug\Doodle\Source`、`content=true`）定位，**不要在整仓上用 `grep` 加 `include=*.cpp` 搜多个函数名——多函数名 `|` 组合 + 扩展名过滤会零命中**（evidence 步骤 19）；退回 `esearch` 限定目录后一次命中（步骤 20），再 `grep` 该文件拿函数定义行号、`read` 全文（如 `DoodleLightingOps.cpp`）拿常量与分支。
5. **只写代码里真实存在的步骤**：把逻辑整理成有序小节（如 `步骤一 …`、坐标/单位/角度如何换算、每步读哪个输入、失败/回退分支是什么、最终落到哪个 API）。数值（如默认距离 `3.0`）、单位换算、回退规则都必须来自读到的源码，不编造；代码里没写的行为不要补。
6. **写进既有分片的 `description`，两段都写**：用 `edit` 在 `get` 与 `post` 的 `description` 末尾各追加一个小节（如 `### 与 UE 侧执行方式的关系` / `### 方案如何实现到关卡`），内容一致或对称（`post` 侧说明“这里落库的候选就是 `ApplyCandidateToWorld` 的输入”）。保留原有内容，**不改 `responses.$ref`**（如仍都指向 `../components/schemas/AiLightIterationView.yaml`）。
7. **写完后回读并修**：`read` 改过的分片，核对叙述与源码一致、Markdown 小节标题在 YAML `|-` 块内缩进正确，再走 V 节校验。

### V. 校验（七类改动都做，且必须贴真实输出）

1. **先确认脚本在不在，再读脚本确认它检什么，最后才跑**：先 `Test-Path E:\Doodle\build\tmp\check_openapi.py` 并 `python --version`、`ls E:\Doodle\build\tmp\*.py`。**本工程 `build/tmp/check_openapi.py` 实际上常常已存在**（evidence 步骤 34 输出 `True`），直接 `read` 它确认检查项（遍历 `openapi.yaml` 的 `paths` 查每个 `$ref` 文件是否存在、再查分片里的 `#/components/schemas/...` 引用）并运行即可。只有 `Test-Path` 为 False 时才 `esearch` 搜 `check_openapi.py`（先 `path=E:\Doodle\build\tmp`，再放宽到 `E:\Doodle`，旧副本可能在 `E:\Doodle_old\build\tmp\check_openapi.py`），`read` 旧副本后 `write` 重建再跑。
   - 运行：`python build/tmp/check_openapi.py`（workdir 为仓库根）。它输出 `total paths: N`、`missing path refs: []`、`missing schema refs: []` 与若干 `parsed OK: ...` 行（evidence 步骤 36：`total paths: 227`，两个列表均空）。
   - 不要用 PowerShell 内联 `python -c` 写带 `$ref` 的完整脚本（`$` 会被吞掉）。
2. 对每个改动过的文件单独做 `yaml.safe_load` 解析校验（含 `openapi.yaml` 本体与新建 schema），例如：
   ```powershell
   python -c "import yaml,io; [yaml.safe_load(io.open(p,encoding='utf-8')) for p in ['docs/api/openapi.yaml','docs/api/paths/api_data_sequences_with-tasks.yaml','docs/api/paths/api_data_entities_{id}.yaml','docs/api/components/schemas/SequencesWithTasks.yaml','docs/api/components/schemas/EntityEpisodeExtend.yaml']]; print('YAML OK')"
   ```
3. **断言分片结构（不只解析）**：用 pyyaml 读回分片，打印被断言的具体值，证明改动真的落到了目标位置。对 F 类（描述说明）用下面这行的口径（evidence 步骤 37）：
   ```powershell
   python -c "
   import yaml,io
   p='docs/api/paths/api_doodle_ai_light_session_{session_id}_iteration.yaml'
   d=yaml.safe_load(io.open(p,encoding='utf-8'))
   print('methods:', list(d.keys()))
   print('get schema ref:', d['get']['responses']['200']['content']['application/json']['schema'])
   print('post schema ref:', d['post']['responses']['200']['content']['application/json']['schema'])
   print('get desc len:', len(d['get']['description']))
   print('post desc len:', len(d['post']['description']))
   print('get mentions 步骤一:', '步骤一' in d['get']['description'])
   print('post mentions ApplyCandidateToWorld:', 'ApplyCandidateToWorld' in d['post']['description'])"
   ```
   期望：`methods` 含 `get` / `post`、两边 schema `$ref` 完全相同、两段 description 非空且都含新写的小节关键字。字段/参数类改动则改为打印 `properties` 键清单与 `parameters` 里的 `name`（如 `ji_du2`、`backend`）。多行 `python -c` 在 pwsh 里用双引号包裹、脚本内用单引号，必要时写成 `build/tmp/` 下的一次性脚本。
4. 用 `glob`（如 `docs/api/paths/*canvas*`）或 `grep` 核对分片清单与 `openapi.yaml` 的 `$ref` 一一对应，并确认 `openapi.yaml` 里已无对旧分片的引用；新建的组合 schema 用 `grep` 其文件名确认被至少一个分片引用。
5. **核对改动边界**：`git status --porcelain -- docs/api`，确认只改了预期文件（`M` 已改分片、`??` 新建 schema），没有越界改动；报告里列出这些状态。

## Pitfalls

- **把用户给的 git 入口当成“没给入口”**：用户说“检查这两个 .cpp 的 git 更改并同步文档”时，起点是 `git diff --stat` + `git status --porcelain` + `git --no-pager diff`，不是行号。直接 `read` 整份 .cpp 会淹没在格式化噪音里，也无法在报告里说清哪些 hunk 是 clang-format。
- **`git diff` 不带 `--no-pager`**：沙箱里会卡在分页器。一律 `git --no-pager diff -- <files>`。
- **把格式化噪音当语义改动（或反之）**：`update(...)` 换行、`json{...}` 块移位只是 clang-format；只有新增键、新增分支、新增路由、新增成员才是语义改动。报告里两类都要写明，语义改动对应到文档，格式化部分标“对文档无影响”。
- **把用户口语 URL 直接当分片名去搜，0 命中就断言文档缺失**：`iterate` vs `_iteration` 是已知情况——`docs/api/paths` 上 `*iterate*`、`content:/iterate`、`grep iterate` 全部 0 命中（evidence 步骤 9-11），但分片 `api_doodle_ai_light_session_{session_id}_iteration.yaml` 一直都在。退回 `src/doodle_lib/http_method` 找注册路由（`ai_light.h` 注释行 / `ai_light.cpp` 的 `iteration_submit_route` 实现 / `kitsu.cpp` 的 `reg_c`），由真实 URL 反推文件名再 `read`。
- **把“把实现逻辑提取为文档”当成六类之外的私活**：这是第七类 **F**，照样要按 F 定位真实分片、读源码、写 get/post 两段 description，并在报告里给出依据；不要因为“没有字段/路由变化”就跳过校验或自创一个新分片。
- **多函数名 `grep` + `include=*.cpp` 零命中当“函数不存在”**：evidence 步骤 19 用 `ResolveLightPlacement|ResolveSourceRadiusCm|...` + `include=*.cpp` 在 `src` 上无匹配，步骤 20 用 `esearch` 限定 `path=E:\Doodle\script\uePlug\Doodle\Source` 一次就命中 3 处。UE 插件源码优先 `esearch` + `path` 限定目录，命中后再 `grep` 单文件取行号。
- **把 esearch 的 0 命中当成“符号不存在”**：本工作区 Everything 索引陈旧是常态（同一次任务里 `iteration_submit_route` 在 `kitsu.cpp` 上 `total: 0`、`ai/light/session/...` 片段却能在 `openapi.yaml` 上命中）。0 命中只说明该范围内索引没刷。立刻退回 `grep`，并且**从窄到宽换两个目录**：先 `path=src/doodle_lib/http_method/kitsu` + `include=*.cpp` 找实现，再 `path=src/doodle_lib`（pattern 加 `|` 备选写法，如 `iteration_submit_route|ai/light/session/\{session_id_\}/iteration`）找 `reg_c<>` 注册点。只有两轮都空、或 `grep` 报 `exit 2` 路径不存在，才能说该符号不在代码里。而且在 `docs/api` 上 0 命中恰恰是“文档缺失”的**有效信号**，要立刻新建 schema / 补字段。
- **从行号猜 URL / 猜结构体路径**：行号只告诉你 handler 在哪，真实注册 URL 要去注册表（`kitsu.cpp` 的 `reg_t<>` / `reg_c<>`）里按类名反查；`.h` 里的宏声明也只是声明。结构体在 `src/doodle_core/metadata/...`，`src/doodle_lib/metadata/seedance2` 这类路径不存在时 `grep` 会直接 `exit 2`，别在同一路径上反复重试，换成 include 头文件名 / `include=*x*.h` 过滤再搜。
- **已知路径只是缺引用就去加 `$ref`**：分片已存在说明这是改字段/参数/响应形状/描述说明，`openapi.yaml` 不动，报告里要明确说“不是新路径”。
- **`git mv` / `Remove-Item` 会被沙箱拒绝**（`ResourceUnavailable: ... 拒绝访问`）。别重试、别换 `cmd /c del`，按 A.3–A.5 的 write + edit 路径走。
- **只改 `openapi.yaml` 不改分片**：`$ref` 指向不存在的文件名，文档校验必挂。新建分片、改 `$ref`、处理旧分片必须一起做。
- **字段级只改一处**：只往 Write schema 加字段、忘了响应 schema 的“不含密钥”描述和 path 分片的“可写字段清单”，文档会自相矛盾。动手前先全量 `grep` 字段名拿到完整清单。
- **写了描述不核对就交差**：`write` / `edit` 出来的 description 常把项目过滤、外包账号、默认值、步骤顺序写反，**成稿后立刻回读并 `edit` 修正**（evidence 里就是 write 后马上 edit 才把过滤语义写对），再进入校验。
- **描述里写了代码没有的行为**：F 类的步骤、数值、回退规则必须逐条对应读过的源码（如 `DoodleLightingOps.cpp` 里的 `GDefaultDistanceM = 3.0`）；没有源码支撑的“看起来合理”的说法不要写。
- **只给 get 写说明、post 不写**：同一 URL 的 `get` 与 `post` 共用一份数据形状（evidence 步骤 31-33 两段都补了），只写一段会让两个方法的文档口径分裂；两段都补，且都不改 `responses.$ref`。
- **投影型响应漏写“未返回的字段”**：只写“返回 entity 部分字段”而不列 `code` / `project_id`，调用方会照 `Entity.yaml` 取全量字段导致空值；扩展数据（如 `ji_du2`）也要写清写入入口是哪个 PUT。
- **复用形状的后端清单只改一处**：`TransferStationTaskDataRequest.yaml` 说“只有 transfer_station”，而分片后端对照表没加 `kmt_image` 行（含 `/v1/images/generations`、`/v1/images/edits` 与计费方式），文档就会前后打架；先 `grep` 后端清单再逐处改，只留一种说法。
- **新增枚举型字段时在 `$ref` 与内联之间来回改判**：先看既有引用点（`Task.yaml` 的 `backend` 就是 `$ref: ./TaskBackend.yaml`），照抄仓库惯例落地一次，别“先 $ref → 嫌描述不贴切改内联 → 再改回 $ref”。枚举 schema 的 `description` 就是各引用点共用的权威说明，不算跑偏。
- **内联手抄枚举值**：无论是新字段属性还是查询参数 `schema`，都 `$ref` 到枚举 schema，避免值与 schema 漂移；也不要新建第二份 `TaskBackend` 副本。
- **可选字段没写默认值语义 / 误加进 `required`**：`if (j.contains(...))` + 成员默认值 = 可省略字段，必须进 `required` 之外，并把“省略时默认为 xxx”写进接口 description；只加 `properties` 属性会让调用方不知道省略后的行为。
- **查询参数级只加参数块**：只往分片 `parameters` 里塞 `backend`，却没在枚举 schema 里加值、没改分片 `description` 的过滤语义与“非法枚举值”清单，是最容易漏的联调点；三处一起改。也别只补用户点名的那一个参数：handler 里 `time_beg` / `time_end` / `episode` 等一起被读取时，分片缺的都要补齐。
- **响应体改形只换 `$ref` 不建 schema**：把分片 `$ref` 改成 `../components/schemas/TaskListPage.yaml` 却没 `write` 该文件，`check_openapi.py` 会直接报 missing schema refs；新建文件里的 `items.$ref` 也要指向真实存在的 `X.yaml`。
- **改形后没改描述**：分片 `description` 还写着“返回数组”，或没说明 `size` / `offset` 只作用于 `data`、`count` / `sum` 不受分页影响，调用方会误用。
- **只统一了一个方法的 responses**：同一 URL 的 `get` 改成新 schema 而 `post` 还挂在旧 schema（或反过来）时，用户看到的仍是“两个 URL 结构不一致”。改完必须 `read` 整个分片，确认每个方法各自的 `responses.200.$ref` 是同一个文件。
- **改了两处形状却没回读聚合 schema**：新 schema 的 `iteration` 字段若与 `AiLightSessionDetail.yaml` 的 `iterations` 项类型不同（如一处 `integer` 一处 `int64`/`nullable`），同一数据在文档里出现两种形状。改完顺手 `read` 聚合 schema 比对，**并把逐字段比对结果写进报告**，不能只写“已核对”。
- **没探测就重建校验脚本 / 没建脚本就说校验通过 / 用占位符充当执行结果**：`build/tmp/check_openapi.py` **在本工程已存在**，先用 `Test-Path` + `ls build/tmp/*.py` 探测（evidence 步骤 34 输出 `True`）。别跳过这一步，也别写 `missing path refs: []`（假设值）充当输出——报告里必须贴出真实运行结果（`total paths: N`、两个缺失列表、`YAML OK`），并且先 `read` 脚本确认它真的检查这些项。同样地，报告里**不允许出现 `<类名>`、`待定`、`需反查` 这类占位符**：每个类名、URL、行号、文件名都要是实际读到的值。
- **只解析不断言**：`yaml.safe_load` 通过只说明 YAML 语法没坏，不代表新字段落到了 requestBody / responses 里、或描述小节真的写进去了。按 V.3 把 `properties` 键清单、`responses.200.schema`、两段 description 的长度与关键字打印出来核对。
- **枚举值追加打乱顺序 / 漏写语义**：新值要接在 `enum` 末尾保持既有顺序，且枚举 schema 的 `description` 里必须写明它的中文语义（含异步/同步、是否已下线），只在 `enum` 里加一个裸字符串不算同步。
- **新路径多出的层级参数漏写**：`subproject_id` 之类必须同时出现在每个受影响分片的 `parameters` 里；反过来，路径缩短后要批量清掉则用脚本 + `grep` 验证，别靠肉眼。
- **文件名花括号**：参数名要保留 `{...}`（如 `api_..._{subproject_id}_canvas.yaml`），不要转成别的占位符。
- **用户先改过的文档不是错误**：不要把 `canvas` 回滚成 `canvas_ids` 之类，只修正真正不一致的地方。
- **不要碰 `external/` 与 `vcpkg/`**；文档改动只限 `docs/api/`，不要顺手改未涉及的其他 path/schema 文件。
- **不要跑构建/测试来“验证文档”**：本仓库禁止测试，验证只靠文档解析与 `$ref` 完整性脚本。
- **不要 `git commit`**：提交一律由用户执行，只报告改动与未提交状态；也不要建议用户如何去提交。

## Verification

- 在仓库根跑 `python build/tmp/check_openapi.py`：输出 `total paths: N`（如 227 条或当前实际条数），`missing path refs` 与 `missing schema refs` 均为空列表；只有 `Test-Path` 为 False 时才按 V.1 从 `E:\Doodle_old\build\tmp` 重建后运行。
- 对每个改动过的 YAML（含 `openapi.yaml` 与新建 schema）逐个 `yaml.safe_load`，无 parse failure，报告 `YAML OK`。
- 按 V.3 断言分片结构：打印新字段/新 schema 所在位置的键清单与 `responses` 形状，确认与代码一致。
- `git status --porcelain -- docs/api`：只出现预期的 `M` / `??` 条目，报告里列出。
- 从 git 改动出发的任务：报告里有 `git diff --stat` 的增删行数、逐 hunk 的“语义 / 纯 clang-format”判定，以及每个语义改动对应的文档位置。
- 从行号出发的任务：报告里先写出「行号 → 类名 → 实际注册 URL（含 `reg_c` 所在文件行号）→ 对应分片 → 是否新增 `$ref`」的对照，再列字段/参数改动；代码与文档已一致时直接写“文档已与代码一致”，且不乱改文件。
- `grep` 新路由前缀与旧路由前缀：新 path 都有 `$ref` 且格式与相邻条目一致；旧前缀（如 `paths/api_canvas.yaml`）在 `openapi.yaml` 里已无引用。
- 字段级：`grep` 字段名，确认可写 schema 有属性、响应 schema 描述含该名、path 分片字段清单含该名，三处齐全。
- 共享形状的后端清单：`grep` 后端值（如 `kmt_image`、`apipy_image`）在 `TaskBackend.yaml`、`TransferStationTaskDataRequest.yaml` 与相关 path 分片里表述一致，`grep` “只有|仅” 已无残留旧说法。
- 枚举型新字段：`read` 改过的 schema 与分片，确认 `backend` 这类属性写的是 `$ref` 到枚举 schema（响应 `./TaskBackend.yaml`、分片 `../components/schemas/TaskBackend.yaml`），与 `Task.yaml` 既有写法一致；`read` 分片 `description` 确认写了“省略时默认 `seedance2`”一类语义，且该字段不在 `required` 列表里。
- 查询参数 / 枚举级：`read` 改过的分片，确认 `parameters` 里有 `name: backend`（及 `time_beg` / `time_end` 等）且 `schema.$ref` 指向枚举 schema；`read` 枚举 schema，确认新值在 `enum` 末尾、`description` 含其语义；`grep` 分片 `description`，确认过滤字段与“非法枚举值”清单都含新参数。
- 响应体改形 / 投影型响应：`read` 新建或改动的响应 schema，确认 `data.items.$ref` 或投影字段指向原有 schema、`count` / `sum` 的类型与可空性与代码一致，`description` 写明投影未返回的字段；`grep` 新 schema 文件名，确认至少一个分片引用它，且该分片不再指向裸 array。
- 多方法一致性：`read` 整个分片，确认 `get` 与 `post` 的 `responses.200.$ref` 都指向同一 `components/schemas/*.yaml`；`grep` 该 schema 文件名，确认没有别的分片还指向旧形状；`read` 聚合读模型 schema（如 `AiLightSessionDetail.yaml`），逐个比对字段名与类型，差异已修正或在报告里说明。
- 实现逻辑说明（F 类）：报告里有「口语 URL（如 `/iterate`）→ 真实注册 URL → 分片文件」对照及其 `.h` 注释行 / `.cpp` 实现行 / `reg_c` 行依据；`V.3` 输出两段 description 都非空且含新写小节关键字、两段 `responses.200.schema` 相同；`grep` 分片里 `$ref` 与改动前一致，`openapi.yaml` 未新增条目。
- 组合 schema：`read` 新 schema，确认用 `allOf` 复用主 schema（如 `./Task.yaml`），外键 `description` 里写明的取值来源与实现（实际读过的 .cpp 文件与字段）一致。
- `read` 至少一个新分片，确认 `tags` / `summary` / `parameters`（含新增层级参数）/ `responses` 结构完整、YAML 缩进正确。
- 最终报告固定用表格收口（每行一个改动点，无改动也列一行说明），并附校验输出：

  | 代码位置（文件:行/函数） | 实际注册 URL | 生效文档（分片 / schema） | 是否新增 `$ref` | 说明 |
  | --- | --- | --- | --- | --- |
  | `entities.cpp:101-116`（`data_entities::put` 新增 `entity_episode_extend` 分支） | `PUT /api/data/entities/{id_}` | `paths/api_data_entities_{id}.yaml` | 否，路径已存在 | 请求体字段清单追加 `ji_du2` |
  | `sequences.cpp:18-60, 94-102, 186-189`（`sequences_with_tasks`） | `GET /api/data/sequences/with-tasks` | `paths/api_data_sequences_with-tasks.yaml` + 新建 `components/schemas/SequencesWithTasks.yaml` | 否，路径已存在 | 新建投影型响应 schema |
  | `DoodleLightingExecutor.cpp:636-677`（`ApplyCandidateToWorld`）+ `DoodleLightingOps.cpp` | `/api/doodle/ai/light/session/{session_id}/iteration`（`kitsu\ai_light.h:109` 注释口语写作 `/iterate`） | `paths/api_doodle_ai_light_session_{session_id}_iteration.yaml` 的 `get` / `post` description | 否，路径已存在 | 只补描述小节，`responses` 仍指 `AiLightIterationView` |
  | `entities.cpp` / `sequences.cpp` 其余 diff | — | 无影响 | — | 纯 clang-format 重排（`update(...)` 换行、`json{...}` 块移位） |

  表格下方另列：`check_openapi.py` 的真实输出（total paths 与两个缺失列表）、`YAML OK`、`git status --porcelain -- docs/api` 结果，以及“改动处于未提交状态”；若残留旧分片，明确列出“需手工 `git rm`”。
- TODO: 若团队成员规定旧分片可以随提交一并删除，请在此补充仓库层面约定（现有证据只支持“写迁移说明”的处理方式）。
