# Doodle · Copilot 入口

VS Code Copilot 在本仓库的规范入口：一张仓库地图 + 一页硬约束。
C++ 规范的完整版（豁免清单、正反例、踩坑记录）在 [docs/cpp_guidelines.md](../docs/cpp_guidelines.md)。

## 仓库地图

- `src/`：C++ 源码。分层 `doodle_core`（底层核心）→ `doodle_lib`（可复用库）→ `exe_*`（可执行入口）；`maya_plug` / `maya_plug_mll` 是 Maya 插件，`exe_maya*` 是 Maya 相关 exe。新增公共 API 进 `doodle_lib`，内部实现进 `doodle_core`。
- `src/doodle_lib/http_method/**`：HTTP 处理器，声明与实现必须用宏（见下方「分区域规则」）。
- `src/test/**`：C++ 单元测试，范围仅限数据库 ORM/升级与视频图片解码。既有内容不要改动，也不要为了新功能往里加测试。
- [`test_js/`](../test_js/)：Playwright HTTP 契约测试，只有用户明确要求测试时才写。
- [`CMake/`](../CMake/)：自定义 Find 模块（`FindMaya.cmake`、`FindAutodesk_FBX.cmake` 等），外部 SDK 的接入点。
- [`CMakePresets.json`](../CMakePresets.json)：configure / build / package 的权威入口。改构建步骤优先改它和 `CMake/` 下的 Find 模块，不要在 `CMakeLists.txt` 里写死平台路径。
- [`vcpkg.json`](../vcpkg.json) + `vcpkg_ports/` + `vcpkg_triplets/`：依赖清单与 overlay，依赖只从这里管理。
- `external/`、`vcpkg/`：第三方内容（USD、libtorch 等），**不要改动**。
- [`build_script/`](../build_script/)：构建/部署脚本；`build_script/build/set_env.ps1` 是激活 vcvars64 并调用 cmake 的 wrapper。
- [`docs/`](../docs/)：领域文档；`docs/api/` 是接口文档。
- `UnrealEngine/Engine/Plugins/Doodle` 与 `script/uePlug/Doodle` 是同一份文件（前者是后者的目录链接），改一次即可。

## 硬约束

- **Git 提交由用户执行**：完成后只报告改了什么、验证结果、当前未提交状态；不要 `commit` / `push`，也不要提议代提交。
- **默认不测试**：验证只到编译通过。只有用户明确要求测试时才写用例并运行（Playwright 用例放 `test_js/`，约定见 [test_js/README.md](../test_js/README.md)）。
- **不跑发布构建**：`Ninja_release` / `Release_*` / `package_release` 已停用。
- **依赖只经 vcpkg 管理**：自定义 port 放 `vcpkg_ports/`，triplet 放 `vcpkg_triplets/`。
- **HTTP 处理器必须用宏**：声明 `DOODLE_HTTP_FUN` / `DOODLE_HTTP_JWT_FUN` + `DOODLE_HTTP_FUN_OVERRIDE`，实现一律 `DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(类名, 方法名)`；手写完整签名是禁止的。
- **没有明确要求就不写注释。**
- **不修旁支**：只做请求范围内的改动；顺手发现的其它缺陷先报告，由用户决定改不改。

## 构建、搜索与验证（VS Code 入口）

- **构建一律走 [.vscode/tasks.json](../.vscode/tasks.json) 的任务**：任务内部经 `build_script/build/set_env.ps1` 激活 vcvars64 再调 cmake。不要自己拼 `cmake` 命令行、不要手工激活 VS 环境、不要 `--fresh`。

| 目的 | 任务标签 | configure / build preset |
| --- | --- | --- |
| 独立软件、测试目标 | `cmake config` → `cmake build` | `Ninja_debug` / `debug_doodle` |
| Maya 插件 | `cmake config maya` → `cmake build maya` | `Ninja_RelWithDebInfo` / `debug_maya` |
| 清理构建产物 | `cmake clean` | — |

- 任务没覆盖的场合（纯 CI）才直接经 `build_script/build/set_env.ps1` 调用 cmake。
- **不要把「shell 手工调 wrapper」当作任务失败的兜底**：实测 `chocolatey/bin/ninja.exe` shim 会以 `0xE0434352`（CLR 异常）退出、`cmake --preset` 会因 vcpkg 下载不到 cmake 3.31.10 中断配置，这两条只说明入口错了，换 `.vscode/tasks.json` 的任务重跑即可。
- 不要调高构建并行度：`cl` 抢写 obj 会报 `Permission denied: cannot open compiler generated file`。
- 搜索用内置 `glob` / `grep`；不要用 `Get-ChildItem -Recurse` / `Select-String` 全仓库递归。
- 测试范围只有 ORM/升级与视频图片解码；网络客户端、HTTP 处理器、Maya 插件**没有**单元测试，不要期待 `test_main` 替你验证。
- Maya 版本由 CMake 变量 `Maya_Version` 驱动（条件分支见 `src/CMakeLists.txt`）；支持新版本优先扩展 `CMake/FindMaya.cmake`。
- 以下任一变化都会让 vcpkg ABI 失效、依赖全量重编（可达数小时）：MSVC 工具集版本变（`Ninja_build` preset 不声明 toolset，由 wrapper 调用的 `vcvars64.bat` 决定）；PowerShell 版本变（经 vcpkg-cmake helper 进入所有包的 ABI 哈希）；二进制缓存丢失（当前 `VCPKG_DEFAULT_BINARY_CACHE` 为 `E:\vcpkg_cache\archives`）。

## 分区域规则

进入下列路径前，先读对应规则文件：

| 路径 | 规则 |
| --- | --- |
| `src/doodle_lib/http_method/**` | [http-fun-macro-signature.instructions.md](instructions/http-fun-macro-signature.instructions.md)（声明/实现用宏）<br>[http-put-update-standard.instructions.md](instructions/http-put-update-standard.instructions.md)（`put` 用 `orm::update` 链式 + `run_sql`） |
| `UnrealEngine/Engine/Plugins/Doodle/**` | 与 `script/uePlug/Doodle` 是同一份文件（目录链接），改动只需做一次 |

新增或修改 HTTP 路由后，确认它在 `kitsu.cpp` 一类的注册表里有对应 `reg_t<>`：未注册的 route 在运行期不可达，改了也不生效。