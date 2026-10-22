---
name: doodle-convert-regt-to-regc-route-macro
description: 把 Doodle 仓库 src/doodle_lib/http_method 等注册表（如 kitsu.cpp）里的 .reg_t<类>("/url") 批量迁移为「类体内 DOODLE_HTTP_ROUTE(URL, 捕获字段) + 注册表 .reg_c<类>(构造参数)」，并以 debug_doodle 编译通过为准；当用户要求“把 reg_t 改成 reg_c / 把 URL 挪进 handler 类里 / 用 DOODLE_HTTP_ROUTE 注册路由 / 把某个 reg.h 或 ai_light.h 里的路由类改成宏”时使用。
---

# Doodle：reg_t 注册 → 类内 DOODLE_HTTP_ROUTE + reg_c

目标形态：URL 从注册表里消失，写进处理器类体内；注册表只剩 `.reg_c<类>(构造参数)`。

先读 [.github/instructions/http-fun-macro-signature.instructions.md](.github/instructions/http-fun-macro-signature.instructions.md)，声明宏形式以它为准。

## Steps

1. **盘点注册表**：读注册表文件（如 `src/doodle_lib/http_method/kitsu.cpp`）中所有带 URL 的注册行，例如
   `.reg_t<some_class>("/api/doodle/xxx", l_arg)`。逐条记下：类全名、URL、构造参数个数、所在头文件。
2. **先把所有目标类定位齐，再动笔（预检）**：迁移是「注册表 + 类声明」两端的成对改动，任何一端漏掉都会让构建失败。动手前对每个要迁移的类 grep `类名`（不要只 grep `DOODLE_HTTP_JWT_FUN(类名)`），确认它的声明**实际用哪种宏**：
   - `DOODLE_HTTP_FUN(类名)` / `DOODLE_HTTP_JWT_FUN(类名)` —— 朴素类；
   - `DOODLE_HTTP_FUN_C(类名, 基类)` —— 继承类（证据：`DOODLE_HTTP_FUN_C(doodle_data_shots_file_maya, up_file_shots_base)`），grep `DOODLE_HTTP_JWT_FUN(类名)` 会零命中，别据此判断“类不存在”。
   同时记下类所在的**集群头文件**：除了 `kitsu/reg_url.h`、`seedance2/reg.h` 这类集中注册头，还有 `kitsu/ai_light.h`、`http_method/other/other.h`、`http_method/up_file.h` 等按功能分组的头文件，它们同样承载 `.reg_t` 注册的类。这一步的输出应是一张「类 → 宏形式 → 头文件 → 注册行」的全量表；表里所有类的 route 都要在这一轮里补上，不要只补主头文件。
3. **按「区间」而非「已定位的类」圈定待迁类**：用户给的是注册表的一段行号区间（证据：`kitsu.cpp:236-424`，共 68 条 `.reg_t`），就把该区间内出现的**每一个**类都纳入（证据中漏掉 `kitsu_reg_url.h` 里的 `data_updata_logs`、`data_preview_files` 等类，首次构建 exit 2、7 个错误诊断；补上后才 `ok`）。定位这些类的办法是在区间覆盖的头文件里 grep `DOODLE_HTTP_JWT_FUN(` / `DOODLE_HTTP_FUN(` 逐个核对，而不是逐个手写 grep 类名。
4. **核对每个类的声明与构造签名**（不要凭记忆）：读头文件里的宏声明与 `class` 声明，读构造函数，确认实际参数（证据中出现 16、20、22 个参数与 1 个 `l_sid_ctx` 的情况），确认它是在 `doodle::http` 还是在子命名空间（如 `model_library`、`socket_io`、`seedance2`）。
5. **若 `reg.h` 里还没有 `DOODLE_HTTP_ROUTE`，先核查 `/Zc:preprocessor` 是否覆盖下游 TU**：`DOODLE_HTTP_ROUTE` 依赖 `__VA_OPT__`，要求 MSVC `/Zc:preprocessor`。`reg.h` 被大量 TU 包含，所以引入前必须确认该选项对所有包含它的目标生效——
   - grep 仓库（`path=CMakeLists.txt`、`path=CMake`、`path=src`）找 `/Zc:preprocessor`，证据中只在 [src/doodle_core/CMakeLists.txt](src/doodle_core/CMakeLists.txt) 出现一次；
   - 读该处上下文，确认它写在 `target_compile_options` 的 `PUBLIC` 段（或 `INTERFACE`/`add_compile_options`）里——PUBLIC 会传递给所有下游目标，`doodle_core` 的 PUBLIC 项即可覆盖 `doodle_lib` 与各 `exe_*`；
   - 只有确认传递链成立后才动手加 route；若只在个别 target 上，先在对应 CMake 里补齐再接宏。
