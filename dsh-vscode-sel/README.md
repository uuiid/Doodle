# dsh-vscode-sel

DeepSeek Harness 插件：把 VS Code 里选中的代码位置，送进**浏览器中运行的 DSH 页面**输入框的光标处。

它与 `dsh-drop-caret` 完全独立——不复用代码、不共享路由、不改动对方。

## 它解决什么

VS Code 扩展和浏览器里的 DSH 是两个进程，扩展无法直接向浏览器页面发消息。本插件在 DSH 的本地 web 服务上开一个**内存信箱**：

```
VS Code 扩展 ──POST /api/dsh-vscode-sel/push──▶ DSH host 信箱（内存，TTL 60 秒）
                                                        │
                                        GET /api/dsh-vscode-sel/pending
                                                        ▼
                                              浏览器 DSH 页面（窗口聚焦时拉取）
```

浏览器侧在**窗口重新获得焦点**时拉取一次，把内容以纯文本插入输入框的**当前光标处**。

## 插入内容

```
@src/doodle_lib/http_method/kitsu/ai_light.cpp:779-795
```

- 路径由 VS Code 扩展决定：默认是**相对工作区根目录**的路径（`dshSel.format: relative`），也可配 `full`（绝对路径）或 `basename`（仅文件名）
- 单行选区：`@路径:行号`
- 多行选区：`@路径:起始行-结束行`
- 多条同时到达时用空格连接，整体末尾补一个空格

## HTTP 接口

前缀 `/api/dsh-vscode-sel`，均为普通 HTTP，无额外鉴权（DSH 的 `webServer.register` 直接分派到 handler）。

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| POST | `/push` | body `{ filePath, startLine?, endLine? }`，入队，返回 `{ ok: true, seq }` |
| GET | `/pending` | **取出并清空**未消费条目，返回 `{ items: [...] }` |

约束：单条 body ≤ 64 KB，队列上限 32 条，条目存活 60 秒。

## 安装

> ⚠️ **不要**把这个目录交给 VS Code 的 `Developer: Install Extension from Location...`。
> 本目录是 **DSH 插件**（`dsh-vscode-sel`），不是 VS Code 扩展，安装进 VS Code 会被报
> `属性"engines"是必要属性，其类型必须是 object`。配套的 VS Code 扩展在 `E:\Doodle\vscode-extension-sel`。

```powershell
cd C:\Users\TD\.dsh\profiles\web
pnpm remove dsh-vscode-sel
pnpm add "E:\Doodle\dsh-vscode-sel"
```

装完确认是软链而不是副本：

```powershell
(Get-Item 'C:\Users\TD\.dsh\profiles\web\node_modules\dsh-vscode-sel').LinkType
# 期望输出 SymbolicLink
```

若是软链，之后修改 `E:\Doodle\dsh-vscode-sel` 下的文件只需刷新 DSH 页面即可生效。

> `pnpm-workspace.yaml` 里 `nodeLinker: hoisted`。**同版本的 `pnpm install` 会被静默跳过**，必须让本地版本号与 registry 不同（本包 `0.1.0`），或用 `pnpm add <绝对路径>`。

重启 `dsh web` 后生效。

## 验证

1. 浏览器打开 DSH 页面
2. 在 VS Code 里选中代码，执行 VS Code 扩展的命令（见 `vscode-extension-sel`）
3. 焦点切到浏览器后，输入框光标处应出现 `@路径:行号`

只验证服务端是否收到推送（不依赖 VS Code 扩展）：

```powershell
Invoke-RestMethod -Method Post -Uri 'http://127.0.0.1:3080/api/dsh-vscode-sel/push' `
  -ContentType 'application/json' `
  -Body '{"filePath":"src/doodle_lib/http_method/kitsu/ai_light.cpp","startLine":779,"endLine":795}'
```

然后切到浏览器窗口，输入框应出现 `@src/doodle_lib/http_method/kitsu/ai_light.cpp:779-795 `。

## 设计取舍

**为什么不用 DSH 的 `slash/input-insert-reference` 引用通道？**

那条通道要求 `span` 是编辑器 **detect 投影**的坐标，而外部只能读到 `InputState.draft`（**clipboard 投影**，每个引用 chip 展开为完整路径文本）。两套投影在存在 chip 时长度不等，外部无法换算；`selectSpan` 越界后返回 `null`，整条链路静默失败且不抛错。

因此本插件改用浏览器原生编辑原语（`document.execCommand('insertText')`，失败时回退 `Range.insertNode` + `InputEvent`），只插入纯文本，位置取真实光标。

**为什么聚焦驱动而不是常驻轮询？**

页面不可见时不做任何请求；`focus` 与 `visibilitychange` 各触发一次拉取。同时保留 5 秒兜底轮询，覆盖焦点事件被面板内交互吞掉的场景。

## 已知限制

- `/pending` 是**消费式**语义。同时打开多个 DSH 标签页时，先拉取的那个页面拿到内容。
- 聚焦导致插入时，光标位置取决于拉取那一刻编辑器的实际光标；切窗口后光标通常仍在原处。
- 输入框为空且从未聚焦过时，插入位置回退到编辑器末尾。
