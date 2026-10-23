---
name: doodle-http-namespace-flatten
description: "把 Doodle 仓库 src/doodle_lib/http_method 与 src/doodle_lib/http_client 下的二级命名空间（doodle::http::ai_light / doodle::http::preview / doodle::http::seedance2）并入 doodle::http，同步修改调用点与 kitsu.cpp 注册表限定名，并以 debug_doodle 编译通过为准；当用户要求合并/去掉/扁平化 http 层二级命名空间、修复改完后『找不到 seedance2 / ai_light』的编译错误时使用。"
whenToUse: 用户说“把 doodle::http::xxx 并入 doodle::http”“去掉 http_method/http_client 里的二级命名空间”“代理说找不到 seedance2/ai_light 帮我修好并编译通过”时加载；不适用于新增实体字段、reg_t→reg_c 迁移、路由拆分或 OpenAPI 同步。
---

# Doodle HTTP 层二级命名空间扁平化

把 `doodle::http::ai_light` / `doodle::http::preview` / `doodle::http::seedance2` 这类**HTTP 层**二级命名空间并入 `doodle::http`，同时不误伤 `doodle::ai_light`、`doodle::seedance2` 等同名**模型/数据层**命名空间。

## Steps

1. 先用 `esearch`（限定 `path: E:\\Doodle\\src`）或 `grep` 枚举全部落点，锁定三类目标：
   - 开闭声明：`namespace doodle::http::<子名> {` 与 `}  // namespace doodle::http::<子名>`
   - 限定引用：`http::<子名>::`、`http::seedance2::run_inference_task` 之类
   - 注册表：`src/doodle_lib/http_method/kitsu.cpp` 里的 `.reg_c<ai_light::...>` / `.reg_c<seedance2::...>`
   一次查全再动手，不要边改边找。
2. 判定每个命中属于哪一层：
   - `namespace doodle::http::<子名>`、`http::<子名>::` → HTTP 层，要改。
   - `doodle::ai_light::analyze_reference_file`（如 `ai_light_stats.h`）、`doodle::seedance2::...` → 模型/数据层，**不改**。
   拿不准就 `read` 该文件确认所在命名空间层级。
3. 逐文件修改，每个文件先 `read` 再 `edit`：同一文件内分两次 edit，开与闭各一处（闭括号带回注释 `}  // namespace doodle::http::<子名>` → `}  // namespace doodle::http`）。
   **`edit` 对未 read 的文件一律拒绝**（`cannot modify ...: file has not been read`），批量脚本不可靠，见 Pitfalls。
4. 改文件内的限定调用点：把 `http::ai_light::analyze_reference_file(...)`、`http::seedance2::run_inference_task(...)` 等改为不带子命名空间的写法。
5. 改注册表：对 `src/doodle_lib/http_method/kitsu.cpp` `read` 相关区段，统计 `ai_light::` / `seedance2::` 前缀（证据里共 56 处，全部在 `.reg_c<>` 模板参数里），用 `grep` 确认后 `edit` 批量去掉前缀，保持每行一条 `reg_c` 的格式。
6. 用 `grep` 兜底扫描残留，模式 `namespace doodle::http::(ai_light|preview|seedance2)|http::(ai_light|preview|seedance2)::`，必须无结果；同时确认模型层引用（`doodle::ai_light::`）仍在。
7. `cmake_build`，参数 `preset=debug_doodle`（`timeoutMs` 给足，如 `3600000`）。报错只看 `diagnostics`。
8. 若只 `kitsu.cpp` 失败、其余编译单元通过，99% 是注册表限定名残留：回到步骤 5，修完重跑 `cmake_build preset=debug_doodle`。
9. 全部通过后报告：改动的文件清单（`git status --short`）、编译结果。**不要 `git commit` / `git push`，不要跑测试或 release 构建。**

## Pitfalls

- **未读文件就 edit**：`preview.h` 直接 edit 被拒（`file has not been read`）。每个文件先 `read` 再 `edit`，不要指望前一步的 grep 结果算作读取。
- **用 pwsh 脚本批量替换**：对 18 个文件跑批量写入被沙箱拦截，所有文件都未生效（输出里每个文件看似 `OK`，实际没改）。不要用脚本批量改写源码；统一走 `read` + `edit`。
- **误伤同名模型层命名空间**：`doodle::ai_light::analyze_reference_file`（数据模型层）与 `doodle::http::ai_light`（路由层）同名。只改带 `http::` 限定的那一层。
- **只顾命名空间、忘了注册表**：首次 `debug_doodle` 构建 exit 2、104 条诊断，仅 `kitsu.cpp` 失败，原因是 `.reg_c<ai_light::...>` / `.reg_c<seedance2::...>` 仍然使用旧限定名。改完文件必须回头扫注册表。
- **只改开括号或只改闭括号**：闭括号注释 `}  // namespace doodle::http::<子名>` 也要一起改，否则残留 grep 会命中。
- **grep 前缀范围写错**：`pattern=http::ai_light` 会把 `doodle::ai_light` 的上下文也带出来，改完后用步骤 6 的组合模式复查。
- 测试不存在于此路径（`http_client/**`、HTTP 处理器无单测），别期待 `test_main` 验证；验证手段只有编译。

## Verification

- `grep` 模式 `namespace doodle::http::(ai_light|preview|seedance2)|http::(ai_light|preview|seedance2)::`（`path: E:\\Doodle\\src`）返回 **No matches found**。
- `doodle::ai_light::analyze_reference_file` 等模型层引用仍然存在且未被改写。
- `cmake_build` `preset=debug_doodle` 返回 `ok: true` / exit 0 且无 error diagnostics（证据中为 `[6/6] Linking CXX executable bin\\doodle_kitsu_supplement.exe`）。
- `git status --short` 列出改动但未提交，交由用户提交。