6. **确认宏与注册组件语义**：读 `src/doodle_lib/core/http/http_function.h`（`url_route_component_t` 等）与 `src/doodle_lib/core/http/http_route.h`，确认 `reg_c<T>(...)` 的参数就是类构造函数参数、`DOODLE_HTTP_ROUTE` 接受的参数顺序（URL 在前，其后是捕获字段名列表，可为零个）；并确认 `DOODLE_HTTP_ROUTE` 在 `DOODLE_HTTP_FUN_C` 类体内同样可用（证据中 `up_file.h` 里直接在 `DOODLE_HTTP_FUN_C(...)` 之后、`DOODLE_HTTP_FUN_END()` 之前插入，构建通过）。
7. **在类体内插入 `DOODLE_HTTP_ROUTE`**：放在最后一个 `DOODLE_HTTP_FUN_OVERRIDE(...)` / 成员声明之后、`DOODLE_HTTP_FUN_END()` 之前，例如
   - `DOODLE_HTTP_FUN(doodle_tool_version)` + `DOODLE_HTTP_FUN_OVERRIDE(get)` → 追加 `DOODLE_HTTP_ROUTE("/api/doodle/tool/version")`
   - 带路径参数：`DOODLE_HTTP_ROUTE("/api/doodle/data/assets/{id_}/file/maya", id_)`，捕获字段名必须是该类自身的真实成员。
   - `DOODLE_HTTP_FUN_C(...)` 继承类：
     ```cpp
     DOODLE_HTTP_FUN_C(doodle_data_shots_file_maya, up_file_shots_base)
     FSys::path gen_file_path() override;
     DOODLE_HTTP_ROUTE("/api/doodle/data/shots/{id_}/file/maya", id_)
     DOODLE_HTTP_FUN_END()
     ```
     注意该类自身常不声明 URL 字段，`id_` 等成员来自基类（如 `up_file_base`）也要先确认基类确实有该成员（或在类体里补声明）。
   - 旧 URL 有**多处捕获**时全部列出，参数个数与旧 `reg_t` 的 URL 占位一致：
     `DOODLE_HTTP_ROUTE("/api/actions/tasks/{task_id_}/comments/{comment_id_}/preview-files/{preview_file_id_}", task_id_, comment_id_, preview_file_id_)`；
     换行写法（超过 `ColumnLimit: 120` 时）同样合法：
     ```cpp
     DOODLE_HTTP_ROUTE(
         "/api/actions/tasks/{task_id_}/comments/{comment_id_}/preview-files/{preview_file_id_}", task_id_, comment_id_, preview_file_id_
     )
     ```
   - **`edit` 的 `old_string` 必须唯一**：同一文件里大量类共享 `DOODLE_HTTP_FUN_OVERRIDE(get)\nDOODLE_HTTP_FUN_OVERRIDE(put)\nDOODLE_HTTP_FUN_OVERRIDE(delete_)` 这种片段，证据中它以该片段为锚在 `ai_light.h` 命中 3 次而失败。改用**含类名的更长锚点**——从 `DOODLE_HTTP_JWT_FUN(类名)` / `DOODLE_HTTP_FUN(类名)` 那一行起，连到该类独有的成员声明与 `DOODLE_HTTP_FUN_END()`，把整段作为 `old_string`，在末尾追加 route：
     ```text
     old_string:
     DOODLE_HTTP_JWT_FUN(session_instance_route)
     DOODLE_HTTP_FUN_OVERRIDE(get)
     DOODLE_HTTP_FUN_OVERRIDE(put)
     DOODLE_HTTP_FUN_OVERRIDE(delete_)
     uuid id_{};
     DOODLE_HTTP_FUN_END()
     new_string: 同上，在 `uuid id_{};` 之后插入 DOODLE_HTTP_ROUTE(...)
     ```
     一次 edit 只改一个类；不要为图省事开 `replace_all`，那会把同形片段全部写坏。
