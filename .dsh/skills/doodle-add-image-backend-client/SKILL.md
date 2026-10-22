---
name: doodle-add-image-backend-client
description: 在 Doodle 仓库新增 AI 图片/视频中转站后端：往 task_backend 枚举末尾追加后端值并补 JSON 名字映射、新建继承 ai_client_base 的异步客户端 .h/.cpp、在 http_client/CMakeLists.txt 双列表登记、在 task.cpp client_factory 加 include/成员/构造分支，最后用 cmake_configure Ninja_debug 与 cmake_build debug_doodle 编译验证；当用户要求“新增图片/视频中转站子类”“加一个后端分类”“参考 transfer_station_client 写异步 client”时使用。
---

# 在 Doodle 新增 AI 图片/视频中转站后端

一次完整改造有且只有 5 处代码改动点 + 编译验证。按顺序做完再编译，避免多轮返工。

关键锚点文件（改动前先读，现状可能已变）：

- `src/doodle_core/metadata/seedance2/task.h` —— `task_backend` 枚举与其 JSON 名字映射（约 70–75 行）
- `src/doodle_lib/http_client/ai_client_base.h` —— 所有 client 的基类（约 18–117 行）
- `src/doodle_lib/http_client/transfer_station_client.h` / `.cpp` —— **异步后端范本**（约 16–43 行）
- `src/doodle_lib/http_client/CMakeLists.txt` —— 新源文件登记（`.h` 与 `.cpp` 两个列表各一处）
- `src/doodle_lib/http_method/seedance2/task.cpp` —— `client_factory`（约 114–137 行）

## Steps

1. **确定异步形态。** 除非用户明确要同步单次调用（如 `apipy_image_client`），新中转站一律参考 `transfer_station_client` 的异步链路：提交任务拿 `task_id` → 轮询任务状态 → 下载结果。先读 `transfer_station_client.h` 与 `ai_client_base.h`，照搬构造函数、`logger_`、`set_timeout`、`server_ip_`、结果结构体（如 `status_` / `need_retry_` / `data_response_`）等既有约定。
2. **查接口文档。** 用 `esearch`（必须带 `path`，例如 `docs\\api`）或 grep 找 `docs/api` 下对应中转站的 OpenAPI 分片（如 `docs/api/components/schemas/*.yaml`），确认提交/查询/下载的 URL、请求体字段与 `*_key` 凭据字段名。没有文档就按用户给的链接内容核对，不要臆造 URL。
3. **追加后端枚举。** 编辑 `src/doodle_core/metadata/seedance2/task.h`：
   - 在 `enum class task_backend` **末尾逗号之后追加**新值（`kmt_image,` 这类），**不要插到中间或重排**——已有枚举的 magic_enum 名字串已落库，挪动位置会破坏存量任务兼容。
   - 同一次编辑/紧接着编辑里，在 `DOODLE_JSON_SERIALIZE_ENUM_STRICT` 列表中补一行 `{task_backend::<新值>, "<新值>"}`，放在最后一项之后。
4. **新建客户端文件。** 在 `src/doodle_lib/http_client/` 下创建 `<backend>_client.h` 与 `<backend>_client.cpp`，类继承 `ai_client_base`，实现异步提交 + 轮询 + 下载；失败路径置 `status_ = doodle::seedance2::task_status::failed`、`need_retry_ = true` 并在 `data_response_` 里写错误 JSON，日志用 `SPDLOG_LOGGER_ERROR(logger_, ...)`。include 顺序交给 `.clang-format` 的 Regroup 规则，不要手排。
5. **登记 CMake 双列表。** 编辑 `src/doodle_lib/http_client/CMakeLists.txt`，**两次编辑**：一次把 `<backend>_client.h` 追加到头文件列表，一次把 `<backend>_client.cpp` 追加到源文件列表。只登记一处会导致链接期或编译期缺定义。
6. **接入工厂。** 编辑 `src/doodle_lib/http_method/seedance2/task.cpp`，三处：
   - include：在既有 client include 后加 `#include <doodle_lib/http_client/<backend>_client.h>`；
   - 缓存结构体：加成员 `std::shared_ptr<<backend>_client> <backend>_{};`；
   - `client_factory`：在最后一个 `if` 后加分支，判 `in_task.backend_ == sd2::task_backend::<新值>`，为空则 `std::make_shared<<backend>_client>(*core_set::get_set().ctx_ptr_)`（参数照抄相邻分支），然后 `return` 该成员。
7. **编译验证。** 只编译，不跑测试：
   1. `cmake_configure`，`preset: "Ninja_debug"`，必须 `exit 0`；
   2. `cmake_build`，`preset: "debug_doodle"`，`timeoutMs: 3600000`。
   若 `debug_doodle` 失败，先看返回的 `diagnostics`（文件+行号）修完再重编；重编可先用 `cmake_build` 的 `buildDir: "build/Ninja_debug"`, `targets: ["doodle_lib"]` 快速收敛，再用 `debug_doodle` 全量确认。**不要**为了绕过失败去调高 `parallel`，也不要跑 `Ninja_release` / `Release_*` / `package_release`。
8. **收尾报告。** 只报告：改了什么、两次编译是否 `exit 0`、当前处于未提交状态。**不要** `git commit` / `git push`，也不要提议代提交。

## Pitfalls

- **枚举插在中间**：`task_backend` 只能末尾追加；漏掉 `DOODLE_JSON_SERIALIZE_ENUM_STRICT` 映射会让持久化/接口层的后端名字串对不上。
- **CMakeLists 只登记一半**：`.h` 与 `.cpp` 是两个列表，必须各补一行。
- **工厂漏 include**：`task.cpp` 的 include 与成员、分支三处要同时到位，否则 `std::shared_ptr` 不完整类型报错。
- **写成同步单次调用**：用户在返工里明确纠正过“这个有异步，参考 `transfer_station_client`”。先确认形态再动手，别照 `apipy_image_client` 抄。
- **`debug_doodle` 首轮失败不是方案错**：全量编译失败（exit 2 + 若干诊断）很常见，按诊断修即可；先用 `doodle_lib` 目标收敛能省时间。
- **搜索别用 shell 递归**：文件名/内容搜索走 `esearch` 并限定 `path`；`Get-ChildItem -Recurse` / `Select-String` 会让 shell 卡死。
- **别加注释**：任务没要求就默认不写任何注释。
- **别写测试**：`src/doodle_lib/http_client/**` 本来就没有单元测试，改动也不要去 `src/test/**` 里补测试。

## Verification

- `src/doodle_core/metadata/seedance2/task.h`：新值在枚举最后，且 JSON 映射表里有同名条目。
- `src/doodle_lib/http_client/CMakeLists.txt`：`.h` 与 `.cpp` 两个列表都含新文件名。
- `src/doodle_lib/http_method/seedance2/task.cpp`：include、缓存成员、`client_factory` 分支三者齐全。
- `cmake_configure`（`Ninja_debug`）与 `cmake_build`（`debug_doodle`）均返回 `exit 0`，期间未运行任何测试、未执行 `git commit` / `git push`。
