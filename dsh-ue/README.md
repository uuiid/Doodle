# dsh-ue

给 DSH 用的 **Unreal Engine 插件**：把 `Engine\Build\BatchFiles\Build.bat` 包成编译工具，并直接用引擎自带的 `UnrealEditor.exe` / `UnrealEditor-Cmd.exe` 启动工程，把 UBT / 编辑器的原始输出收敛成结构化诊断。

- `ue_build` —— 编译一个目标（游戏工程 / 编辑器 / 引擎），返回结论、诊断、命令与日志尾巴。
- `ue_editor` —— 启动 `UnrealEditor-Cmd.exe` 或 `UnrealEditor.exe`，第一个参数固定为配置里的 `.uproject`，其余参数由 `extraArgs` 原样透传。
- `ue_targets` —— 只读列出当前配置下可以编译 / 启动的 target，用来确认路径与默认值。

## 安装

插件是 workspace 内的 bundle 包，用 `plugin_manager install_bundle` 安装到当前 profile：

```
plugin_manager action=install_bundle target=<仓库路径>\dsh-ue
```

安装后 profile 的项目里会有 `"dsh-ue": "link:<...>/dsh-ue"`，`cordis.patch.yml` 会插入：

```yaml
- insert:
    - id: ue-build
      name: dsh-ue
      config:
        repoRoot: E:\dsh_work\Doodle
        engineRoot: E:\dsh_work\UnrealEngine
        projectFile: <工程>.uproject
        defaultTarget: <工程名>
```

> `defaultTarget` 不是可选项的摆设：项目的 `Source/<工程名>.Target.cs` 里若只有 `<工程名>Target` 一个类（`Type = TargetType.Editor`），
> 那么 UBT 的 target 名就是 **`<工程名>`** 而不是 `<工程名>Editor`。名字错了 UBT 会报
> `Couldn't find target rules file for target '...'` 并以 `Result: Failed (RulesError)` 结束。

## 配置

| 键 | 默认值 | 说明 |
| --- | --- | --- |
| `repoRoot` | `E:\Doodle` | 工程所在的仓库根；相对路径形式的 `projectFile` 都相对它解析 |
| `engineRoot` | `E:\UnrealEngine` | 引擎安装目录（含 `Engine/Build/BatchFiles/Build.bat`） |
| `engineBuildBat` | `Engine/Build/BatchFiles/Build.bat` | 相对 `engineRoot` 的构建脚本 |
| `projectFile` | 空 | 默认 `.uproject`，相对 `repoRoot` 或绝对路径；空则只能用 `scope=absolute` |
| `pluginBuildScript` | 空 | 若填写，则不指定 target/projectFile 时直接调用这个批处理 |
| `comspec` | `C:\WINDOWS\system32\cmd.exe` | 启动 Build.bat 的 shell |
| `defaultPlatform` | `Win64` | 默认平台 |
| `defaultConfiguration` | `Development` | 默认配置 |
| `defaultTarget` | 空 | 不传 `target` 时使用的 target 名；空则回退到 `<工程名>Editor` |
| `buildTimeoutMs` | `5400000` | 单次构建超时（90 分钟）；工具声明的 `timeoutMs` 比它多 60 秒 |
| `editorProjectFile` | 空 | `ue_editor` 启动时固定附加的 `.uproject`（相对 `repoRoot` 或绝对路径）；空则沿用 `projectFile` |
| `editorTimeoutMs` | `1800000` | 单次编辑器会话超时（30 分钟）；工具声明的 `timeoutMs` 比它多 60 秒 |
| `maxOutputBytes` | `33554432` | 每个输出流保留的字节上限 |
| `outputHeadBytes` / `outputTailBytes` | `6144` / `20480` | 返回给模型时保留的头/尾字节 |

## ue_build 参数

| 参数 | 说明 |
| --- | --- |
| `target` | UBT target 名，例如 `UnrealEditor`、`UnrealPak`。省略时用 `defaultTarget`，再否则取 `<工程名>Editor`（`engine=true` 时取 `UnrealEditor`） |
| `projectFile` | 相对 `repoRoot` 或绝对路径的 `.uproject`，编译该工程的目标 |
| `engine` | `true` 时编译引擎自身的目标（默认 target 变成 `UnrealEditor`） |
| `scope` + `projectAbsolute` | 显式指定口径时用绝对路径的 `.uproject` 编译仓库外的工程（只给绝对路径也可以直接写进 `projectFile`） |
| `platform` | 默认 `Win64` |
| `configuration` | `Debug` / `DebugGame` / `Development` / `Shipping` / `Test` |
| `extraArgs` | 透传给 UBT 的开关（只接受 `-Foo` / `-Foo=bar` 形状，核心选项会被拒绝） |
| `timeoutMs` | 覆盖本次构建的超时 |

调用形态等价于：

```
cmd.exe /d /s /c <Build.bat> <Target> <Platform> <Configuration> [-Project=<abs .uproject>] [extraArgs...]
```

`/c` 之后是**逐个独立的 argv 项**，不要再手工拼成一条带引号的命令字符串：宿主按 `CommandLineToArgvW` 规则给参数转义，把内部 `"` 变成 `\"`，而 cmd 的 `/s /c` 不认 `\"`，会报「找不到程序」。

## ue_editor 参数

