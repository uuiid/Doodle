# dsh-kitsu-supplement

在 DeepSeek Harness 里拉起 `doodle_kitsu_supplement.exe` 的 HTTP 服务，供本机测试使用：起服务、拿到**真实**监听端口、改数据库填测试数据、跑 `test_js` 里的 Playwright 用例、测试完停掉。

五个工具：

| 工具 | 作用 |
| --- | --- |
| `kitsu_server_start` | 按 `production` / `local` 模式拉起服务，返回 `port` 与 `baseUrl`（来自服务端发布的共享内存段） |
| `kitsu_server_status` | 列出本插件记录的实例，并用 netstat 查端口被谁监听 |
| `kitsu_server_stop` | 停掉本插件拉起的实例（终止进程树并等停稳） |
| `kitsu_server_sql` | 对磁盘库执行 SQL 文件或片段（建表、改数据、填测试数据），**不需要停服或重启** |
| `kitsu_playwright_test` | 把参数原样转发给 `test_js` 里的 playwright CLI（`playwright test ...` 的 `test ...` 部分），返回摘要、失败标题与输出尾巴；**不可并发** |

## 为什么这样设计

每一条都对应服务端已核实的行为（源码位置见 `lib/index.js` 头部注释）：

- **端口必须显式传，且必须落在 50030–50330 之间。** local 模式不给 `--port` 时服务端会 `listen(0)`（随机端口），而 local 分支同样不防重复实例（`kitsu_supplement.cpp:219` 的 `stop_previous_instance` 已被注释）。随机端口意味着每次重启地址都变，测试用例没法引用，所以缺 `port` 直接报错；越界同理（报 `port 必须在 50030-50330 之间`）。
- **开关一律写成 `--key <value>` 两个 argv 项，不能用 `--key=value`。** 服务端用 argh 解析，`=` 形式只落进 `params_`（`argh.h:196-204`），而 `arg_({"config"})` / `arg_["local"]` 查的是 `flags_`，于是 `--config=x` 会被静默当成"没给"，进程照常启动但不读 JSON。生成命令行固定为 `--config <json路径> --root <路径> --dbPath <库路径> --local --port <端口>`。
- **真实端口从共享内存段读取。** 服务端 `listen` 之后把端口写进命名段 `doodle_http_listener_port_<pid>`（`http_listener_port_shm.cpp:35-63`），结构是 12 字节的 `magic(u32) / version(u32) / port(u16) / reserved(u16)`。插件用子进程 pid 拼段名，轮询到 `magic=0x444c5054 && version=1 && 端口合法` 为止；段名带 pid，每个实例各占一块，不会互相覆盖。stdout 仍会打印端口（`http_listener.cpp:35`），但只在失败诊断时用于比对（`shm/stdout-mismatch`）。
- **读段用 `node:ffi`。** 需要 Node.js 26.1+ 且构建带 FFI 支持（`--experimental-ffi` 已在 26.9+ 默认开启；权限模型下需要 `--allow-ffi`）。三个实测约束写在 `readListenerPortShm` 上：签名必须是 `{ return, arguments }`；`string` 参数按 UTF-8 传，所以只能用 `OpenFileMappingA`（`*W` 永远读不到）；插件只读，`MapViewOfFile` 恒用 `FILE_MAP_READ`。
- **固定端口先探占用。** 默认失败并给出占用 PID；只有显式 `killExisting: true` 才先强杀。上个实例没关干净是固定端口最常见的故障。
- **config JSON 由插件生成。** 服务端 `--config` 解析失败时会 `return true` 且**不启动 HTTP**，进程可能一闪而过。JSON 写在 `%TEMP%\dsh-kitsu-supplement\kitsu-N.json`，调用返回后即删除（`configPath` 字段仍会报告路径）。
- **cwd 默认取 exe 所在目录。** exe 依赖同目录的 dll；运行路径也可以由参数 `cwd` 或配置 `defaultCwd` 显式指定，插件不猜别的位置。
- **不用 `POST /api/doodle/stop-server`。** 它要 admin JWT，且 `connection_count` 不为 0 时拒绝退出（`base_data.cpp:343-366`），测试机上的连接数恰恰不为 0。这里直接终止进程树并 `waitForExit`。
- **写库不停服。** `upgrade()` 会对磁盘库调 `journal_mode(wal)`（`sqlite_database.cpp:1335`），但实测新建库上 `pragma journal_mode` 仍是 `delete`——两种模式下外部进程提交的写入都能被运行中的服务端读到，所以 `kitsu_server_sql` 只做"开库 → 事务 → 提交"，不碰进程。写锁最多等 5 秒（`busy_timeout`），撞上服务端正好在写也只是排队。
- **local 模式不给 dbPath 就没有可写的库。** 服务端 local 分支在没拿到 `--dbPath` 时落到 `":memory:"`（`kitsu_supplement.cpp:175-178`），那个库只活在本进程里，外部进程根本看不到，`kitsu_server_sql` 会直接拒绝并说明原因。

