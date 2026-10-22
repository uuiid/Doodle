---
name: doodle-openapi-doc-sync
description: 在 Doodle 仓库改了后端 HTTP 路由（新增/删除/迁移 URL）或结构体字段（新增 kmt_key、canvas_ids 等）后，同步 docs/api 下的 OpenAPI 分片（openapi.yaml 的 paths $ref、paths/*.yaml、components/schemas/*.yaml 三处联动），并用 build/tmp/check_openapi.py 与 pyyaml 校验；当用户说“把 ai_studio 的字段更改同步到 docs/api/openapi.yaml”“路由改了同步文档”“openapi 引用校验”时使用。
version: 2
---

# Doodle OpenAPI 文档同步（路由 + 字段）

`docs/api/` 是接口文档的唯一来源，结构是 `openapi.yaml`（`paths` 里每条只是 `$ref: paths/xxx.yaml`）+ `paths/*.yaml` 分片 + `components/schemas/*.yaml` 分片。本 skill 覆盖两类改动，二者都需要“先全量定位、再改文件、最后校验”：

- **A. 路由级**：新增 / 删除 / 迁移 URL（`git mv` 改名、去掉路径层级参数）。
- **B. 字段级**：后端结构体或响应新增/改名一个字段，需要同步可写 schema、响应 schema 描述、以及 path 分片里硬编码的字段清单。

两类都要用 `pyyaml` 校验，能跑时再跑 `build/tmp/check_openapi.py` 查 `$ref` 完整性。本工作区里 `git mv`、`Remove-Item`、`git status` 常被沙箱拒绝，迁移只能走“write 新建 + edit 旧文件 + 改 $ref”。

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

1. **先定位，再动手**：用 `grep`（pattern 取字段名，如 `apipy_key`、`kmt_key`）在 `docs/api` 上**全量搜索**，一次性列出所有出现该字段的分片文件。字段级改动通常不止一处。
2. 再 `grep`（pattern 取业务域，如 `AI工作室|ai_studio`）在 `docs/api/openapi.yaml` 上确认该业务域的 `paths` `$ref` 与 `tags` 列表，检查是否还有没被分片覆盖的引用。
3. `read` 读后端源头确认语义：结构体定义、`put_property_list()`、返回体应取哪些字段（密钥类字段是否只写不读）。
4. 按三处联动改（缺一处文档就自相矛盾）：
   - **可写 schema**（如 `components/schemas/AiStudioWrite.yaml`）：新增字段的 `type` 与 `description`，放在同类字段后面。POST 与 PUT 共用的 schema，新字段必须在这里可写。
   - **响应 schema 描述**（如 `components/schemas/AiStudio.yaml`）：若描述里硬编码了“响应中不包含 `a` / `b` / `c`”这类密钥清单，把新字段名补进去。
   - **path 分片里的字段清单**：分片的 `description` 常写成“可写字段与 `xxx::put_property_list()` 一致：`name`、`color`……”，硬编码清单，必须同步追加，否则文档与 schema 互相矛盾。
5. 用户说“我自己已经改过一部分文档”时：先 `read` 读现有文档，把它当作当前事实，只补不一致的地方，**绝不把用户已做的改名回滚成旧名**。最终报告同时列出代码位置（文件 + 键名）和文档位置（文件 + 属性名）。

### C. 校验（两类改动都做）

1. 首选落到文件再执行，别用 PowerShell 内联 `python -c`（`$ref` 中的 `$` 会被吞掉）。写 `build/tmp/check_openapi.py`，遍历 `openapi.yaml` 的 `paths`，检查每个 `$ref` 指向的文件是否存在，统计 total paths 与 missing path refs；再检查各分片里的 `#/components/schemas/...` 引用，输出 missing schema refs。
2. 对每个改动过的文件单独做 `yaml.safe_load` 解析校验（含 `openapi.yaml` 本体），例如：
   ```powershell
   python -c "import yaml,io; [yaml.safe_load(io.open(p,encoding='utf-8')) for p in ['docs/api/openapi.yaml','docs/api/components/schemas/AiStudio.yaml','docs/api/components/schemas/AiStudioWrite.yaml']]; print('YAML OK')"
   ```
3. 用 `glob`（如 `docs/api/paths/*canvas*`）或 `grep` 核对分片清单与 `openapi.yaml` 的 `$ref` 一一对应，并确认 `openapi.yaml` 里已无对旧分片的引用。

## Pitfalls

- **`git mv` / `Remove-Item` / `git status` 会被沙箱拒绝**（`ResourceUnavailable: ... 拒绝访问`）。别重试、别换 `cmd /c del`，按 A.3–A.5 的 write + edit 路径走。
- **只改 `openapi.yaml` 不改分片**：`$ref` 指向不存在的文件名，文档校验必挂。新建分片、改 `$ref`、处理旧分片必须一起做。
- **字段级只改一处**：只往 Write schema 加字段、忘了响应 schema 的“不含密钥”描述和 path 分片的“可写字段清单”，文档会自相矛盾。动手前先全量 `grep` 字段名拿到完整清单。
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
- `read` 至少一个新分片，确认 `tags` / `summary` / `parameters`（含新增层级参数）/ `responses` 结构完整、YAML 缩进正确。
- 汇报改动范围、仍未提交的状态；若残留旧分片，明确列出“需手工 `git rm`”。
- TODO: 若团队成员规定旧分片可以随提交一并删除，请在此补充仓库层面约定（现有证据只支持“写迁移说明”的处理方式）。
