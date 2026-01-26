## 快速目标（对 AI 编码代理）

这是一个以 CMake + vcpkg 为中心的 Windows 优先的 C++ 项目，包含可执行程序、Maya 插件、UE 插件与若干工具。主要路径和约定见下。请在进行改动前阅读并参考下面的文件/命令。只修改能通过本地构建与测试的最小改动。

### 一眼看懂仓库结构
- `src/`：源代码入口。模块分层：`doodle_core`（底层核心）、`doodle_lib`（可复用库）、`exe_*`（各可执行入口）、`maya_plug*`（Maya 插件）、`exe_maya*`（Maya 相关 exe）。
- `CMake/`：自定义的 Find 模块（例如 `FindMaya.cmake`, `FindAutodesk_FBX.cmake`），查看这里即可了解如何寻找并链接外部 SDK（Maya/FBX/Photoshop 等）。
- `external/`、`vcpkg/`：第三方库和预编译内容（例如 USD、libtorch）。依赖由 `vcpkg.json` 管理；本仓库使用 overlay ports (`vcpkg_ports`) 和 overlay triplets (`vcpkg_triplets`).
- `build_script/`：一组用于构建/部署的脚本（Windows .cmd/.ps1）。重要：`build_script/build/set_venv.cmd` 是调用 CMake 的 wrapper，会激活 `.venv` 并运行 vcvars，VS/Ninja 构建都通过它。 
- `docs/`：功能/域特定的文档（UE、Maya、导出流程等），调试新逻辑时常有助。

### 构建与测试：可复制的最小步骤（证据：`CMakePresets.json` + `set_venv.cmd`）
- 常用 presets（在 `CMakePresets.json`）：
  - `Ninja_debug` / build preset `debug_doodle`：调试 exe。
  - `Ninja_RelWithDebInfo` / build preset `debug_maya`：用于构建 Maya 插件（仓库约定 Maya 插件使用 RelWithDebInfo）。
- 在本仓库中，所有 CMake/Build 命令通过 `build_script/build/set_venv.cmd` 调用（该脚本会 set venv 并调用 vcvars）。示例：
  - CMake configure（示例）: 调用 wrapper 并传入 cmake 可执行路径与 `--preset`。VSCode tasks 已配置并使用该 wrapper（参见 workspace tasks）。
- 单元/集成测试：编译后可在 `build/Ninja_debug/bin/test_main.exe` 运行（仓库提供了 VSCode task “运行boost测试”）。测试框架为 Boost.Test（见 vcpkg 依赖 `boost-test`）。

### 项目约定与常见变更点（具体、可操作）
- 新增第三方库：修改 `vcpkg.json`（并把自定义 port 放在 `vcpkg_ports/`），然后使用 CMakePresets（或通过 wrapper）重新 configure。不要直接在 CMakeLists 中硬编码外部依赖路径。
- Maya/FBX 集成：查看 `CMake/FindMaya.cmake` 与 `CMake/FindAutodesk_FBX.cmake`，Maya 版本通过 CMake 的 `Maya_Version` 变量驱动（见 `src/CMakeLists.txt` 条件分支）。要支持新 Maya 版本，优先修改或扩展 `FindMaya.cmake`。 
- 插件调试：Maya 插件用 `Ninja_RelWithDebInfo`；如果需要在本地调试，使用 build preset `debug_maya`（会构建目标 `doodle_maya`, `doodle_maya_exe`）。
- 可执行程序：主要 exe 在 `exe_*` 子目录，通过 `debug_doodle` preset 构建 `doodle_kitsu_supplement` 等目标。

### 代码风格与检查点（从代码/工程可观察到的习惯）
- 倾向在 C++ 里拆分为 core/lib/exe 三层；新增公共 API 放到 `doodle_lib`，内部实现放 `doodle_core`。
- 对平台/SDK 兼容性使用 CMake 条件判断（例如 `if (DEFINED Maya_Version)`）——请在 CMake 修改时维持这种条件分支以避免影响其它构建目标。

### 快速导航：关键文件示例
- 根 README: `README.md` — 总体功能与模块索引（项目域知识）。
- CMake presets: `CMakePresets.json` — 构建/测试/打包的官方入口，AI 修改构建步骤应优先修改 presets。 
- vcpkg config: `vcpkg.json` — 所有库依赖清单（使用 overlay ports/triplets）。
- 自定义查找模块: `CMake/FindMaya.cmake`, `CMake/FindAutodesk_FBX.cmake` — 改变集成点时必看。
- 构建 wrapper: `build_script/build/set_venv.cmd` — 所有 CI/本地 tasks 会调用它以确保环境一致。

### 做变更时的安全步骤（必做）
1. 在本地用对应 preset 进行 configure+build（通过 `set_venv.cmd` wrapper 或 VSCode 提供的 task）。
2. 运行受影响模块的测试（`build/Ninja_debug/bin/test_main.exe` 或对应 test preset）。
3. 对于依赖变更，确保 `vcpkg.json` 与 overlays 一致，并在 CI/同事机器上复现一次 configure。 

如果这些说明中有任何不清楚或缺失的点（比如你想补充某个常用的调试命令或特定模块的开发流程），告诉我你想要增强的部分，我会把文档进一步收紧并合并到仓库。 
