---
name: doodle-convert-regt-to-regc-route-macro
description: 把 Doodle 仓库 src/doodle_lib/http_method 等注册表（如 kitsu.cpp）里的 .reg_t<类>("/url") 批量迁移为「类体内 DOODLE_HTTP_ROUTE(URL, 捕获字段) + 注册表 .reg_c<类>(构造参数)」，并保证编译通过；当用户要求“把 reg_t 改成 reg_c / 把 URL 挪进步handler 类里 / 用 DOODLE_HTTP_ROUTE 注册路由”时使用。
---

# Doodle：reg_t 注册 → 类内 DOODLE_HTTP_ROUTE + reg_c

目标形态：URL 从注册表里消失，写进处理器类体内；注册表只剩 `.reg_c<类>(构造参数)`。

先读 [.github/instructions/http-fun-macro-signature.instructions.md](.github/instructions/http-fun-macro-signature.instructions.md)，声明宏形式以它为准。

## Steps

1. **盘点注册表**：读注册表文件（如 `src/doodle_lib/http_method/kitsu.cpp`）中所有带 URL 的注册行，例如
   `.reg_t<some_class>("/api/doodle/xxx", l_arg)`。逐条记下：类全名、URL、构造参数个数、所在头文件。
2. **核对每个类的声明与构造签名**（不要凭记忆）：对每个类 grep 头文件里的 `DOODLE_HTTP_FUN` / `DOODLE_HTTP_JWT_FUN` / `DOODLE_HTTP_FUN_C` 与 `class` 声明，读构造函数，确认实际参数（证据中出现 16、20、22 个参数与 1 个 `l_sid_ctx` 的情况），确认它是在 `doodle::http` 还是在子命名空间（如 `model_library`、`socket_io`）。
3. **确认宏与注册组件语义**：读 `src/doodle_lib/core/http/http_function.h`（`url_route_component_t` 等）与 `src/doodle_lib/core/http/http_route.h`，确认 `reg_c<T>(...)` 的参数就是类构造函数参数、`DOODLE_HTTP_ROUTE` 接受的参数顺序（URL 在前，其后是捕获字段名列表，可为零个）。
4. **在类体内插入 `DOODLE_HTTP_ROUTE`**：放在最后一个 `DOODLE_HTTP_FUN_OVERRIDE(...)` / 成员声明之后、`DOODLE_HTTP_FUN_END()` 之前，例如
   - `DOODLE_HTTP_FUN(doodle_tool_version)` + `DOODLE_HTTP_FUN_OVERRIDE(get)` → 追加 `DOODLE_HTTP_ROUTE("/api/doodle/tool/version")`
   - 带路径参数：`DOODLE_HTTP_ROUTE("/api/doodle/data/assets/{id_}/file/maya", id_)`，捕获字段名必须是该类自身的真实成员。
5. **改注册表**：把每条 `.reg_t<X>("/url", 参数...)` 改成 `.reg_c<X>(参数...)`；子命名空间类保留限定名（`.reg_c<socket_io::socket_io_http>(l_sid_ctx)`）。与本次迁移无关的注册行保持原样。
6. **构建验证**：用 dsh-cmake 插件 `cmake_build`，preset `debug_doodle`，`timeoutMs` 给足（证据中全量重编两次数分钟到七分钟以上：70214 ms、435715 ms）。只看 `ok` 字段与 `diagnostics` 的文件行号。失败则按下述 Pitfalls 修复后重跑直到 `ok`。
7. **汇报**：给出「头文件 → 加了哪几行 DOODLE_HTTP_ROUTE」与「注册表 → 多少条 reg_c」的表格；说明仍未提交。不要 `git commit`，不要运行测试。

## Pitfalls

- **绝不对已存在的头文件整文件 `write`。** 证据里对 `model_library.h` 整体重写，丢失了 `pictures_base` 的完整类体，直到构建报 17 个错误（`thumbnail.cpp` 找不到成员）才暴露。正确做法：只在类体内做局部 `edit`（old_string 取 `DOODLE_HTTP_FUN_OVERRIDE(...)` 或 `DOODLE_HTTP_FUN_END()` 附近唯一片段）。确实需要重写时，先在 edit 前完整读一遍原文件并逐成员核对。
- **`http_function.h` 与 `http_route.h` 互相包含，`#pragma once` 下谁先被包含谁残缺。** `http_route.h` 已经 `#include <doodle_lib/core/http/http_function.h>`，所以**不要**在 `http_function.h` 里加 `#include "http_route.h"` 去修“类型不完整”——那是循环包含，构建期会被顺序打脸。正解：让缺少完整 `http_route` 类型的 `.cpp`（如 `kimodo_train.cpp`）自己 `#include <doodle_lib/core/http/http_route.h>`。
- **沙箱里的 git 可能被拒（`git show HEAD:...` → 拒绝访问，exit 1）。** 不要指望用 git 取回原文件；改用 grep 反推：在消费该头文件的 `.cpp` 里 grep `类名::方法名` 的实际定义，按定义签名还原声明。
- **捕获字段名必须对得上**：旧的 `reg_t` URL 里 `{xxx}` 若原本指向另一个类的指针，迁移到本类后要改成该类自己的成员（证据中 maya/ue 两处改用本类的 `id_`），不要照抄。
- **一个类注册了多个 URL 的情况先不动**：那会需要多个 route 或新的 `DOODLE_HTTP_ROUTE` 变体，证据里刻意跳过，别顺手改。
- **构造参数别丢**：`reg_c<T>(...)` 必须重放原 `reg_t<T>("/url", ...)` 里除 URL 外的全部参数（含 `l_sid_ctx` 这类上下文）。
- 搜索用 `esearch`（限定 `path`），不要 `pwsh` 递归遍历；构建只用插件工具，不要手拼 `cmake`。

## Verification

- `cmake_build(preset="debug_doodle")` 返回 `ok`，无 error diagnostics；证据中成功时 `doodle_lib.dll`、`test_main.exe`、`doodle_kitsu_supplement.exe` 均链接成功。
- 注册表文件里不再出现 `.reg_t<` 的已迁移条目，对应类体内都能 grep 到 `DOODLE_HTTP_ROUTE`。
- 改动仅限本次涉及的头文件与注册表 `.cpp`；`http_function.h` / `http_route.h` 的 include 结构与迁移前一致。
- 最终只报告改动与验证结果，保持未提交状态。
