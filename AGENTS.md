# Doodle 工作区指令

本文件是 agent 在本仓库工作时的**第一入口**。它只承载「每次动手前都必须知道、且在本仓库会与默认做法不同」的内容。
其余细节一律**引用**已有文件，原文永远优先于本文件的转述。

修改本文件前先读 [.github/copilot-instructions.md](.github/copilot-instructions.md)：两者受众重叠，不要制造两份互相漂移的规范。

---

## 1. 不可协商项

这些是硬约束，违反即为错误改动：

- **Git 提交一律由用户执行。** 完成改动后只报告：改了什么、验证结果、当前处于未提交状态。不要 `git commit` / `git push`，也不要主动提议代提交。
- **绝不测试。** 不要运行任何测试、不要构建测试目标、不要跑测试验证改动；验证手段只有编译通过。
- **不要跑发布构建**：`Ninja_release` / `Release_*` / `package_release` 耗时过长，用户已明确要求停用。
- **不要用非常量引用入参返回结果（out-parameter）**，即使麻烦也要用返回值。
  协程下应返回 `awaitable<结果结构体>`（如 `awaitable<spec_branch_result>`），而不是 `awaitable<void>` + 非常量引用。
- **依赖只经 `vcpkg.json` 管理。** 自定义 port 放 `vcpkg_ports/`，triplet 放 `vcpkg_triplets/`。不要在 `CMakeLists.txt` 里硬编码外部依赖路径。
- **HTTP 处理器的声明与实现必须用宏**，见 §4。手写完整签名是禁止的。
- **没有明确说明，不写注释。** 代码里默认不出现任何注释；只有任务中明确要求写注释时才写。

以 [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) 作为本库编程的基本指导。

## 2. 构建与测试

**搜索入口唯一：Everything（`esearch` 工具）。** 文件名与内容搜索一律走 `esearch`（必须限定 `path` 范围），不要用 `pwsh` 拼 `Get-ChildItem -Recurse` / `Select-String` 遍历仓库——全量递归会让 shell 卡住甚至被用户强杀。`glob` / `grep` 只在 `esearch` 无结果或 Everything 不可用时兜底。

**构建入口唯一：dsh-cmake 插件的工具。** 不要自己拼 `cmake` 命令行，不要手工激活 VS 环境，不要用 `--fresh`。

| 工具 | 作用 |
| --- | --- |
| `cmake_presets` | 查询可用 preset（只读，不确定用哪个先调它）|
| `cmake_configure` | `cmake --preset <name>` |
| `cmake_build` | 传 preset（如 `debug_doodle`）或 buildDir + targets |
| `cmake_clean` | 清理构建产物 |

| 目的 | configure preset | build preset |
| --- | --- | --- |
| 独立软件 / 测试 | `Ninja_debug` | `debug_doodle`（`doodle_kitsu_supplement` + `test_main`）|
| Maya 插件 | `Ninja_RelWithDebInfo` | `debug_maya`（`doodle_maya`）|

插件另有一条独立约束：**保持 `parallel` 默认值**，调高并行度会让 `cl` 抢写 obj 报 `Permission denied: cannot open compiler generated file`。工具的 `ok` 字段是权威结论，失败先看 `diagnostics` 给出的文件与行号。

插件内部经 `build_script/build/set_env.ps1` wrapper 调用 cmake——它激活 vcvars64 并在仓库根目录执行。**非 DSH 环境**（CI、编辑器任务、其他 agent）没有这些工具，需自行经该 wrapper 调用 cmake。相关路径解析见 [CMakePresets.json](CMakePresets.json) 与 [CMake/](CMake/) 下的 Find 模块。

其余 preset 的权威定义在 [CMakePresets.json](CMakePresets.json)；改构建步骤优先改该文件与 `CMake/` 下的 Find 模块，不要在 `CMakeLists.txt` 里写死平台路径。

- **测试范围收窄**：现有测试只有两部分——**数据库 ORM 与升级**（`sqlite_*` 系列、schema/迁移/外键），以及**视频图片文件解码**。
  其余路径（网络客户端 `src/doodle_lib/http_client/**`、HTTP 处理器、Maya 插件等）**没有**单元测试，改动这些路径不要期待 `test_main` 会替你验证。
  `src/test/**` 的既有内容不要改动，也不要为了新功能往那里加测试。
- Maya 版本由 CMake 变量 `Maya_Version` 驱动，条件分支见 `src/CMakeLists.txt`；扩展新版本优先改 `CMake/FindMaya.cmake`。

### vcpkg 全量重编的三个诱因

以下任一变化都会让 vcpkg ABI 失效、依赖全量重编（代价可达数小时），动手前先想清楚：

