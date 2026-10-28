# DSH Selection Bridge (VS Code 扩展)

把 VS Code 中选中的代码位置，推送到**浏览器里运行的 DSH**。DSH 窗口获得焦点时，内容插入到输入框的当前光标处。

配合 DSH 插件 [`dsh-vscode-sel`](../dsh-vscode-sel) 使用；两者缺一不可。

## 用法

1. 在编辑器中选中代码
2. 触发以下任一入口：
   - 快捷键 `Ctrl+Alt+D`（需在编辑器内有选区）
   - 命令面板 → `DSH: 发送选中到 DSH 浏览器`
   - 编辑器右键菜单 → `发送选中到 DSH 浏览器`
3. 切到浏览器中的 DSH 窗口

DSH 输入框光标处出现：

```
@src/doodle_lib/http_method/kitsu/ai_light.cpp:779-795
```

路径固定取**绝对路径**。

## 命令

| 命令 | 说明 |
| --- | --- |
| `dshSel.sendSelection` | 推送当前选区 |
| `dshSel.checkEndpoint` | 探测 DSH 端点是否可达，并报告待取条数 |

## 配置

| 键 | 类型 | 默认 | 说明 |
| --- | --- | --- | --- |
| `dshSel.endpoint` | string | `http://127.0.0.1:3080` | DSH 本地服务地址（协议 + 主机 + 端口）|
| `dshSel.format` | `full` \| `basename` | `full` | `full` 绝对路径（默认）；`basename` 仅文件名 |
| `dshSel.includeLineNumbers` | boolean | `true` | 是否附带行号 |

## 安装

### 方式一：手工放进扩展目录（推荐）

本扩展无编译步骤，`extension.js` 就是全部运行时。把整个目录拷进 VS Code 扩展目录即可跳过任何安装工具：

```powershell
$dest = "$env:USERPROFILE\.vscode\extensions\doodle.dsh-sel-bridge-0.1.0"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item 'E:\Doodle\vscode-extension-sel\*' $dest -Recurse -Force
```

目录名必须是 `<publisher>.<name>-<version>`，与 `package.json` 一致。改完 `Ctrl+Shift+P` → `Reload Window`，扩展视图里搜 `DSH Selection Bridge`。

### 方式二：vsix

```powershell
cd E:\Doodle\vscode-extension-sel
npx @vscode/vsce@2.32.0 package
code --install-extension .\dsh-sel-bridge-0.1.0.vsix
```

### 方式三：F5 开发宿主

用 VS Code 打开 `E:\Doodle\vscode-extension-sel` 目录，按 `F5` 启动扩展开发宿主，在那里验证命令。

> 需要 VS Code ≥ 1.85（依赖内置 Node 18+ 的全局 `fetch`）。

### ⚠️ 不要对 `E:\Doodle\dsh-vscode-sel` 使用 Install Extension from Location

`E:\Doodle` 下有**两个都叫 `*-vscode-sel` 的目录**，极易选错：

| 目录 | 是什么 | 该不该给 VS Code 安装 |
| --- | --- | --- |
| `E:\Doodle\vscode-extension-sel` | **VS Code 扩展**（`dsh-sel-bridge`，有 `engines`）| 要 |
| `E:\Doodle\dsh-vscode-sel` | **DSH 插件**（`dsh-vscode-sel`，服务端 + 浏览器端，**没有 `engines`**）| **不要** |

把 DSH 插件目录交给 `Developer: Install Extension from Location...`，会安装出一个 `undefined_publisher.dsh-vscode-sel`，并在 Extensions 输出通道里持续刷：

```
[/E:/Doodle/dsh-vscode-sel]: 属性"engines"是必要属性，其类型必须是 `object`
```

这不是 VS Code 扩展的代码问题，是**选错了目录**。先卸载那个无效条目，再按上面的方式一安装正确的目录：

```powershell
$dest = "$env:USERPROFILE\.vscode\extensions\doodle.dsh-sel-bridge-0.1.0"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item 'E:\Doodle\vscode-extension-sel\*' $dest -Recurse -Force
```

`dsh-vscode-sel` 这个 DSH 插件走它自己的 `README.md` 里的 `pnpm add` 安装流程。

### 安装时可能遇到的报错

| 报错 | 原因与处理 |
| --- | --- |
| `属性 engines 是必要属性，其类型必须是 object` | **选错目录**了：选到了 `E:\Doodle\dsh-vscode-sel`（DSH 插件，确实没有 `engines`）。见上一节 |
| `Missing publisher name` | `package.json` 缺 `publisher` 字段 |
| 装上了但命令面板搜不到命令 | 没执行 `Reload Window`；或扩展目录名不是 `<publisher>.<name>-<version>` 形式 |
| `@vscode/vsce` 安装失败 / Node 版本不符 | `vsce@4` 要求 Node ≥ 22、`vsce@3` 要求 ≥ 20；锁定 `@vscode/vsce@2.32.0` 可兼容 Node 16+ |

## 与旧扩展的区别

旧扩展（`dsh-drop-caret` 配套的那个）把选区 `postMessage` 到**它自己 webview 里的 DSH iframe**，因此对浏览器中独立打开的 DSH 页面完全无效，且依赖 webview 内联脚本不出语法错误。

本扩展不建 webview、不管 DSH 页面在哪：只往本地 HTTP 端点推一条记录，由 DSH 侧插件在页面聚焦时取走。

## 排障

**提示「无法连接 DSH」** —— 依次确认：

1. `dsh web` 正在运行
2. 端口与 `dshSel.endpoint` 一致（DSH 默认 3080）
3. DSH 里已安装 `dsh-vscode-sel` 并重启过

跑一次 `DSH: 检查 DSH 端点连通性` 可区分「端口不通」与「插件未装」。

**推送成功但 DSH 没有插入** ——

1. 确认已切到浏览器窗口（插入由聚焦事件驱动）
2. 打开浏览器 DevTools Console，看有无 `[dsh-vscode-sel] pull failed:`
3. 手动验证端点：

```powershell
Invoke-RestMethod -Uri 'http://127.0.0.1:3080/api/dsh-vscode-sel/pending'
```

注意该接口是**消费式**的：手动查过之后队列就空了。