## 安装

```text
plugin_manager(action: "install_bundle", target: "E:\\Doodle\\dsh-kitsu-supplement")
```

安装后 `kitsu_server_start` / `kitsu_server_status` / `kitsu_server_stop` / `kitsu_server_sql` 出现在工具表里。

## 配置（`cordis.patch.yml`）

全部可选：

| 键 | 默认 | 说明 |
| --- | --- | --- |
| `exePath` | `E:\build\Ninja_debug\bin\doodle_kitsu_supplement.exe` | 可执行文件 |
| `defaultCwd` | 空（= exe 所在目录） | 默认运行路径 |
| `defaultMode` | `production` | `production` 或 `local` |
| `defaultRoot` | 空 | 没传 `--root` 就不写 |
| `defaultConfigPaths` | 见 patch | 写进 config JSON 的 `db_path` / `kitsu_front_end_path` / `root` / `public_url_port`；空串表示不写；`db_path` 同时是 `kitsu_server_sql` 的兜底目标 |
| `readyPath` | `/` | 就绪探测路径；任何 HTTP 状态码都算就绪 |
| `readyTimeoutMs` | `30000` | 就绪等待预算 |
| `stopTimeoutMs` | `5000` | 终止后等待退出预算 |
| `maxOutputBytes` | `8388608` | stdout / stderr 各自的内存缓冲上限 |
| `guidanceOrder` | `1396` | 系统提示词里本工具指引段的排序值 |
| `playwrightCwd` | `E:\Doodle\test_js` | `kitsu_playwright_test` 的默认 `cwd` |
| `playwrightNodePath` | `node` | 跑 playwright 用的 node；空串 = 从 PATH 取 |
| `playwrightCliPath` | 空（= `<cwd>\node_modules\@playwright\test\cli.js`） | 显式指定 playwright 入口 |
| `playwrightDefaultBaseUrl` | 空 | 调用时没给 `baseUrl`、父进程也没有 `DOODLE_BASE_URL` 时的兜底值 |
| `playwrightTimeoutMs` | `900000` | 单次运行的整体预算 |
| `playwrightKillTimeoutMs` | `5000` | 终止后等待退出的预算 |

## 参数

`kitsu_server_start`：

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `port` | 是 | 50030–50330 的整数，两种模式都必须显式给；越界或缺失都直接报错 |
| `mode` | | `production`（默认）/ `local`（argv 加 `--local`） |
| `local` | | `true` 等价于 `mode: "local"` |
| `cwd` | | 运行路径，绝对路径；缺省取配置，再缺省取 exe 目录 |
| `dbPath` | | 写进 config 的 `db_path`，同时以 `--dbPath` 传给服务端；local 模式不给就是内存库 `:memory:` |
| `root` | | 绝对路径，作为 `--root` 传入并作为 config 的 `root` 兜底 |
| `frontEndPath` | | 写进 config 的 `kitsu_front_end_path` |
| `publicUrlPort` | | 写进 config 的 `public_url_port` |
| `extraConfig` | | 原样并进 config JSON 的额外键（不允许 `port`） |
| `extraArgs` | | 追加在生成开关之后的 argv 项（不允许 `--local` / `--config` / `--port` / `--root` / `--dbPath`） |
| `exePath` | | 仅本次调用覆盖可执行文件 |
| `readyPath` | | 本次的就绪探测路径 |
| `killExisting` | | `true` 才强杀端口上的旧进程；默认失败并报 PID |

`kitsu_server_sql`：

| 参数 | 说明 |
| --- | --- |
| `sql` | 内联 SQL，可含多条语句；在所有 `files` 之后执行 |
| `files` | SQL 文件绝对路径数组（`.sql`），按给定顺序先执行 |
| `dbPath` | 目标库；缺省取 `serverId` 实例的库路径，再缺省取 `defaultConfigPaths.db_path` |
| `serverId` | 目标实例（只用于定位库与在结果里标注），**不会**停服或重启 |
| `dryRun` | 整批在事务里跑一遍再回滚，用来验证脚本而不落库 |

