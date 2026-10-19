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

路径默认取**相对工作区根目录**的形式。文件不属于任何工作区目录时（无工作区、或文件在工作区之外）自动回退为绝对路径。

## 命令

| 命令 | 说明 |
| --- | --- |
| `dshSel.sendSelection` | 推送当前选区 |
| `dshSel.checkEndpoint` | 探测 DSH 端点是否可达，并报告待取条数 |

## 配置

| 键 | 类型 | 默认 | 说明 |
| --- | --- | --- | --- |
| `dshSel.endpoint` | string | `http://127.0.0.1:3080` | DSH 本地服务地址（协议 + 主机 + 端口）|
| `dshSel.format` | `relative` \| `full` \| `basename` | `relative` | `relative` 相对工作区根目录；`full` 绝对路径；`basename` 仅文件名 |
| `dshSel.includeLineNumbers` | boolean | `true` | 是否附带行号 |

## 安装

```powershell
cd E:\Doodle\vscode-extension-sel
npx @vscode/vsce package
code --install-extension .\dsh-sel-bridge-0.1.0.vsix
```

开发调试时也可以直接按 `F5` 启动扩展开发宿主。

> 需要 VS Code ≥ 1.85（依赖内置 Node 18+ 的全局 `fetch`）。

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
