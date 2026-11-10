# dsh-kitsu-supplement

在 DeepSeek Harness 里拉起 `doodle_kitsu_supplement.exe` 的 HTTP 服务，供本机测试使用：起服务、拿到**真实**监听端口、测试完停掉。

三个工具：

| 工具 | 作用 |
| --- | --- |
| `kitsu_server_start` | 按 `production` / `local` 模式拉起服务，返回 `port` 与 `baseUrl`（来自进程 stdout 的真实端口） |
| `kitsu_server_status` | 列出本插件记录的实例，并用 netstat 查端口被谁监听 |
| `kitsu_server_stop` | 停掉本插件拉起的实例（终止进程树并等停稳） |

## 为什么这样设计

每一条都对应服务端已核实的行为（源码位置见 `lib/index.js` 头部注释）：

- **端口必须显式传，且必须落在 50030–50330 之间。** local 模式不给 `--port` 时服务端会 `listen(0)`（随机端口），而 local 分支同样不防重复实例（`kitsu_supplement.cpp:212` 的 `stop_previous_instance` 已被注释）。随机端口意味着每次重启地址都变，测试用例没法引用，所以缺 `port` 直接报错；越界同理（`errorText` 报 `port 必须在 50030-50330 之间`）。
- **开关一律写成 `--key <value>` 两个 argv 项，不能用 `--key=value`。** 服务端用 argh 解析，`=` 形式只落进 `params_`（`argh.h:196-204`），而 `arg_({"config"})` / `arg_["local"]` 查的是 `flags_`，于是 `--config=x` 会被静默当成"没给"，进程照常启动但不读 JSON。生成命令行固定为 `--config <json路径> --root <路径> --local --port <端口>`。
- **真实端口只认 stdout 第一行。** 监听协程 `listen` 之后立刻 `std::cout << l_port << std::endl`（`http_listener.cpp:35`）。共享内存里的端口是全局单例，会被后启动的实例覆盖，不能作为某次启动的事后判据。
- **固定端口先探占用。** 默认失败并给出占用 PID；只有显式 `killExisting: true` 才先强杀。上个实例没关干净是固定端口最常见的故障。
- **config JSON 由插件生成。** 服务端 `--config` 解析失败时会 `return true` 且**不启动 HTTP**，进程可能一闪而过。JSON 写在 `%TEMP%\dsh-kitsu-supplement\kitsu-N.json`，调用返回后即删除（`configPath` 字段仍会报告路径）。
- **cwd 默认取 exe 所在目录。** exe 依赖同目录的 dll；运行路径也可以由参数 `cwd` 或配置 `defaultCwd` 显式指定，插件不猜别的位置。
- **不用 `POST /api/doodle/stop-server`。** 它要 admin JWT，且 `connection_count` 不为 0 时拒绝退出（`base_data.cpp:343-366`），测试机上的连接数恰恰不为 0。这里直接终止进程树并 `waitForExit`。

## 安装

```text
plugin_manager(action: "install_bundle", target: "E:\\Doodle\\dsh-kitsu-supplement")
```

安装后 `kitsu_server_start` / `kitsu_server_status` / `kitsu_server_stop` 出现在工具表里。

## 配置（`cordis.patch.yml`）

全部可选：

| 键 | 默认 | 说明 |
| --- | --- | --- |
| `exePath` | `E:\build\Ninja_debug\bin\doodle_kitsu_supplement.exe` | 可执行文件 |
| `defaultCwd` | 空（= exe 所在目录） | 默认运行路径 |
| `defaultMode` | `production` | `production` 或 `local` |
| `defaultRoot` | 空 | 没传 `--root` 就不写；local 分支会忽略它 |
| `defaultConfigPaths` | 见 patch | 写进 config JSON 的 `db_path` / `kitsu_front_end_path` / `root` / `public_url_port`；空串表示不写 |
| `readyPath` | `/` | 就绪探测路径；任何 HTTP 状态码都算就绪 |
| `readyTimeoutMs` | `30000` | 就绪等待预算 |
| `stopTimeoutMs` | `5000` | 终止后等待退出预算 |
| `maxOutputBytes` | `8388608` | stdout / stderr 各自的内存缓冲上限 |
| `guidanceOrder` | `1396` | 系统提示词里本工具指引段的排序值 |

## 参数

`kitsu_server_start`：

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `port` | 是 | 50030–50330 的整数，两种模式都必须显式给；越界或缺失都直接报错 |
| `mode` | | `production`（默认）/ `local`（argv 加 `--local`） |
| `local` | | `true` 等价于 `mode: "local"` |
| `cwd` | | 运行路径，绝对路径；缺省取配置，再缺省取 exe 目录 |
| `dbPath` | | 写进 config 的 `db_path`；local 模式被服务端忽略 |
| `root` | | 绝对路径，作为 `--root` 传入并作为 config 的 `root` 兜底 |
| `frontEndPath` | | 写进 config 的 `kitsu_front_end_path` |
| `publicUrlPort` | | 写进 config 的 `public_url_port` |
| `extraConfig` | | 原样并进 config JSON 的额外键（不允许 `port`） |
| `extraArgs` | | 追加在生成开关之后的 argv 项（不允许 `--local` / `--config` / `--port` / `--root`） |
| `exePath` | | 仅本次调用覆盖可执行文件 |
| `readyPath` | | 本次的就绪探测路径 |
| `killExisting` | | `true` 才强杀端口上的旧进程；默认失败并报 PID |

`kitsu_server_stop`：`serverId` 指定实例，`port` 指定端口，都不给就停掉全部。本插件**不会**杀别人启动的进程。

## 典型用法

```text
kitsu_server_start { mode: "local", port: 50123 }
→ ok, serverId=kitsu-1, baseUrl=http://127.0.0.1:50123

kitsu_server_start { port: 50124, dbPath: "E:\\tmp\\kitsu_test.db", killExisting: true }
→ ok, 先强杀端口 50124 上的旧进程再启动

kitsu_server_status { port: 50124 }
kitsu_server_stop { serverId: "kitsu-2" }
```

## 失败码

`invalid-args` / `port-in-use` / `kill-failed` / `spawn-failed` / `exited-before-listen` / `no-port-line` / `exited-before-ready` / `ready-timeout` / `cancelled`。

失败结果里会带 `outputTail`（stdout + stderr 尾巴）与服务端源码位置提示，`--config` 解析失败会给出 `config/hint` 诊断。

## 已知边界

- **local 模式忽略 `dbPath` / `root`**：服务端 local 分支使用内置数据库并把 `root` 硬编码成 `D:/sy_maigc`（用户已知，另行修正）；工具在返回的 `warnings` 里说明。
- **生产模式依赖注册表** `HKLM\SOFTWARE\Doodle\MainConfig`（以及 `\Email`）：读不到会抛异常，服务不启动。这属于测试机环境配置，插件不处理。
- 端口是否放通"公开访问"（`public_url_port`）由服务端注册表配置与服务端声明决定，插件只把它写进生成的 config。
- 实例表只在本进程内有效：DSH 重启后插件不再管之前拉起的进程（它们仍在运行），需要用 `kitsu_server_status { port }` 看占用并手工处理。

## 开发

```text
node --check lib/index.js
node test/pure.mjs
```

`test/pure.mjs` 用 `node:vm` 从 `lib/index.js` 的第一个区段（校验、argv、config、端口行解析、渲染）求值，不依赖 `@deepseek-ai/*` 包。
