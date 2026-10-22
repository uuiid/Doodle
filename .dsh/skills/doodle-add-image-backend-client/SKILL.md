---
name: doodle-add-image-backend-client
description: 在 Doodle 仓库新增或修改 AI 图片/视频中转站后端与客户端：往 task_backend 枚举末尾追加后端值并补 JSON 名字映射、新建或改造继承 ai_client_base 的异步客户端 .h/.cpp（请求体用类内 request_t 结构体承载）、在 http_client/CMakeLists.txt 双列表登记、在 task.cpp client_factory 加 include/成员/构造分支，最后用 cmake_configure Ninja_debug 与 cmake_build debug_doodle 编译验证；当用户要求“新增图片/视频中转站子类”“加一个后端分类”“参考 transfer_station_client 写异步 client”“把请求体改成类内结构体 / 不要临时拼 json”时使用。
whenToUse: 用户要在 Doodle 的 src/doodle_lib/http_client 下新增中转站客户端、在 src/doodle_core/metadata/seedance2/task.h 里加 task_backend 枚举值、在 http_method/seedance2/task.cpp 的 client_factory 里接新后端，或要把既有客户端（如 kmt_image_client）的请求体从临时拼 json 改成类内 request_t 结构体时。
---

# 在 Doodle 新增/改造 AI 图片/视频中转站后端

一次完整新增有 5 处代码改动点 + 编译验证；若只是改造既有客户端的请求体，只做第 4 步中的“请求体结构体化”部分 + 编译验证。按顺序做完再编译，避免多轮返工。

关键锚点文件（改动前先读，现状可能已变）：

- `src/doodle_core/metadata/seedance2/task.h` —— `task_backend` 枚举与其 JSON 名字映射（约 70–75 行）
- `src/doodle_lib/http_client/ai_client_base.h` —— 所有 client 的基类（约 18–117 行）
- `src/doodle_lib/http_client/transfer_station_client.h` / `.cpp` —— **异步后端范本**（约 16–43 行）
- `src/doodle_lib/http_client/kmt_image_client.h` / `.cpp` —— 类内 `request_t` + `to_json` + multipart 的范本
- `src/doodle_lib/http_client/CMakeLists.txt` —— 新源文件登记（`.h` 与 `.cpp` 两个列表各一处）
- `src/doodle_lib/http_method/seedance2/task.cpp` —— `client_factory`（约 114–137 行）

## Steps

1. **确定异步形态。** 除非用户明确要同步单次调用（如 `apipy_image_client`），新中转站一律参考 `transfer_station_client` 的异步链路：提交任务拿 `task_id` → 轮询任务状态 → 下载结果。先读 `transfer_station_client.h` 与 `ai_client_base.h`，照搬构造函数、`logger_`、`set_timeout`、`server_ip_`、结果结构体（如 `status_` / `need_retry_` / `data_response_`）等既有约定。
2. **查接口文档。** 用 `esearch`（必须带 `path`，例如 `docs\\api`）或 grep 找 `docs/api` 下对应中转站的 OpenAPI 分片（如 `docs/api/components/schemas/*.yaml`），确认提交/查询/下载的 URL、请求体字段与 `*_key` 凭据字段名。没有文档就按用户给的链接内容核对，不要臆造 URL。
3. **追加后端枚举。** 编辑 `src/doodle_core/metadata/seedance2/task.h`：
   - 在 `enum class task_backend` **末尾逗号之后追加**新值（`kmt_image,` 这类），**不要插到中间或重排**——已有枚举的 magic_enum 名字串已落库，挪动位置会破坏存量任务兼容。
   - 同一次编辑/紧接着编辑里，在 `DOODLE_JSON_SERIALIZE_ENUM_STRICT` 列表中补一行 `{task_backend::<新值>, "<新值>"}`，放在最后一项之后。
4. **新建或改造客户端文件。** 在 `src/doodle_lib/http_client/` 下创建 `<backend>_client.h` 与 `<backend>_client.cpp`，类继承 `ai_client_base`，实现异步提交 + 轮询 + 下载；失败路径置 `status_ = doodle::seedance2::task_status::failed`、`need_retry_ = true` 并在 `data_response_` 里写错误 JSON，日志用 `SPDLOG_LOGGER_ERROR(logger_, ...)`。include 顺序交给 `.clang-format` 的 Regroup 规则，不要手排。
   - **请求体结构体化（强制）。** 请求体**不得**用临时拼 `nlohmann::json` 的 helper（如 `to_kmt_body()`），在客户端类体内定义 `public struct request_t`：
     - 字段用中转站接口字段名 + 尾下划线（如 `model_` / `prompt_` / `size_` / `quality_` / `image_urls_`），容器字段需要时自己补 `<vector>` 等头。
     - 提供 `explicit request_t(const doodle::seedance2::transfer_station_request_t& in_req)` **构造函数做字段映射**（含默认值/回退，如 `size_` 优先 `image_size_`、否则 `aspect_ratio_`），映射逻辑只放构造里，不塞进序列化。
     - 在 `.h` 类体内只写 **friend 声明**：`friend void to_json(nlohmann::json& j, const request_t& in_req);`，**不要**在类体内写 inline 定义（否则与 `.cpp` 里的定义冲突，需回退改声明）。
     - 在 `.cpp` 里分两段实现：`<class>::request_t::request_t(...)` 字段映射 + `void to_json(nlohmann::json& j, const <class>::request_t& in_req)` **只做序列化**。该 `to_json` 如果在中转站命名空间内定义，需要能被 ADL 找到（放在与 `transfer_station_request_t` 同名的 `doodle::http::seedance2` 命名空间或相应命名空间内）。
     - multipart 分支改为接收 `const <class>::request_t&` 生成表单（`edits_form_t to_multipart(const <class>::request_t& in_req)`），文本字段从结构体取、文件/URL 字段从 `image_urls_` 等容器取。
     - 提交处先 `const <class>::request_t l_kmt_req{l_req_body};` 再据此选分支：`image_urls_` 为空走纯 JSON 请求体，非空走 multipart，**两条分支都要保留**。
   - 保留既有约定：失败路径置 `status_ = doodle::seedance2::task_status::failed`、`need_retry_ = true` 并写 `data_response_`，日志用 `SPDLOG_LOGGER_ERROR(logger_, ...)`。
