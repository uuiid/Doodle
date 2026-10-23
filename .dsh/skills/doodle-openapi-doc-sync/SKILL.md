---
name: doodle-openapi-doc-sync
description: 在 Doodle 仓库改了后端 HTTP 路由（新增/删除/迁移 URL）、结构体或响应字段（新增 kmt_key、canvas_ids、person_links、backend 等）、查询过滤参数（新增 in query 的 backend / task_type）、或枚举取值（如 TaskBackend 加 kmt_image）后，同步 docs/api 下的 OpenAPI 分片（openapi.yaml 的 paths $ref、paths/*.yaml 的参数与 description 清单、components/schemas/*.yaml 的属性与 enum 三处联动），并处理新增枚举型字段复用既有枚举 schema、可选字段默认值语义写入 description，最后用 build/tmp/check_openapi.py 与 pyyaml 校验；当用户说“把 ai_studio 的字段更改同步到 docs/api/openapi.yaml”“路由改了同步文档”“新增 backend 查询参数同步文档”“新增后端枚举值同步文档”“新增了 task_backend backend_{task_backend::seedance2} 同步文档”“openapi 引用校验”时使用。
version: 4
---

# Doodle OpenAPI 文档同步（路由 + 字段 + 查询参数/枚举）

`docs/api/` 是接口文档的唯一来源，结构是 `openapi.yaml`（`paths` 里每条只是 `$ref: paths/xxx.yaml`）+ `paths/*.yaml` 分片 + `components/schemas/*.yaml` 分片。本 skill 覆盖三类改动，都要“先全量定位、再改文件、最后校验”：

- **A. 路由级**：新增 / 删除 / 迁移 URL（`git mv` 改名、去掉路径层级参数）。
- **B. 字段级**：后端结构体或响应新增/改名一个字段，需要同步可写 schema、响应 schema 描述、以及 path 分片里硬编码的字段清单。若新增字段是枚举型（如 `task_backend backend_`），还要决定“复用既有枚举 schema 还是内联手抄”，并把代码里的默认值与可选性写进 description。
- **C. 查询参数级 / 枚举取值级**：给 GET 等接口新增 `in: query` 过滤参数，或给枚举 schema（如 `TaskBackend.yaml`）追加取值，需要同步分片 `parameters`、枚举 schema 的 `enum` 与各取值 `description`、分片 `description` 里硬编码的过滤字段与“非法枚举值”清单。

三类都要用 `pyyaml` 校验，能跑时再跑 `build/tmp/check_openapi.py` 查 `$ref` 完整性。本工作区里 `git mv`、`Remove-Item`、`git status` 常被沙箱拒绝，迁移只能走“write 新建 + edit 旧文件 + 改 $ref”。

## Steps

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
3. `read` 读后端源头确认语义：结构体定义、`put_property_list()`、`from_json` / `to_json` 的读取方式（是否 `if (j.contains("xxx"))`、成员默认值是什么）、返回体应取哪些字段（密钥类字段是否只写不读）。
4. 按三处联动改（缺一处文档就自相矛盾）：
   - **可写 schema**（如 `components/schemas/AiStudioWrite.yaml`）：新增字段的 `type` 与 `description`，放在同类字段后面。POST 与 PUT 共用的 schema，新字段必须在这里可写。
   - **响应 schema 描述**（如 `components/schemas/AiStudio.yaml`）：若描述里硬编码了“响应中不包含 `a` / `b` / `c`”这类密钥清单，把新字段名补进去。响应里实际带出该字段时，在 `properties` 里加属性。
   - **path 分片里的字段清单**：分片的 `description` 常写成“可写字段与 `xxx::put_property_list()` 一致：`name`、`color`……”，硬编码清单，必须同步追加，否则文档与 schema 互相矛盾。
5. 响应体新增 / 改结构时：在 `components/schemas/` 下新增或修改对应 schema（`properties` 用 `type: array` + `items: $ref` 指向已存在的分片；整体改形如 `{canvas, nodes, connections}` 时新建组合 schema），把 path 分片 `responses.200.content.application/json.schema.$ref` 改指新 schema，或对原 `$ref` 用 `allOf` 追加属性后保留原引用；确认新 `$ref` 的目标文件真实存在。
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
2. `read` 后端源头确认语义：查询参数名、是否能空、对应响应字段名（如 `backend` / `task_type` 过滤的是响应里的 `backend` / `type` 字段）、新枚举值在代码里代表什么（同步还是异步、计费方式、是否已下线）。
3. 改分片 `parameters`：在同类参数（如 `task_type`）后面追加 `- name: backend` / `in: query` / `required: false`，`schema` 用 `$ref: ../components/schemas/TaskBackend.yaml` 复用枚举而不是重抄一遍值，`description` 写“按任务后端过滤（seedance2 / transfer_station / ...）”。
4. 改枚举 schema（如 `components/schemas/TaskBackend.yaml`）：在 `enum` **末尾按既有顺序追加**新值（如 `kmt_image`），不要打乱原有值顺序；同时在 `description` 里补该取值的中文语义（图片中转站、异步任务、轮询地址、计费方式；已下线的写“已下线，仅为存量任务兼容保留”）。
5. 改分片 `description` 里硬编码的约束清单，两处都要同步：过滤语义行（“`task_type` 过滤响应中的 `type`，`backend` 过滤响应中的 `backend`”）与非法值清单（“非法枚举值（`status` / `task_type` / `backend`）、非法 UUID……会导致请求失败”）。
6. 若该枚举还被别的 schema（如 `Task.yaml`）引用，顺着 `$ref` 检查这些 schema 的描述里是否也列了取值清单，一并核对；不要再新建一份枚举副本。

### D. 校验（三类改动都做）

1. 首选落到文件再执行，别用 PowerShell 内联 `python -c` 拼带 `$ref` 的完整脚本（`$` 会被吞掉）。写 `build/tmp/check_openapi.py`，遍历 `openapi.yaml` 的 `paths`，检查每个 `$ref` 指向的文件是否存在，统计 total paths 与 missing path refs；再检查各分片里的 `#/components/schemas/...` 引用，输出 missing schema refs。
2. 对每个改动过的文件单独做 `yaml.safe_load` 解析校验（含 `openapi.yaml` 本体），例如：
   ```powershell
   python -c "import yaml,io; [yaml.safe_load(io.open(p,encoding='utf-8')) for p in ['docs/api/openapi.yaml','docs/api/paths/api_seedance2_task.yaml','docs/api/components/schemas/TaskBackend.yaml']]; print('YAML OK')"
   ```
3. 用 `glob`（如 `docs/api/paths/*canvas*`）或 `grep` 核对分片清单与 `openapi.yaml` 的 `$ref` 一一对应，并确认 `openapi.yaml` 里已无对旧分片的引用。

## Pitfalls

- **`git mv` / `Remove-Item` / `git status` 会被沙箱拒绝**（`ResourceUnavailable: ... 拒绝访问`）。别重试、别换 `cmd /c del`，按 A.3–A.5 的 write + edit 路径走。
- **只改 `openapi.yaml` 不改分片**：`$ref` 指向不存在的文件名，文档校验必挂。新建分片、改 `$ref`、处理旧分片必须一起做。
- **字段级只改一处**：只往 Write schema 加字段、忘了响应 schema 的“不含密钥”描述和 path 分片的“可写字段清单”，文档会自相矛盾。动手前先全量 `grep` 字段名拿到完整清单。
- **新增枚举型字段时在 `$ref` 与内联之间来回改判**：先看既有引用点（`Task.yaml` 的 `backend` 就是 `$ref: ./TaskBackend.yaml`），照抄仓库惯例落地一次，别“先 $ref → 嫌描述不贴切改内联 → 再改回 $ref”。枚举 schema 的 `description` 就是各引用点共用的权威说明，不算跑偏。
- **内联手抄枚举值**：无论是新字段属性还是查询参数 `schema`，都 `$ref` 到枚举 schema，避免值与 schema 漂移；也不要新建第二份 `TaskBackend` 副本。
- **可选字段没写默认值语义 / 误加进 `required`**：`if (j.contains(...))` + 成员默认值 = 可省略字段，必须进 `required` 之外，并把“省略时默认为 xxx”写进接口 description；只加 `properties` 属性会让调用方不知道省略后的行为。
- **查询参数级只加参数块**：只往分片 `parameters` 里塞 `backend`，却没在枚举 schema 里加值、没改分片 `description` 的过滤语义与“非法枚举值”清单，是最容易漏的联调点；三处一起改。
- **枚举值追加打乱顺序 / 漏写语义**：新值要接在 `enum` 末尾保持既有顺序，且枚举 schema 的 `description` 里必须写明它的中文语义（含异步/同步、是否已下线），只在 `enum` 里加一个裸字符串不算同步。
- **新路径多出的层级参数漏写**：`subproject_id` 之类必须同时出现在每个受影响分片的 `parameters` 里；反过来，路径缩短后要批量清掉则用脚本 + `grep` 验证，别靠肉眼。
- **文件名花括号**：参数名要保留 `{...}`（如 `api_..._{subproject_id}_canvas.yaml`），不要转成别的占位符。
- **用户先改过的文档不是错误**：不要把 `canvas` 回滚成 `canvas_ids` 之类，只修正真正不一致的地方。
- **不要碰 `external/` 与 `vcpkg/`**；文档改动只限 `docs/api/`，不要顺手改未涉及的其他 path/schema 文件。
- **不要跑构建/测试来“验证文档”**：本仓库禁止测试，验证只靠文档解析与 `$ref` 完整性脚本。
- **不要 `git commit`**：提交一律由用户执行，只报告改动与未提交状态。

## Verification

- 在 `docs/api` 下跑 `check_openapi.py`：输出 total paths 数量（如 227 条或当前实际条数），`missing path refs` 与 `missing schema refs` 均为空列表。
- 对每个改动过的 YAML（含 `openapi.yaml`）逐个 `yaml.safe_load`，无 parse failure，报告 `parsed OK` / `YAML OK`。
- `grep` 新路由前缀与旧路由前缀：新 path 都有 `$ref` 且格式与相邻条目一致；旧前缀（如 `paths/api_canvas.yaml`）在 `openapi.yaml` 里已无引用。
- 字段级：`grep` 字段名，确认可写 schema 有属性、响应 schema 描述含该名、path 分片字段清单含该名，三处齐全。
- 枚举型新字段：`read` 改过的 schema 与分片，确认 `backend` 这类属性写的是 `$ref` 到枚举 schema（响应 `./TaskBackend.yaml`、分片 `../components/schemas/TaskBackend.yaml`），与 `Task.yaml` 既有写法一致；`read` 分片 `description` 确认写了“省略时默认 `seedance2`”一类语义，且该字段不在 `required` 列表里。
- 查询参数 / 枚举级：`read` 改过的分片，确认 `parameters` 里有 `name: backend` 且 `schema.$ref` 指向枚举 schema；`read` 枚举 schema，确认新值在 `enum` 末尾、`description` 含其语义；`grep` 分片 `description`，确认过滤字段与“非法枚举值”清单都含新参数。
- `read` 至少一个新分片，确认 `tags` / `summary` / `parameters`（含新增层级参数）/ `responses` 结构完整、YAML 缩进正确。
- 汇报改动范围、仍未提交的状态；若残留旧分片，明确列出“需手工 `git rm`”。
- TODO: 若团队成员规定旧分片可以随提交一并删除，请在此补充仓库层面约定（现有证据只支持“写迁移说明”的处理方式）。