8. **按集群头文件分批补 route，再改注册表**：同一批里「先给所有相关头文件补 `DOODLE_HTTP_ROUTE`，然后才把注册表改成 `reg_c`」。证据中 `other/other.h` 的 `other::deepseek_key`、`other::key_ji_meng` 因只改了 `kitsu.cpp` 而首次构建 exit 2（7 个错误）。改注册表时：把每条 `.reg_t<X>("/url", 参数...)` 改成 `.reg_c<X>(参数...)`；子命名空间类保留限定名（`.reg_c<socket_io::socket_io_http>(l_sid_ctx)`）。与本次迁移无关的注册行保持原样。
9. **构建验证**：用 dsh-cmake 插件 `cmake_build`，preset `debug_doodle`，`timeoutMs` 给足（证据中全量重编两次数分钟到七分钟以上：70214 ms、435715 ms；增量为 38 s ~ 186 s，最近一次迁移为 195832 ms 失败 + 202786 ms 成功）。只看 `ok` 字段与 `diagnostics` 的文件行号。验收标准就是 `ok`（exit 0、无 error diagnostics）。失败则按下述 Pitfalls 修复后重跑直到 `ok`——**不要因失败就回退宏写法**，逐条补齐诊断指出的类与 route。
10. **收尾反查遗漏**：改完一批后 grep 注册表里的 `.reg_t<`（证据中 `kitsu.cpp` 有 94 处），逐条确认哪些已迁移、哪些刻意保留；也 grep 已迁移类名，确认注册表里没有残留的 `.reg_t<该类>`。
11. **汇报**：按分组统计「头文件 → 加了哪几行 DOODLE_HTTP_ROUTE」与「注册表 → 多少条 reg_c」（证据中按 `ai_light::*`、`seedance2::*` 等分组列条数与类名），并写明 `debug_doodle` 的编译结果（`ok`、exit 0、无诊断）。不要 `git commit`，不要运行测试。

## Pitfalls