| 参数 | 说明 |
| --- | --- |
| `exe` | `cmd`（默认，`UnrealEditor-Cmd.exe`）或 `editor`（`UnrealEditor.exe`），都在 `<engineRoot>\Engine\Binaries\Win64` 下 |
| `extraArgs` | 编辑器参数，按顺序原样透传给 exe；`-project` / `-absproject` / `-abslog` 会被拒绝（工程由配置固定） |
| `timeoutMs` | 覆盖本次会话的超时 |

调用形态等价于：

```
<engineRoot>\Engine\Binaries\Win64\UnrealEditor[-Cmd].exe <editorProjectFile> [extraArgs...]
```

cwd 是 `.uproject` 所在目录。两个 exe 都在 Windows Job 容器里以隐藏窗口启动，只回收 stdout/stderr，不做 UI 交互；GUI 版请配合 `-NullRHI`、`-stdout`、`-log` 之类的无头开关使用，并用 `-ExecCmds=QUIT_EDITOR` 之类的方式让进程自己退出，否则会一直占用到超时。

返回里的 `target.log` 给出整份日志位置（`E:\ue_main\Saved\Logs\ue_main.log` 这种形式，`exists` 表示本次运行是否真的写出了文件）；模型默认只看到诊断与关键行，深挖细节时按该路径去读。

## 行为要点

- **cwd 是 `.uproject` 所在目录**：UBT 与编辑器都靠 cwd / 首个参数找工程，乱设目录会解析到错误的 `.uproject`。
- **不自带 dotnet 也能跑通**：Build.bat 内部会 `call GetDotnetPath.bat`，优先用引擎自带的 dotnet（`Engine/Binaries/ThirdParty/DotNet/...`），所以即使 Agent 的 PATH 被清洗也能编译；失败时退出码 **999** 表示构建脚本自身出错（通常是没找到 dotnet 或 UBT 编译失败）。
- **全局串行锁**：Build.bat 用 `%tmp%` 下的锁文件串行化，并发调用会排队等待，因此 `ue_build` 标记为 `isConcurrencySafe: false`（`ue_editor` 同样标记，避免并发启动多个编辑器会话）。
- **不谎报成功**：`ue_build` 只有「退出码 0 且 UBT 输出 `Result: Succeeded`」才返回 `ok: true`；退出码 0 但没有 `Result:` 行会判为 `no-result`。`ue_editor` 以退出码与日志里的 error 行为准：退出码 0 且没有 error 级日志才算成功。
- **诊断解析**：同时识别 MSVC（`file(line,col): error C2039: ...`、`note:`）、clang、UBT 的 `LogXxx : error:` / 中文摘要行（`一个或多个模块编译失败`）、MSBuild（`MSB1008`）、`Result:` / `Total execution time:` 行，以及编辑器模式的 `LogXxx : Error:/Warning:/Fatal:`、`Fatal error:`、`Assertion failed`、`Unhandled Exception` 行，并做去重。
- **输出截断**：超长输出按头/尾字节保留，不会把代理对切成半个字符；完整输出落在临时文件里，路径在 `spillPath`。
- **编辑器输出三段式**：`ue_editor` 只把「诊断 + 关键行 + 日志位置」交给模型，不把整段 stdout 塞进上下文：
  - `diagnostics` / `warnings`：解析出的 error / warning 行（warning 最多 5 条，其余折叠成一行计数）；
  - `output.keyLines`：从原始输出里抽出的关键行（带 `Error`/`Fatal`/`Failed`/`Missing`/`Unable`/`Assertion failed`/`Unhandled Exception` 等关键字，最多 200 行、单行截到 400 字符），`output.totalLines` 给出非空行总数；
  - `target.log`：`{ logDir, logFile, exists }`，编辑器整份日志在 `<工程目录>\Saved\Logs\<项目名>.log`。需要细节时按 `logFile` 用 read / grep 取，不必回传全文。
  - 为兼容启动早期失败（此时日志文件可能还没建）仍保留 `output.head` / `output.tail`。

## 退出码

`ue_build`：

| `code` | 含义 |
| --- | --- |
| `ok` | 成功 |
| `compile-failed` | 有编译错误 |
| `link-failed` | 有链接错误 |
| `error-diagnostics` | 有其它 error 级诊断 |
| `ubt-failed` | UBT 自己输出 `Result: Failed` |
| `inconsistent` | UBT 说成功但进程退出码非 0 |
| `exit-code` | 进程退出码非 0（999 = Build.bat 脚本失败） |
| `unknown` | 既没有退出码也没有 `Result:` 行 |
| `no-result` | 退出码 0 但找不到 `Result:` 行 |
| `cancelled` | 被取消或超时 |

`ue_editor`：`ok`（退出码 0 且无 error 日志）、`error-diagnostics`（日志里有 error 且进程未崩溃）、`exit-code`（退出码非 0）、`unknown`（没有退出码）、`cancelled`（超时或被取消）。

## 开发

```powershell
cd dsh-ue
node --check lib/index.js
node test/pure.mjs        # 纯函数回归：解析、判定、参数校验、target / 编辑器解析
```

## 已知限制

- 只支持 Windows + UE 的 Windows 构建与启动流程（`Build.bat` / `UnrealEditor*.exe` / MSVC）。
- 不解析 `.uproject` 的 `TargetPlatforms` 之外的组合，也不做 UAT / 打包（那是 `RunUAT.bat` 的职责）。
- 编译引擎目标（`engine=true`）耗时很长，默认 90 分钟超时可能不够，按需传 `timeoutMs`。
- `ue_editor` 不提供 detach / 交互式终端：编辑器窗口不显示，跑常驻会话（如 `-game` 交互）不适用。
