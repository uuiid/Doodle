---
description: "Use when writing or modifying code in DoodleLiveLink (LiveLink hub, 独立 UE 项目). 项目性质：独立 Unreal Engine 项目；引擎源码根目录 E:\\UnrealEngine；目标：接收外部 LiveLink 源（动捕/面捕）并作为 WebSocket 服务端转发到 Maya 客户端。"
applyTo: "DoodleLiveLink/**"
---
# DoodleLiveLink 项目约定

`DoodleLiveLink/` 是一个 **Unreal Engine 项目**（不是 doodle 主仓库的 C++ 模块）。

## 项目定位

- **形态**：独立 Unreal Engine 项目（不是 doodle 主仓库的 C++ 模块，也不是 doodle 下的 UE 插件）。
- **引擎**：Unreal Engine 源码版，源码根目录为 `E:\UnrealEngine`。
  - 引擎路径不要硬编码到代码或 CMake 中；依赖引擎标准构建体系（`.uproject` + `Source/**/*.Build.cs` + `.Target.cs`）。
  - 项目与引擎的关联由 `DoodleLiveLink.uproject` 的 `EngineAssociation` 指定。
- **目标**：LiveLink 中心（hub）——接收外部 LiveLink 源（如动捕、面捕）的消息，并转发到 Maya。
- **通讯**：使用 **WebSocket**，UE 端为 **WebSocket 服务端**，Maya 端为 **WebSocket 客户端**（Python）。UE 侧使用 `WebSockets` 模块；Maya 侧为 Python websocket 客户端。

## 目录与结构

遵循 UE 标准项目结构：

- `Source/`：C++ 源码（模块通过 `.Build.cs` 声明依赖）
- `Content/`：资产
- `Config/`：项目配置
- `Plugins/`：插件（如需独立 LiveLink 插件）
- `DoodleLiveLink.uproject`：项目描述文件

## 约定

- 新增依赖/模块时，在对应 `*.Build.cs` 中声明（如 `"WebSockets"`、`"LiveLink"`、`"LiveLinkInterface"`、`"Networking"`、`"Sockets"`），不要写死绝对路径。
- `DoodleLiveLink.uproject` 中 `DisableEnginePluginsByDefault: true`：默认禁用引擎插件，按需显式启用所需插件（如 LiveLink 相关插件、WebSockets 相关插件）。
- UE 侧作为 WebSocket **服务端**监听并广播；Maya 侧作为客户端连接。保持两端 JSON 协议字段一致。
- 涉及 UE 的改动请优先使用 UE 自身的构建方式（`.uproject` / UBT），不要套用 doodle 主仓库的 CMake/vcpkg 构建流程。