5. **登记 CMake 双列表。** 编辑 `src/doodle_lib/http_client/CMakeLists.txt`，**两次编辑**：一次把 `<backend>_client.h` 追加到头文件列表，一次把 `<backend>_client.cpp` 追加到源文件列表。只登记一处会导致链接期或编译期缺定义。
6. **接入工厂。** 编辑 `src/doodle_lib/http_method/seedance2/task.cpp`，三处：
   - include：在既有 client include 后加 `#include <doodle_lib/http_client/<backend>_client.h>`；
   - 缓存结构体：加成员 `std::shared_ptr<<backend>_client> <backend>_{};`；
   - `client_factory`：在最后一个 `if` 后加分支，判 `in_task.backend_ == sd2::task_backend::<新值>`，为空则 `std::make_shared<<backend>_client>(*core_set::get_set().ctx_ptr_)`（参数照抄相邻分支），然后 `return` 该成员。
7. **编译验证。** 编译通过即唯一验收，**不跑测试**：
   1. 若新增了源文件或改了 CMakeLists，先 `cmake_configure`，`preset: "Ninja_debug"`，必须 `exit 0`；
   2. `cmake_build`，`preset: "debug_doodle"`，`timeoutMs: 3600000`。
   若全量失败，先看返回的 `diagnostics`（文件+行号）修完再重编；重编可先用 `cmake_build` 的 `buildDir: "build/Ninja_debug"`, `targets: ["doodle_lib"]` 快速收敛，再用 `debug_doodle` 全量确认。**不要**为了绕过失败去调高 `parallel`，也不要跑 `Ninja_release` / `Release_*` / `package_release`。
8. **收尾报告。** 只报告：改了什么、编译是否 `exit 0` / `ok` 无诊断、当前处于未提交状态。**不要** `git commit` / `git push`，也不要提议代提交。

## Pitfalls

- **枚举插在中间**：`task_backend` 只能末尾追加；漏掉 `DOODLE_JSON_SERIALIZE_ENUM_STRICT` 映射会让持久化/接口层的后端名字串对不上。
- **CMakeLists 只登记一半**：`.h` 与 `.cpp` 是两个列表，必须各补一行。
- **工厂漏 include**：`task.cpp` 的 include 与成员、分支三处要同时到位，否则 `std::shared_ptr` 不完整类型报错。
- **写成同步单次调用**：用户在返工里明确纠正过“这个有异步，参考 `transfer_station_client`”。先确认形态再动手，别照 `apipy_image_client` 抄。
- **请求体还是临时拼 json**：用户明确要求“把这个请求转换为结构体，而不是新的 json”，结构体放在客户端类内部即可；映射写在构造函数里，`to_json` 只做序列化，multipart 也从该结构体生成。
- **类体内写 inline `to_json` 定义**：先写成定义再改成 friend 声明会多一轮返工；`.h` 里只写 `friend void to_json(nlohmann::json& j, const request_t& in_req);`，实现在 `.cpp`。
- **`to_json` 找不到**：nlohmann 靠 ADL 找 `to_json`，`request_t` 是嵌套类时其关联命名空间是外围类所在的命名空间；把实现放进对应命名空间，或用 `j = <helper>(in_req)` 显式调用。
- **`edit` 报 old_string 匹配多处**：`.cpp` 里 `}  // namespace`、函数尾部等片段重复出现时，把 old_string 连同上一行/下一个函数首行一起带上以扩大上下文；只有确实要替换全部时才用 `replace_all: true`。
- **`debug_doodle` 首轮失败不是方案错**：全量编译失败（exit 2 + 若干诊断）很常见，按诊断修即可；先用 `doodle_lib` 目标收敛能省时间。
- **搜索别用 shell 递归**：文件名/内容搜索走 `esearch` 并限定 `path`；`Get-ChildItem -Recurse` / `Select-String` 会让 shell 卡死。
- **别加注释**：任务没要求就默认不写任何注释。
- **别写测试**：`src/doodle_lib/http_client/**` 本来就没有单元测试，改动也不要去 `src/test/**` 里补测试。

## Verification

- `src/doodle_core/metadata/seedance2/task.h`：新值在枚举最后，且 JSON 映射表里有同名条目。
- 客户端 `.h`：类体内有 `public struct request_t` 与 `friend void to_json(...)` **声明**（无 inline 定义），并包含所需容器头。
- 客户端 `.cpp`：无临时拼 json 的 helper；有 `request_t::request_t(...)` 字段映射与只做序列化的 `to_json`；multipart 由 `request_t` 生成且空/非空 `image_urls_` 两分支都在。
- `src/doodle_lib/http_client/CMakeLists.txt`：`.h` 与 `.cpp` 两个列表都含新文件名。
- `src/doodle_lib/http_method/seedance2/task.cpp`：include、缓存成员、`client_factory` 分支三者齐全。
- `cmake_configure`（`Ninja_debug`）与 `cmake_build`（`debug_doodle`，或改造场景下 `buildDir: "build/Ninja_debug"` / `targets: ["doodle_lib"]`）返回 `exit 0` / `ok` 且无 `diagnostics`，期间未运行任何测试、未执行 `git commit` / `git push`。
