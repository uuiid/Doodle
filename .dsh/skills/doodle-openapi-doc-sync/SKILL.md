---
name: doodle-openapi-doc-sync
description: 在 Doodle 仓库里改了后端路由 URL（如把 /api/canvas 迁到 /api/seedance2/subproject/{subproject_id}/canvas）后，同步 docs/api 下 OpenAPI 分片文件与 openapi.yaml 的 paths $ref；当 git mv / Remove-Item / git status 被沙箱拒绝时用 write + edit 替代完成迁移。
---

# Doodle OpenAPI 文档路径同步

改后端路由 URL 时，`docs/api/` 是接口文档的唯一来源，必须同步三类东西：`docs/api/openapi.yaml` 里的 `paths` 条目、`docs/api/paths/*.yaml` 分片文件名与内容、分片引用的 `components`。本 skill 记录实际可用的迁移手法——本工作区里 `git mv`、`Remove-Item`、`git status` 都会被沙箱拒绝，所以走“write 新建 + edit 旧文件 + 改 $ref”的替代路径。

## Steps

1. 先看清现有结构：用 `grep`（pattern 取路由前缀，如 `canvas`）在 `docs/api/openapi.yaml` 上定位所有相关 `paths` 条目，记录每条 path 对应的 `$ref: paths/....yaml`。分片命名规则是**路径转文件名**：`/api/a/{id}/b` → `api_a_{id}_b.yaml`，参数保留花括号。

2. 确定新路径后反推新分片文件名。例如 `/api/seedance2/subproject/{subproject_id}/canvas` → `api_seedance2_subproject_{subproject_id}_canvas.yaml`，`/api/seedance2/subproject/{subproject_id}/canvas/{canvas_id}` → `api_seedance2_subproject_{subproject_id}_canvas_{canvas_id}.yaml`。

3. 用 `read` 读取每个旧分片的完整内容，作为新文件内容的底稿（替代 `git mv` 的内容保留作用）。同时 `read` 相关 `docs/api/components/schemas/*.yaml`（如 `InfiniteCanvas.yaml`），确认 schema 语义与新路径层级一致。

4. 用 `write` 在新文件名下创建分片：保留原 `tags` / `summary` / `responses`，按新路径补齐路径参数（新增层级参数如 `subproject_id` 放在 `canvas_id` 前，`in: path`、`required: true`、类型与 format 与同级参数保持一致），并把 `description` 改成新语义（如“指定项目下”改为“指定子项目下”）。

5. 用 `edit` 更新 `docs/api/openapi.yaml`：把旧 path 条目替换为新 path 与对应 `$ref`，保持同一缩进与相邻条目顺序风格。

6. 旧分片文件不删（删不掉）：用 `edit` 把旧文件内容整体替换为两行迁移说明，例如：
   ```yaml
   # 已迁移至 api_seedance2_subproject_{subproject_id}_canvas.yaml
   # 画布现在必须归属于子项目：/api/seedance2/subproject/{subproject_id}/canvas
   ```
   这样旧文件不再含有效 YAML 定义，文件仍残留在 `docs/api/paths/` 下但不被 `openapi.yaml` 引用。

7. 用 `glob`（如 `docs/api/paths/*canvas*`）或 `grep` 核对分片清单与 `openapi.yaml` 的 `$ref` 一一对应，列出“已迁移但仍残留的旧文件名”交给用户自行处理。

8. 不要尝试用 `git` 收尾：本工作区禁止代提交、`git status` / `git mv` 也常被拒。改用 `glob` / `grep` / `read` 自证改动，然后向用户报告“改了什么、如何验证、当前未提交状态”。

## Pitfalls

- **`git mv` / `Remove-Item` / `git status` 会被沙箱拒绝**（`ResourceUnavailable: ... 拒绝访问`）。别重试、别换 `cmd /c del`，按上面 Steps 3–6 的 write + edit 路径走。
- **只改 `openapi.yaml` 不改分片**：`$ref` 指向不存在的文件名，文档校验必挂。三个动作（新建分片、改 `$ref`、处理旧分片）必须一起做。
- **新路径多出的层级参数漏写**：`subproject_id` 之类的层级参数必须同时出现在每个受影响的分片 `parameters` 里，否则生成的文档缺参。
- **文件名花括号**：参数名要保留 `{...}`（如 `api_..._{subproject_id}_canvas.yaml`），不要转成别的占位符。
- **不要碰 `external/` 与 `vcpkg/`**；文档改动只限 `docs/api/`。
- **不要跑构建/测试来“验证文档”**：本仓库禁止测试，验证只靠编译通过；文档改动用 `grep`/`glob`/`read` 自检即可。

## Verification

- `grep` 在 `docs/api/openapi.yaml` 上搜新路由前缀（如 `subproject`），确认每条新 path 都有 `$ref`，且相邻行格式与其他条目一致。
- `grep` 搜旧路由前缀（如 `/api/canvas:`、`paths/api_canvas.yaml`），确认 `openapi.yaml` 里已无对旧分片的引用。
- `glob` `docs/api/paths/*.yaml`，逐个比对：`openapi.yaml` 里出现的每个 `paths/*.yaml` 都存在；新分片存在；旧分片只剩迁移注释。
- `read` 至少一个新分片，确认 `tags` / `summary` / `parameters`（含新增层级参数）/ `responses` 结构完整、YAML 缩进正确。
- 汇报改动范围与仍未提交的状态，把残留旧文件清单明确列给用户。
- TODO: 若团队成员规定旧分片可以随提交一并删除，请在此补充仓库层面约定（现有证据只支持“写迁移说明”的处理方式）。