`kitsu_playwright_test`：

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `args` | | `playwright` 之后那串参数，**原样转发**；`["test"]`、`["show-report"]` 等都可以 |
| `cwd` | | 运行路径（必须是目录）；缺省取 `playwrightCwd` |
| `baseUrl` | | `"auto"` 接上唯一在跑的 local 实例；绝对地址直接指定；不传则沿用父进程环境。见[下一节](#跑-test_js-kitsu_playwright_test) |
| `env` | | 追加/覆盖的环境变量（字符串键值对），与注入的 `DOODLE_BASE_URL` 同名时以 `env` 为准 |
| `isolateOutput` | | `true` 时报告与产物落到 `%TEMP%\dsh-kitsu-supplement\playwright\<runId>\`，不覆盖项目目录 |
| `timeoutMs` | | 本次运行的整体预算，缺省取 `playwrightTimeoutMs` |
| `nodePath` | | 本次指定 node |
| `cliPath` | | 本次指定 playwright 入口 |

`kitsu_server_stop`：`serverId` 指定实例，`port` 指定端口，都不给就停掉全部。本插件**不会**杀别人启动的进程。

## 跑 test_js（`kitsu_playwright_test`）

`args` 是 **`playwright` 之后那串参数，原样转发**，插件不改写、不解释：

```text
kitsu_playwright_test { args: ["test"] }                                  # 全部用例
kitsu_playwright_test { args: ["test", "face_capture.js"] }               # 单个文件
kitsu_playwright_test { args: ["test", "-g", "创建设备"] }                 # 按标题过滤
kitsu_playwright_test { args: ["test", "--list"] }                        # 只列用例，不连服务端
kitsu_playwright_test { args: ["show-report"] }                           # 打开上次的 HTML 报告
```

子进程固定是 `node <cwd>\node_modules\@playwright\test\cli.js <args...>`，`cwd` 默认取配置
`playwrightCwd`（`E:\Doodle\test_js`）。**不需要** `test_js/run-playwright.mjs` 那个
fork→spawn 降级补丁——本插件走宿主的子进程通道，普通 fork 可用；那个脚本服务于 pwsh 沙箱。

### 与被测服务端配合

`baseUrl` 只决定给子进程设不设 `DOODLE_BASE_URL`（`playwright.config.js:17`、`test/config.js:6` 都读它）：

| 传法 | 效果 |
| --- | --- |
| 不传 | 完全沿用父进程环境；父进程没有该变量就落到 `http://localhost:50025` |
| `baseUrl: "auto"` | 接上**唯一在跑**的 `local` 实例（`kitsu_server_start` 起的），否则退回复用父进程的 `DOODLE_BASE_URL`；有两个 local 实例时直接报 `invalid-args`，不猜 |
| `baseUrl: "http://127.0.0.1:50136"` | 指到这个地址 |

典型一条龙（用例需要 `--local` 服务时）：

```text
kitsu_server_start { mode: "local", port: 50136, dbPath: "E:\\tmp\\kitsu_test.db" }
kitsu_playwright_test { args: ["test", "face_capture.js"], baseUrl: "auto" }
kitsu_server_stop { port: 50136 }
```

`env` 可以追加环境变量（如 `{ FACE_CAPTURE_TRANSCODE_E2E: "1" }`），同名键以 `env` 为准。

### 结果与失败码

返回里带：`summary`（passed / failed / skipped / didNotRun / flaky，从 line 报告器输出里抓）、
`failures`（每条失败用例的标题 + 错误首行）、`listed`（`--list` 时的用例总数与文件数）、
`reportDir` / `outputDir`、`outputTail`（stdout+stderr 尾巴，溢出时给 `spillPath`）。

失败码：`ok` / `tests-failed` / `busy` / `timeout` / `cancelled` / `spawn-failed` / `timeout-cli` /
`invalid-args` / `exit-nonzero`。`timeout-cli` 表示找不到 `@playwright/test/cli.js`；
`exit-nonzero` 表示退出码非 0 但输出里没有用例计数（命令或配置写错了，先看 `outputTail`）。

素材缺失的用例会 `test.skip()`（见 `test_js/README.md`），所以 `skipped` 非 0 是正常结果，
不等于失败。

### 不可并发

同一时刻只允许一个 `kitsu_playwright_test` 在跑：第二次调用立刻返回 `busy`（写明正在跑的 runId
与参数），**不排队**。原因是用例会往 `test_js/test-results` 与 `test_js/playwright-report` 写，
两个运行互相覆盖。运行期间会在 `<cwd>\.playwright-run.lock` 留一个标记文件（只作提示，
并发判断不依赖它），结束时删除。

要保留上一次的报告，传 `isolateOutput: true`，报告与产物会落到
`%TEMP%\dsh-kitsu-supplement\playwright\<runId>\`，不覆盖项目目录。

### 超时

`timeoutMs` 是**整个运行**的预算（默认 900000，15 分钟），超时后先 `terminate()`，
`playwrightKillTimeoutMs`（默认 5000）内仍未退出就由宿主结束进程树。单条用例的超时仍是
`playwright.config.js` 里的 `timeout: 60000`。

## 典型用法

```text
kitsu_server_start { mode: "local", port: 50123, dbPath: "E:\\tmp\\kitsu_test.db" }
→ ok, serverId=kitsu-1, baseUrl=http://127.0.0.1:50123

kitsu_server_sql { serverId: "kitsu-1", files: ["E:\\seed\\projects.sql"], sql: "INSERT INTO person ...;" }
→ ok, statements=12, changes=34（服务端仍在跑，直接就能查）

kitsu_server_sql { serverId: "kitsu-1", files: ["E:\\seed\\big.sql"], dryRun: true }
→ 先验一遍，确认无误再去掉 dryRun

kitsu_server_status { port: 50124 }

kitsu_playwright_test { args: ["test", "face_capture.js"], baseUrl: "auto" }
→ ok, passed=3 failed=0 skipped=1（playwright-report 已更新）
kitsu_server_stop { serverId: "kitsu-1" }
```

## 失败码

**`kitsu_server_start`**：`invalid-args` / `port-in-use` / `kill-failed` / `spawn-failed` / `exited-before-listen` / `no-shm-port` / `exited-before-ready` / `ready-timeout` / `cancelled`。
失败结果里会带 `shmName`（查的是哪个段）、`outputTail`（stdout + stderr 尾巴）与服务端源码位置提示，`--config` 解析失败会给出 `config/hint` 诊断。

**`kitsu_playwright_test`**：`ok` / `tests-failed` / `busy` / `timeout` / `cancelled` / `spawn-failed` / `timeout-cli` / `invalid-args` / `exit-nonzero`。

**`kitsu_server_sql`**：`ok` / `invalid-args` / `empty` / `sql-failed`。
`sql-failed` 会给出「来源（文件名或 `<inline sql>`）+ 第几条语句 + SQLite 原文 + 截断后的 SQL」，整批已回滚。

## 已知边界

- **local 模式没人传 dbPath 时是内存库**：服务端落 `:memory:`，进程退出即丢，`kitsu_server_sql` 直接拒绝。要用可写的库，启动时传 `dbPath`。
- **生产模式依赖注册表** `HKLM\SOFTWARE\Doodle\MainConfig`（以及 `\Email`）：读不到会抛异常，服务不启动。这属于测试机环境配置，插件不处理。
- **读端口依赖 `node:ffi`。** 宿主 Node 必须是 26.1+ 且带 FFI 支持；`--no-experimental-ffi`、或开了权限模型却没给 `--allow-ffi` 时读不到段，`kitsu_server_start` 会报 `no-shm-port` 并在诊断里给出 `shm/read-failed` 的真实原因。
- **段名按子进程 pid 拼。** 如果 `exePath` 指向 `.bat` 或其它包装器，拿到的 pid 属于包装器而不是 `doodle_kitsu_supplement.exe`，段名会对不上；此时诊断里会同时报 `shm/stdout-mismatch`（stdout 有端口、共享内存没有）。
- 端口是否放通"公开访问"（`public_url_port`）由服务端注册表配置与服务端声明决定，插件只把它写进生成的 config。
- `kitsu_server_sql` 连接时模拟服务端的连接级 PRAGMA（`foreign_keys = ON`、`synchronous = NORMAL`、`recursive_triggers = ON`，见 `sqlite_database.cpp:1310-1312`），但不会加载服务端的 jieba FTS5 分词器；写 FTS5 虚拟表或依赖该分词器的内容请走服务端接口。
- `ATTACH` / `DETACH` 被拒绝：本工具只写目标库文件。
- **`kitsu_playwright_test` 用 `test_js` 自己的 playwright**：`cwd` 下没有 `node_modules/@playwright/test` 时直接报 `timeout-cli`，插件不替你装依赖。
- **playwright 与 kitsu 服务端是两个独立进程**：工具只负责把 `DOODLE_BASE_URL` 递给 playwright，服务端有没有起来由调用方保证（`baseUrl: "auto"` 只接本插件记录的 local 实例）。
- 实例表只在本进程内有效：DSH 重启后插件不再管之前拉起的进程（它们仍在运行），需要用 `kitsu_server_status { port }` 看占用并手工处理。

## 开发

```text
node --check lib/index.js
node test/pure.mjs
```

`test/pure.mjs` 用 `node:vm` 从 `lib/index.js` 的第一个区段（校验、argv、SQL 切分、config、共享内存段名与端口解析、渲染）求值，不依赖 `@deepseek-ai/*` 包。