- **绝不对已存在的头文件整文件 `write`。** 证据里对 `model_library.h` 整体重写，丢失了 `pictures_base` 的完整类体，直到构建报 17 个错误（`thumbnail.cpp` 找不到成员）才暴露。正确做法：只在类体内做局部 `edit`（old_string 取 `DOODLE_HTTP_FUN_OVERRIDE(...)` 或 `DOODLE_HTTP_FUN_END()` 附近唯一片段）。确实需要重写时，先在 edit 前完整读一遍原文件并逐成员核对。
- **忘记集群头文件里的类，是首次构建失败最常见的原因。** 改注册表前先把 `other/other.h`、`up_file.h`、`kitsu_reg_url.h`、`seedance2/reg.h` 这类头文件里的类全部补上 route（step 2/3 的全量表就是为了这个）；否则 `reg_c` 已经生效而类体没有 route，构建 exit 2。漏一个类就要多等一轮两到三分钟的增量编译。
- **只按 `DOODLE_HTTP_JWT_FUN(类名)` 搜不到类 ≠ 类不存在。** 继承类用 `DOODLE_HTTP_FUN_C(类名, 基类)`，朴素 GET 类用 `DOODLE_HTTP_FUN(类名)`；搜类名再读声明行的宏。
- **捕获字段名必须对得上，且必须是目标类（或其基类）自身的成员**：旧的 `reg_t` URL 里 `{xxx}` 若原本指向另一个类的指针（证据中的 `{entity_id_}`），迁移到本类后要改成该类自己的成员（如 `id_`），不要照抄；证据中 maya/ue 两处改用本类的 `id_`。`DOODLE_HTTP_ROUTE` 的字段名须与 `DOODLE_HTTP_FUN_END()` 前声明的成员名一致。
- **多处捕获参数要一起搬**：旧 URL 有两个以上 `{}` 时，`DOODLE_HTTP_ROUTE` 后面的字段列表也必须有同样多个，参数个数少于占位符会在构建/运行期出错。保留子项目/实体等占位参数，不要为了“简化”把 `{subproject_id}` 之类删掉。
- **`edit` 的 `old_string` 多处匹配就会失败**：`old_string matched 3 times ... provide a more specific old_string or set replace_all to true`。按 step 7 用含类名的整段锚点重试；不要换成 `replace_all`，也不要在同一个不唯一片段上反复盲试。若报 `old_string was not found`，先带 offset/limit `read` 该处再取新片段。
- **在 `reg.h` 里加 `DOODLE_HTTP_ROUTE` 前必须确认 `/Zc:preprocessor` 对下游 TU 生效**：该宏靠 `__VA_OPT__` 展开，没有该选项的老预处理器会静默报语法错。确认方式见 step 5（grep 全仓 → 读 CMake 里是否为 PUBLIC），不要在没确认时就改注册表。
- **`http_function.h` 与 `http_route.h` 互相包含，`#pragma once` 下谁先被包含谁残缺。** `http_route.h` 已经 `#include <doodle_lib/core/http/http_function.h>`，所以**不要**在 `http_function.h` 里加 `#include "http_route.h"` 去修“类型不完整”——那是循环包含，构建期会被顺序打脸。正解：让缺少完整 `http_route` 类型的 `.cpp`（如 `kimodo_train.cpp`）自己 `#include <doodle_lib/core/http/http_route.h>`。
- **沙箱里的 git 可能被拒（`git show HEAD:...` → 拒绝访问，exit 1）。** 不要指望用 git 取回原文件；改用 grep 反推：在消费该头文件的 `.cpp` 里 grep `类名::方法名` 的实际定义，按定义签名还原声明。
- **一个类注册了多个 URL 的情况先不动**：那会需要多个 route 或新的 `DOODLE_HTTP_ROUTE` 变体，证据里刻意跳过，别顺手改。如果确实要拆，交给 `doodle-http-route-split-subclass`。
- **构造参数别丢**：`reg_c<T>(...)` 必须重放原 `reg_t<T>("/url", ...)` 里除 URL 外的全部参数（含 `l_sid_ctx` 这类上下文）。
- 搜索用 `esearch`（限定 `path`），不要 `pwsh` 递归遍历；构建只用插件工具，不要手拼 `cmake`。

## Verification

- `cmake_build(preset="debug_doodle")` 返回 `ok`（exit 0、无 error diagnostics）——这是本类迁移唯一的验收标准；证据中成功时 `doodle_lib.dll`、`test_main.exe`、`doodle_kitsu_supplement.exe` 均链接成功。
- 注册表文件里不再出现 `.reg_t<` 的已迁移条目（用 grep `.reg_t<` 反查，剩余条目都应是有意保留的），对应类体内都能 grep 到 `DOODLE_HTTP_ROUTE`。
- 用户指定区间（如 `kitsu.cpp:236-424`）内出现的**每一个**类都有 route，包括 `kitsu_reg_url.h`、`seedance2/reg.h` 里的类；集群头文件（`http_method/other/other.h`、`http_method/up_file.h` 等）同理，含 `DOODLE_HTTP_FUN_C(...)` 形式的继承类。
- 若本次在 `reg.h` 一类共享头里新增了 `DOODLE_HTTP_ROUTE`，已确认 `/Zc:preprocessor` 经 PUBLIC 选项覆盖到包含它的所有下游 TU。
- 改动仅限本次涉及的头文件与注册表 `.cpp`；`http_function.h` / `http_route.h` 的 include 结构与迁移前一致；`src/test/**` 未改动。
- 最终只报告改动与验证结果，保持未提交状态。
