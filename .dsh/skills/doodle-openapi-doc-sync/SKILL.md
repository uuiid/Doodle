---
name: doodle-openapi-doc-sync
description: 在 Doodle 仓库改了后端 HTTP 路由（新增/删除/迁移 URL）、结构体或响应字段（新增 kmt_key、canvas_ids、person_links、backend 等）、列表响应体整体改形（裸 array 改为 {data,count,sum} 对象）、查询过滤参数（新增 in query 的 backend / task_type / time_beg / time_end / episode）、或枚举取值（如 TaskBackend 加 kmt_image）后，同步 docs/api 下的 OpenAPI 分片（openapi.yaml 的 paths $ref、paths/*.yaml 的参数与 description 清单、components/schemas/*.yaml 的属性、enum 与组合 schema），并处理新增枚举型字段复用既有枚举 schema、可选字段默认值语义写入 description，最后用 build/tmp/check_openapi.py 与 pyyaml 校验；当用户说“把 ai_studio 的字段更改同步到 docs/api/openapi.yaml”“路由改了同步文档”“新增 backend 查询参数同步文档”“补齐 time_beg/time_end 查询参数”“列表响应改成 data/count/sum 同步文档”“新增后端枚举值同步文档”“新增了 task_backend backend_{task_backend::seedance2} 同步文档”“openapi 引用校验”，或直接贴出 src\doodle_lib\http_method 的 .cpp 行号范围（如 task.cpp:610-722）要求同步相应文档时使用。
version: 5
---

# Doodle OpenAPI 文档同步（路由 + 字段 + 查询参数/枚举 + 响应体改形）

`docs/api/` 是接口文档的唯一来源，结构是 `openapi.yaml`（`paths` 里每条只是 `$ref: paths/xxx.yaml`）+ `paths/*.yaml` 分片 + `components/schemas/*.yaml` 分片。本 skill 覆盖四类改动，都要“先全量定位、再改文件、最后校验”：

- **A. 路由级**：新增 / 删除 / 迁移 URL（`git mv` 改名、去掉路径层级参数）。
- **B. 字段级**：后端结构体或响应新增/改名一个字段，需要同步可写 schema、响应 schema 描述、以及 path 分片里硬编码的字段清单。若新增字段是枚举型（如 `task_backend backend_`），还要决定“复用既有枚举 schema 还是内联手抄”，并把代码里的默认值与可选性写进 description。
- **C. 查询参数级 / 枚举取值级**：给 GET 等接口新增 `in: query` 过滤参数（可能一次缺多个，如 `backend`、`time_beg`、`time_end`、`episode`），或给枚举 schema（如 `TaskBackend.yaml`）追加取值，需要同步分片 `parameters`、枚举 schema 的 `enum` 与各取值 `description`、分片 `description` 里硬编码的过滤字段与“非法枚举值”清单。
- **D. 响应体改形级**：整个响应体形状变了（如列表接口从裸 `type: array` 改为 `{data, count, sum}` 对象），需要新建组合 schema、把分片 `responses.200` 的 `$ref` 改指新 schema，并核对 `description` 里对 `size` / `offset` 等参数作用范围的硬编码说明。

四类都要用 `pyyaml` 校验，能跑时再跑 `build/tmp/check_openapi.py` 查 `$ref` 完整性。本工作区里 `git mv`、`Remove-Item`、`git status` 常被沙箱拒绝：迁移只能走“write 新建 + edit 旧文件 + 改 $ref”，删除旧分片也只能写占位说明。

## Steps

### 0. 从代码行号出发（用户给了 .cpp 路径 + 行号时）

1. `read` 用户给的行号范围（如 `src\doodle_lib\http_method\seedance2\task.cpp:610-722`），确认 handler 名与宏参数：`DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task, get)` 里的第二个实参就是 HTTP 方法（`get` / `post` / `put` / `delete`）。
2. **用 handler 类名反查真实注册路由**，不要从行号猜 URL：`esearch`（或 `grep`）在 `src/doodle_lib/http_method` 的 `kitsu.cpp` 等注册表里搜该类名（如 `seedance2_task`）与 `reg_t<` / `reg_c<`，拿到实际 URL，例如 `/api/seedance2/task`、`/api/seedance2/subproject/{subproject_id}/entity/{entity_id}/task`。
3. 由 URL 反推分片文件名（规则见 A.1），再在 `docs/api/openapi.yaml` 上 `grep` 该分片名确认它**已被引用**。已存在 = 这是改字段/参数，**不要在 `openapi.yaml` 里新增 `$ref`**；只有代码里出现全新 URL 才走 A 类。在报告里先写出“实际注册 URL + 对应分片 + 是否新增 $ref”的结论。
4. `read` 该分片的完整内容作为改动底稿，再按 B / C / D 类分支落地。

### A. 路由级同步

1. 用 `grep`（pattern 取路由前缀，如 `canvas`）在 `docs/api/openapi.yaml` 上定位所有相关 `paths` 条目，记录每条 path 对应的 `$ref: paths/....yaml`。分片命名规则是**路径转文件名**：`/api/a/{id}/b` → `api_a_{id}_b.yaml`，参数保留花括号。
2. 确定新路径后反推新分片文件名。例如 `/api/seedance2/subproject/{subproject_id}/canvas` → `api_seedance2_subproject_{subproject_id}_canvas.yaml`。用 `read` 读取每个旧分片的完整内容与它引用的 schema，作为新文件底稿。
3. 用 `write` 在新文件名下创建分片：保留原 `tags` / `summary` / `responses`，按新路径补齐路径参数（新增层级参数如 `subproject_id` 放在 `canvas_id` 前，`in: path`、`required: true`、类型与同级参数保持一致），`description` 改成新语义。
4. 用 `edit` 更新 `docs/api/openapi.yaml`：把旧 path 条目替换为新 path 与对应 `$ref`，保持同一缩进与相邻条目顺序风格。若 `paths` 下引用的 schema 也要换，一并改分片里的 `$ref`。
5. 删除 / 迁移旧分片时，旧文件不删（删不掉）：用 `edit` 把旧文件内容整体替换为两行迁移说明，例如：
   ```yaml
   # 已迁移至 api_seedance2_subproject_{subproject_id}_canvas.yaml
   # 画布现在必须归属于子项目：/api/seedance2/subproject/{subproject_id}/canvas
   ```
   接口被删除而非迁移时，同样写成 `# 已删除：对应硬删` 之类的注释占位，让旧文件不再是有效 YAML 定义。但**必须在最终报告里明确写出“这些旧文件需你手工 `git rm`”，不要静默保留**。
6. 若有多个分片的同一参数块（如 `- name: subproject_id`）需要统一删除，不要手改逐个冒风险：写一个一次性 python 脚本（或逐文件 `edit`），处理后在 `docs/api/paths` 上 `grep 'name: subproject_id'` 确认目标分片无残留，再逐个 `yaml.safe_load` 确认仍可解析。

### B. 字段级同步

1. **先定位，再动手**：用 `grep`（pattern 取字段名，如 `apipy_key`、`kmt_key`、`backend`）在 `docs/api` 上**全量搜索**，一次性列出所有出现该字段的分片文件。字段级改动通常不止一处。
2. 再 `grep`（pattern 取业务域，如 `AI工作室|ai_studio`）在 `docs/api/openapi.yaml` 上确认该业务域的 `paths` `$ref` 与 `tags` 列表，检查是否还有没被分片覆盖的引用。
3. `read` 读后端源头确认语义。**结构体路径别猜**：实体常定义在 `src/doodle_core/metadata/<域>/<x>.h`（如 `src/doodle_core/metadata/seedance2/task.h`），`src/doodle_lib/metadata/...` 可能根本不存在（`grep` 会报 `exit 2: os error 2`）。先从代码里 include 的头文件名或 `esearch` 的文件名搜索定位，再用 `include=*task*.h` 一类过滤在正确目录上 `grep` 字段名。读 `put_property_list()`、`from_json` / `to_json` 的读取方式（是否 `if (j.contains("xxx"))`、成员默认值是什么）、返回体应取哪些字段（密钥类字段是否只写不读）。
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

### C. 查询参数级 / 枚举取值级同步

1. **先全量定位**：用 `grep` 在 `docs/api` 上搜枚举 schema 名（如 `TaskBackend`）与要新增的参数名（如 `backend`），拿到：引用该枚举的 `components/schemas/*.yaml`（响应 schema）、各 `paths/*.yaml` 里 `$ref` 到它的位置、以及分片 `description` 里硬编码列出的过滤字段 / 枚举值清单。
2. `read` 后端源头确认语义：查询参数名、是否能空、对应响应字段名（如 `backend` / `task_type` 过滤的是响应里的 `backend` / `type` 字段）、新枚举值在代码里代表什么（同步还是异步、计费方式、是否已下线）。分片里现有参数缺哪些再看一遍 handler 的 `get_param` 一类调用——**一次可能缺好几个**（如 `time_beg` / `time_end` / `episode`），别只补用户点名的那一个。
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

### E. 校验（四类改动都做）

1. **先找脚本再跑**，别猜路径：`esearch`/`grep` 搜 `check_openapi.py`（本仓库在 `build/tmp/check_openapi.py`）。它遍历 `openapi.yaml` 的 `paths`，检查每个 `$ref` 指向的文件是否存在，输出 total paths 与 missing path refs；再检查分片里的 `#/components/schemas/...` 引用，输出 missing schema refs。若该脚本不存在，说明原因后再按本步逻辑临时写一个到 `build/tmp/`，不要用 PowerShell 内联 `python -c` ���带 `$ref` 的完整脚本（`$` 会被吞掉）。
2. 对每个改动过的文件单独做 `yaml.safe_load` 解析校验（含 `openapi.yaml` 本体与新建 schema），例如：
   ```powershell
   python -c "import yaml,io; [yaml.safe_load(io.open(p,encoding='utf-8')) for p in ['docs/api/openapi.yaml','docs/api/paths/api_seedance2_task.yaml','docs/api/components/schemas/TaskListPage.yaml','docs/api/components/schemas/Task.yaml']]; print('YAML OK')"
   ```
3. 用 `glob`（如 `docs/api/paths/*canvas*`）或 `grep` 核对分片清单与 `openapi.yaml` 的 `$ref` 一一对应，并确认 `openapi.yaml` 里已无对旧分片的引用；新建的组合 schema 用 `grep` 其文件名确认被至少一个分片引用。

## Pitfalls

- **从行号猜 URL / 猜结构体路径**：行号只告诉你 handler 在哪，真实注册 URL 要去注册表（`kitsu.cpp` 的 `reg_t<>` / `reg_c<>`）里按类名反查；结构体在 `src/doodle_core/metadata/...`，`src/doodle_lib/metadata/seedance2` 这类路径不存在时 `grep` 会直接 `exit 2`，别在同一路径上反复重试，换成 include 头文件名 / `include=*x*.h` 过滤再搜。
- **已知路径只是缺引用就去加 `$ref`**：分片已存在说明这是改字段/参数，`openapi.yaml` 不动，报告里要明确说“不是新路径”。
- **`git mv` / `Remove-Item` / `git status` 会被沙箱拒绝**（`ResourceUnavailable: ... 拒绝访问`）。别重试、别换 `cmd /c del`，按 A.3–A.5 的 write + edit 路径走。
- **只改 `openapi.yaml` 不改分片**：`$ref` 指向不存在的文件名，文档校验必挂。新建分片、改 `$ref`、处理旧分片必须一起做。
- **字段级只改一处**：只往 Write schema 加字段、忘了响应 schema 的“不含密钥”描述和 path 分片的“可写字段清单”，文档会自相矛盾。动手前先全量 `grep` 字段名拿到完整清单。
- **复用形状的后端清单只改一处**：`TransferStationTaskDataRequest.yaml` 说“只有 transfer_station”，而分片后端对照表没加 `kmt_image` 行（含 `/v1/images/generations`、`/v1/images/edits` 与计费方式），文档就会前后打架；先 `grep` 后端清单再逐处改，只留一种说法。
- **新增枚举型字段时在 `$ref` 与内联之间来回改判**：先看既有引用点（`Task.yaml` 的 `backend` 就是 `$ref: ./TaskBackend.yaml`），照抄仓库惯例落地一次，别“先 $ref → 嫌描述不贴切改内联 → 再改回 $ref”。枚举 schema 的 `description` 就是各引用点共用的权威说明，不算跑偏。
- **内联手抄枚举值**：无论是新字段属性还是查询参数 `schema`，都 `$ref` 到枚举 schema，避免值与 schema 漂移；也不要新建第二份 `TaskBackend` 副本。
- **可选字段没写默认值语义 / 误加进 `required`**：`if (j.contains(...))` + 成员默认值 = 可省略字段，必须进 `required` 之外，并把“省略时默认为 xxx”写进接口 description；只加 `properties` 属性会让调用方不知道省略后的行为。
- **查询参数级只加参数块**：只往分片 `parameters` 里塞 `backend`，却没在枚举 schema 里加值、没改分片 `description` 的过滤语义与“非法枚举值”清单，是最容易漏的联调点；三处一起改。也别只补用户点名的那一个参数：handler 里 `time_beg` / `time_end` / `episode` 等一起被读取时，分片缺的都要补齐。
- **响应体改形只换 `$ref` 不建 schema**：把分片 `$ref` 改成 `../components/schemas/TaskListPage.yaml` 却没 `write` 该文件，`check_openapi.py` 会直接报 missing schema refs；新建文件里的 `items.$ref` 也要指向真实存在的 `X.yaml`。
- **改形后没改描述**：分片 `description` 还写着“返回数组”，或没说明 `size` / `offset` 只作用于 `data`、`count` / `sum` 不受分页影响，调用方会误用。
- **枚举值追加打乱顺序 / 漏写语义**：新值要接在 `enum` 末尾保持既有顺序，且枚举 schema 的 `description` 里必须写明它的中文语义（含异步/同步、是否已下线），只在 `enum` 里加一个裸字符串不算同步。
- **新路径多出的层级参数漏写**：`subproject_id` 之类必须同时出现在每个受影响分片的 `parameters` 里；反过来，路径缩短后要批量清掉则用脚本 + `grep` 验证，别靠肉眼。
- **文件名花括号**：参数名要保留 `{...}`（如 `api_..._{subproject_id}_canvas.yaml`），不要转成别的占位符。
- **用户先改过的文档不是错误**：不要把 `canvas` 回滚成 `canvas_ids` 之类，只修正真正不一致的地方。
- **不要碰 `external/` 与 `vcpkg/`**；文档改动只限 `docs/api/`，不要顺手改未涉及的其他 path/schema 文件。
- **不要跑构建/测试来“验证文档”**：本仓库禁止测试，验证只靠文档解析与 `$ref` 完整性脚本。
- **不要 `git commit`**：提交一律由用户执行，只报告改动与未提交状态。

## Verification

- 在 `docs/api` 下跑 `check_openapi.py`：输出 total paths 数量（如 226/227 条或当前实际条数），`missing path refs` 与 `missing schema refs` 均为空列表。
- 对每个改动过的 YAML（含 `openapi.yaml` 与新建 schema）逐个 `yaml.safe_load`，无 parse failure，报告 `parsed OK` / `YAML OK`。
- 从行号出发的任务：报告里先写出“实际注册 URL + 对应分片 + 是否新增 `$ref`”的结论，再列字段/参数改动。
- `grep` 新路由前缀与旧路由前缀：新 path 都有 `$ref` 且格式与相邻条目一致；旧前缀（如 `paths/api_canvas.yaml`）在 `openapi.yaml` 里已无引用。
- 字段级：`grep` 字段名，确认可写 schema 有属性、响应 schema 描述含该名、path 分片字段清单含该名，三处齐全。
- 共享形状的后端清单：`grep` 后端值（如 `kmt_image`、`apipy_image`）在 `TaskBackend.yaml`、`TransferStationTaskDataRequest.yaml` 与相关 path 分片里表述一致，`grep` “只有|仅” 已无残留旧说法。
- 枚举型新字段：`read` 改过的 schema 与分片，确认 `backend` 这类属性写的是 `$ref` 到枚举 schema（响应 `./TaskBackend.yaml`、分片 `../components/schemas/TaskBackend.yaml`），与 `Task.yaml` 既有写法一致；`read` 分片 `description` 确认写了“省略时默认 `seedance2`”一类语义，且该字段不在 `required` 列表里。
- 查询参数 / 枚举级：`read` 改过的分片，确认 `parameters` 里有 `name: backend`（及 `time_beg` / `time_end` 等）且 `schema.$ref` 指向枚举 schema；`read` 枚举 schema，确认新值在 `enum` 末尾、`description` 含其语义；`grep` 分片 `description`，确认过滤字段与“非法枚举值”清单都含新参数。
- 响应体改形：`read` 新建的组合 schema（如 `TaskListPage.yaml`），确认 `data.items.$ref` 指向原有 schema、`count` / `sum` 的类型与可空性与代码一致；`grep` 新 schema 文件名，确认至少一个分片引用它，且该分片不再指向裸 array。
- `read` 至少一个新分片，确认 `tags` / `summary` / `parameters`（含新增层级参数）/ `responses` 结构完整、YAML 缩进正确。
- 汇报改动范围、仍未提交的状态；若残留旧分片，明确列出“需手工 `git rm`”。
- TODO: 若团队成员规定旧分片可以随提交一并删除，请在此补充仓库层面约定（现有证据只支持“写迁移说明”的处理方式）。