1. **MSVC 工具集版本**变化。`Ninja_build` preset 不再声明 `toolset`，工具集完全由 wrapper 调用的 `vcvars64.bat` 决定（当前 `14.44.35207`，即 v143）；换机器或换 VS 安装组件都可能改变它。
2. **PowerShell 版本**变化。它经 vcpkg-cmake helper 级联进所有包的 ABI 哈希。
3. 二进制缓存丢失。当前 `VCPKG_DEFAULT_BINARY_CACHE` 为 `E:\vcpkg_cache\archives`。

排查方法：`vcpkg_abi_info.txt` 只记录 `cmake` / `powershell` / `triplet`，**不含编译器**；编译器要看 `CMakeCache.txt` 的 `Z_VCPKG_CL`、`CMakeFiles/rules.ninja` 里的 cl.exe 绝对路径，以及 `vcpkg/compiler-file-hash-cache.json`。

## 3. 代码组织与风格

分层：`doodle_core`（底层核心）→ `doodle_lib`（可复用库）→ `exe_*`（可执行入口）/ `maya_plug*`（Maya 插件）。新增公共 API 进 `doodle_lib`，内部实现进 `doodle_core`。

- 格式化用 [.clang-format](.clang-format)：Google 基线，`ColumnLimit: 120`，`IncludeBlocks: Regroup`。include 顺序**由该文件的分组规则决定**（mimalloc → doodle_core → doodle_app → doodle_lib → boost → maya_plug），不要凭习惯排。
- 静态检查用 [.clang-tidy](.clang-tidy)：`abseil-* / bugprone-* / google-* / misc-* / modernize-* / performance-* / portability-* / readability-*`，且 `WarningsAsErrors: " "`——命中即为错误。豁免项及其理由写在该文件头部注释里，要加新的豁免请连理由一起补。
- C++ 标准为 C++20（`CMakeLists.txt` 的 `CMAKE_CXX_STANDARD`）。协程已是本仓库一等公民，异步逻辑优先用协程而非回调。

## 4. 分区域规则

工作涉及下列路径时，**先读对应的规则文件**：

- HTTP 处理器（`src/doodle_lib/http_method/**`）
  - [.github/instructions/http-fun-macro-signature.instructions.md](.github/instructions/http-fun-macro-signature.instructions.md) —— 声明用 `DOODLE_HTTP_FUN` / `DOODLE_HTTP_JWT_FUN` + `DOODLE_HTTP_FUN_OVERRIDE`，实现一律 `DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(类名, 方法名)`。
  - [.github/instructions/http-put-update-standard.instructions.md](.github/instructions/http-put-update-standard.instructions.md) —— `put` 必须用 `orm::update` 链式 + `run_sql`，禁止「读取实体 → 改写 → 整体写回」。
- DoodleLiveLink（`DoodleLiveLink/**`）
  - [.github/instructions/doodle-livelink-ue-project.instructions.md](.github/instructions/doodle-livelink-ue-project.instructions.md) —— 它是**独立 UE 项目**而非本仓库的 C++ 模块，引擎根目录 `E:\UnrealEngine`。
- UE 插件（`UnrealEngine/Engine/Plugins/Doodle/**`）
  - `E:\Doodle\UnrealEngine\Engine\Plugins\Doodle` 是指向 `E:\Doodle\script\uePlug\Doodle` 的 junction，两处路径等价、是同一份文件。用哪条路径读写都可以，改动只需做一次。

新增或修改 HTTP 路由后，确认它在 `kitsu.cpp` 一类的注册表里有对应 `reg_t<>`：未注册的 route 在运行期不可达，改了也不生效。

## 5. 任务型 skill

[.github/skills/](.github/skills/) 下的任务清单按需加载：

| skill | 处理什么 |
| --- | --- |
| `debug-fun` | 审查指定 C++ 代码（函数/类/文件），检查逻辑、协程、空指针、线程安全 |
| `refactor-put-set-from-ref` | 把 put 处理器重构为 `orm::update` 链式（JSON 走 `set_from_ref<T>`）|
| `review-git-commit` | 审查 staged / commit / 分支区间的改动 |
| `ai` | 骨骼蒙皮权重推理模型 |
| `write-coding-standards-from-file` | 从既有文件归纳编码规范文档 |

## 6. 超出本文件范围的事

功能语义、业务流程、各模块的域知识在 [README.md](README.md) 与 `docs/` 下；`docs/api/` 是接口文档。
`external/`（USD、libtorch 等）与 `vcpkg/` 是第三方内容，**不要改动**，它们各自带自己的 `AGENTS.md`。
