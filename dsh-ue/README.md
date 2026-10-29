# dsh-ue

给 DSH 用的 **Unreal Engine 编译插件**：把 `Engine\Build\BatchFiles\Build.bat` 包成两个工具，让 Agent 能编译 UE 的工程与引擎，并把 UBT 的原始输出收敛成结构化诊断。

- `ue_build` —— 编译一个目标（游戏工程 / 编辑器 / 引擎），返回结论、诊断、命令与日志尾巴。
- `ue_targets` —— 只读列出当前配置下可以编译的 target，用来确认路径与默认值。

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
        projectFile: DoodleLiveLink/DoodleLiveLink.uproject
        defaultTarget: DoodleLiveLink
```

> `defaultTarget` 不是可选项的摆设：Doodle 的 `Source/DoodleLiveLink.Target.cs` 只有一个 `DoodleLiveLinkTarget` 类（`Type = TargetType.Editor`），
> 所以 UBT 的 target 名是 **`DoodleLiveLink`** 而不是 `DoodleLiveLinkEditor`。名字错了 UBT 会报
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
| `maxOutputBytes` | `33554432` | 每个输出流保留的字节上限 |
| `outputHeadBytes` / `outputTailBytes` | `6144` / `20480` | 返回给模型时保留的头/尾字节 |

## ue_build 参数

| 参数 | 说明 |
| --- | --- |
| `target` | UBT target 名，例如 `DoodleLiveLink`、`UnrealEditor`、`UnrealPak`。省略时用 `defaultTarget`，再否则取 `<工程名>Editor`（`engine=true` 时取 `UnrealEditor`） |
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

## 行为要点

- **cwd 是 `.uproject` 所在目录**：UBT 靠 cwd 找工程，乱设目录会解析到错误的 `.uproject`。
- **不自带 dotnet 也能跑通**：Build.bat 内部会 `call GetDotnetPath.bat`，优先用引擎自带的 dotnet（`Engine/Binaries/ThirdParty/DotNet/...`），所以即使 Agent 的 PATH 被清洗也能编译；失败时退出码 **999** 表示构建脚本自身出错（通常是没找到 dotnet 或 UBT 编译失败）。
- **全局串行锁**：Build.bat 用 `%tmp%` 下的锁文件串行化，并发调用会排队等待，因此工具标记为 `isConcurrencySafe: false`。
- **不谎报成功**：只有「退出码 0 且 UBT 输出 `Result: Succeeded`」才返回 `ok: true`。退出码 0 但没有 `Result:` 行会判为 `no-result`。
- **诊断解析**：同时识别 MSVC（`file(line,col): error C2039: ...`、`note:`）、clang、UBT 的 `LogXxx : error:` / 中文摘要行（`一个或多个模块编译失败`）、MSBuild（`MSB1008`）与 `Result:` / `Total execution time:` 行，并做去重。
- **输出截断**：超长输出按头/尾字节保留，不会把代理对切成半个字符；完整输出落在临时文件里，路径在 `spillPath`。

## 退出码

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

## 开发

```powershell
cd dsh-ue
node --check lib/index.js
node test/pure.mjs        # 纯函数回归：解析、判定、参数校验、target 解析
```

## 已知限制

- 只支持 Windows + UE 的 Windows 构建流程（`Build.bat` / MSVC）。
- 不解析 `.uproject` 的 `TargetPlatforms` 之外的组合，也不做 UAT / 打包（那是 `RunUAT.bat` 的职责）。
- 编译引擎目标（`engine=true`）耗时很长，默认 90 分钟超时可能不够，按需传 `timeoutMs`。
