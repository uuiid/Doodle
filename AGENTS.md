# Doodle 工作区指令

本文件是 agent 在本仓库工作时的**第一入口**。它只承载「每次动手前都必须知道、且在本仓库会与默认做法不同」的内容。
其余细节一律**引用**已有文件，原文永远优先于本文件的转述。

修改本文件前先读 [.github/copilot-instructions.md](.github/copilot-instructions.md)：两者受众重叠，不要制造两份互相漂移的规范。
C++ 规范的完整版（豁免清单、正反例、踩坑记录）在 [docs/cpp_guidelines.md](docs/cpp_guidelines.md)，本文件只摘要其中必须遵守的部分。

---

## 1. 不可协商项

这些是硬约束，违反即为错误改动：

- **Git 提交一律由用户执行。** 完成改动后只报告：改了什么、验证结果、当前处于未提交状态。不要 `git commit` / `git push`，也不要主动提议代提交。
- **默认不测试。** 没有用户明确要求时，不要运行任何测试、不要构建测试目标、不要跑测试验证改动；验证手段只有编译通过。
  **只有用户主动明确需要测试时**，才按 §2「按需测试流程」先用 `kitsu_server_start` 拉起 **`doodle_kitsu_supplement_test.exe`**（唯一的测试服务入口，不传 `exeName` 即为它），再在 `test_js/` 里编写并运行测试；数据不足时用 `kitsu_server_sql` 补。
- **不要跑发布构建**：`Ninja_release` / `Release_*` / `package_release` 耗时过长，用户已明确要求停用。
- **依赖只经 `vcpkg.json` 管理。** 自定义 port 放 `vcpkg_ports/`，triplet 放 `vcpkg_triplets/`。不要在 `CMakeLists.txt` 里硬编码外部依赖路径。
- **HTTP 处理器的声明与实现必须用宏**，见 §4。手写完整签名是禁止的。
- **没有明确说明，不写注释。** 代码里默认不出现任何注释；只有任务中明确要求写注释时才写。
- **不修旁支。** 只做请求范围内的改动；顺手发现的其它缺陷先报告，由用户决定改不改。

## 2. 构建与测试

**搜索入口唯一：Everything（`esearch` 工具）。** 文件名与内容搜索一律走 `esearch`（必须限定 `path` 范围），不要用 `pwsh` 拼 `Get-ChildItem -Recurse` / `Select-String` 遍历仓库——全量递归会让 shell 卡住甚至被用户强杀。`glob` / `grep` 只在 `esearch` 无结果或 Everything 不可用时兜底。

**构建入口唯一：dsh-cmake 插件的工具。** 不要自己拼 `cmake` 命令行，不要手工激活 VS 环境，不要用 `--fresh`。

| 工具 | 作用 |
| --- | --- |
| `cmake_presets` | 查询可用 preset（只读，不确定用哪个先调它）|
| `cmake_configure` | `cmake --preset <name>` |
| `cmake_build` | 传 build preset（如 `debug_doodle`）|
| `cmake_clean` | 清理构建产物 |

| 目的 | configure preset | build preset |
| --- | --- | --- |
| 独立软件 / 测试 | `Ninja_debug` | `debug_doodle`（`doodle_kitsu_supplement` + `doodle_kitsu_supplement_test`）|
| Maya 插件 | `Ninja_RelWithDebInfo` | `debug_maya`（`doodle_maya`）|

插件另有一条独立约束：**保持 `parallel` 默认值**，调高并行度会让 `cl` 抢写 obj 报 `Permission denied: cannot open compiler generated file`。工具的 `ok` 字段是权威结论，失败先看 `diagnostics` 给出的文件与行号。

插件内部经 `build_script/build/set_env.ps1` wrapper 调用 cmake——它激活 vcvars64 并在仓库根目录执行。**非 DSH 环境**（CI、编辑器任务、其他 agent）没有这些工具，需自行经该 wrapper 调用 cmake。相关路径解析见 [CMakePresets.json](CMakePresets.json) 与 [CMake/](CMake/) 下的 Find 模块。

其余 preset 的权威定义在 [CMakePresets.json](CMakePresets.json)；改构建步骤优先改该文件与 `CMake/` 下的 Find 模块，不要在 `CMakeLists.txt` 里写死平台路径。

- **没有 C++ 单元测试**：数据库 ORM 与升级（`sqlite_*` 系列）、视频图片文件解码、网络客户端 `src/doodle_lib/http_client/**`、HTTP 处理器、Maya 插件等路径**都没有**单元测试，改动后不要期待 `doodle_kitsu_supplement_test` 会替你验证。
  HTTP 契约测试走 §2「按需测试流程」拉起的 `doodle_kitsu_supplement_test` 服务 + `test_js/` 下的 Playwright 用例。
  `src/test/**` 的既有内容不要改动，也不要为了新功能往那里加测试；该目录里唯一允许维护的是测试启动器与
  [test_exe_runner.h](src/test/test_exe_runner.h) / [test_exe_runner.cpp](src/test/test_exe_runner.cpp) 这一对 Maya / UE stub。
- Maya 版本由 CMake 变量 `Maya_Version` 驱动，条件分支见 `src/CMakeLists.txt`；扩展新版本优先改 `CMake/FindMaya.cmake`。

### 按需测试流程

**只在用户主动明确要求测试时才走这条流程**（见 §1「默认不测试」）。没有明确要求时，验证一律只到编译通过。
整个流程的**第一步永远是拉起 `doodle_kitsu_supplement_test` 进程**：先有服务，再谈写用例、灌数据、跑 Playwright。

1. **先拉起 `doodle_kitsu_supplement_test` 进程。** 用 `kitsu_server_start`（dsh-kitsu-supplement 插件），**不要传任何路径**：工具按当前会话的工作区与 git 分支解析出
   `<工作区上一级>\build\<分支>\Ninja_debug\bin\<exeName>`（与 `CMakePresets.json` 的 `binaryDir` 同一表达式），默认 `exeName` 就是
   `doodle_kitsu_supplement_test.exe`；`doodle_kitsu_supplement.exe` 只在确需真实 Maya / UE 外部进程时才用
   （那时传 `exeName: "doodle_kitsu_supplement.exe"`）。**解析不出工作区、分支或 exe 时工具会直接失败并结束当前回合**，
   此时必须停下、把细节交给用户检查，不要改传别的路径重试。测试 exe 与生产 exe 共用同一份 `doodle_lib`、路由完全一致，但
   [test_exe_runner.h](src/test/test_exe_runner.h) 注入的 runner 把 Maya / UE 子进程换成 stub：**不需要本机装 Maya、UE 或其插件**，
   也不要为了测试去配这些环境。返回里的 `exeKind` 应为 `test`，若不是说明这次选错了 exe。
   - `port` 必填，落在 50030–50330；`mode: "local"` 起本地服务，不给 `dbPath` 就是内存库（进程退出即丢，写不进数据）。
   - **要跑有数据依赖的用例就显式传 `dbPath`**，指向一个磁盘库；否则 `kitsu_server_sql` 无处可写。
   - 返回的 `port` / `baseUrl` 取自进程 stdout 的真实监听端口，**测试脚本一律引用它**，不要猜端口。
   - 端口被占时工具默认失败并报占用 PID；确认那是上一次的残留才传 `killExisting: true` 强杀。
   - 服务起不来看 `kitsu_server_start` 的返回诊断与 `<root>\log`；测试 exe 的初始化日志里应有 stub 生效的痕迹，Maya / UE 相关依赖报错说明跑错了 exe。
2. **写测试放到 [`test_js/`](test_js/)。** Playwright + `request` fixture 打 HTTP 契约，不启浏览器；`baseURL` 取环境变量 `DOODLE_BASE_URL`（`playwright.config.js`、`test/config.js` 都读它），把它设成上一步的 `baseUrl`。目录约定、环境变量与既有文件清单见 [test_js/README.md](test_js/README.md)。**不要往 `src/test/**` 加测试**（那是 C++ 单元测试）。
3. **数据不足时补数据**，两条通道按下面 §2.1 选，**都不要为它停服或重启**。
4. **测完停服。** `kitsu_server_stop` 停掉本次拉起的实例，别把监听端口和进程留给下一次。
5. **编译前必须先停服。** 服务进程持有 `bin\doodle_lib.dll`，不停服直接 `cmake_build` 会在链接阶段报
   `LNK1168: cannot open bin\doodle_lib.dll for writing`，整次构建失败。顺序固定为
   **`kitsu_server_stop` → `cmake_build` → 需要时再 `kitsu_server_start`**；改完 `doodle_lib` 里的代码要复测时，
   也必须先停服再编译、编译完再重启，不能指望运行中的进程换掉已加载的 DLL。

`kitsu_server_status` 可随时查本插件记录的实例与端口占用。本插件只杀自己拉起的进程。

### 2.1 改测试数据库：两条通道怎么选

补测试数据有两条完全不重叠的通道，**选错会得到"数据明明写进去了却搜不到"的假故障**。

| | `kitsu_server_sql` | `POST /api/doodle/debug/sql` |
| --- | --- | --- |
| 连接 | 工具自己另开 SQLite 直连 `dbPath` | **服务端自己的连接**（strand 上执行） |
| jieba 分词器 | ❌ 没注册，`MATCH` 不可用 | ✅ 与业务代码同一份 |
| 外键 / 触发器 / PRAGMA | 需自行保证 | ✅ 与 `run_sql` 完全一致 |
| 事务 | 整批单事务 | `BEGIN IMMEDIATE` → SQL → `COMMIT`/`ROLLBACK` |

**默认用 `kitsu_server_sql`**：纯 DDL、灌种子数据、批量修正都不涉及全文检索语义。
直接对 `dbPath` 指向的库执行 SQL 文件或片段，写完运行中的服务端即可查到。不确定脚本对不对就先 `dryRun: true`。
整批是单事务，失败会整体回滚并报出「来源 + 第几条 + SQLite 原文」。
它自己的连接**不带** jieba tokenizer，也没跑 `register_custom_extension` 那三条 PRAGMA；
所以它只适合不依赖检索语义的写入，别用它验证 `MATCH`。

**只在测 FTS / 全词检索时用 debug 路由**：分词结果、`MATCH` 命中、触发器内建索引这些行为只有进程内连接才能复现。
它是 [`src/test/test_debug_sql.cpp`](src/test/test_debug_sql.cpp) 里的 `debug_sql`，只注册在 `create_test_route()`，
**只有 `doodle_kitsu_supplement_test.exe` 有这个 URL，生产 exe 没有**。

```bash
curl -X POST "$BASE/api/doodle/debug/sql" -H "Content-Type: application/json" \
  -d '{"sql":"INSERT INTO t(body) VALUES(\"狙击枪\"); SELECT rowid FROM t WHERE t MATCH \"枪\";"}'
```

- 请求体 `{"sql": "..."}`，`sqlite3_exec` 原生跑多语句，不用自己切分号。无鉴权。
- 可选 `"transaction": false`（不包事务）、`"stop_on_error": false`（错误也继续）。
- 响应含 `ok` / `rolled_back` / `error{code,name,message}` / `changes` / `total_changes` /
  `last_insert_rowid` / `elapsed_ms` / `row_count` / `rows`。SQL 出错时 HTTP 是 **400**，
  `error.message` 是 SQLite 原文。
- 成功时 HTTP 状态码是 **201 不是 200**：框架 `body_status()` 把 POST + `ok` 映射成 `created`，是既有行为，别当成失败。

写 `entity` 这类带 `entity_fts` 的业务表时记住两点：

- **走 `INSERT/UPDATE entity`，不要直接写 `entity_fts`。** `entity_fts` 是 `content=<entity>` 的 external content 表，
  本身不存原文；三个触发器负责同步索引，直接写它会和 `entity` 对不上。
- **外键必须真实存在**，否则整批回滚（错误码 787 `FOREIGN KEY constraint failed`）。空库里没有 project / entity_type
  记录时，先在同一个请求里造父行，或改用独立 FTS 表验证分词器本身。

### vcpkg 全量重编的三个诱因

以下任一变化都会让 vcpkg ABI 失效、依赖全量重编（代价可达数小时），动手前先想清楚：

1. **MSVC 工具集版本**变化。`Ninja_build` preset 不再声明 `toolset`，工具集完全由 wrapper 调用的 `vcvars64.bat` 决定（当前 `14.44.35207`，即 v143）；换机器或换 VS 安装组件都可能改变它。
2. **PowerShell 版本**变化。它经 vcpkg-cmake helper 级联进所有包的 ABI 哈希。
3. 二进制缓存丢失。当前 `VCPKG_DEFAULT_BINARY_CACHE` 为 `E:\vcpkg_cache\archives`。

排查方法：`vcpkg_abi_info.txt` 只记录 `cmake` / `powershell` / `triplet`，**不含编译器**；编译器要看 `CMakeCache.txt` 的 `Z_VCPKG_CL`、`CMakeFiles/rules.ninja` 里的 cl.exe 绝对路径，以及 `vcpkg/compiler-file-hash-cache.json`。

## 3. 代码风格

分层：`doodle_core`（底层核心）→ `doodle_lib`（可复用库）→ `exe_*`（可执行入口）/ `maya_plug*`（Maya 插件）。新增公共 API 进 `doodle_lib`，内部实现进 `doodle_core`。

- **格式化交给工具**：[.clang-format](.clang-format)，Google 基线、`ColumnLimit: 120`、`AlignConsecutiveAssignments`、`AlignAfterOpenBracket: BlockIndent`、`IncludeBlocks: Regroup`。include 顺序**由该文件的 IncludeCategories 决定**（mimalloc → doodle_core → doodle_app → doodle_lib → boost → maya_plug），不要凭习惯排；只在代码风格与本文件冲突时才人工干预格式。
- **静态检查交工具**：[.clang-tidy](.clang-tidy)，`abseil-* / bugprone-* / google-* / misc-* / modernize-* / performance-* / portability-* / readability-*`，且 `WarningsAsErrors: " "`——命中即为错误。豁免项及其理由写在该文件头部注释里；要加新的豁免请连理由一起补，不要在代码里 `// NOLINT` 绕开。
- **技术底座**：C++20（`CMakeLists.txt` 的 `CMAKE_CXX_STANDARD`）。协程是一等公民，异步逻辑优先用协程而非回调。

### 3.1 命名

| 前缀/后缀 | 含义 |
|---|---|
| `l_` | 局部量（local）|
| `in_` | 入参（input）|
| `io_` | 读写入参（in-out），**不得用来做出参**，见 §3.2 |
| `_`（尾随）| 类成员，如 `state_`、`uuid_id_` |
| `k_` | `constexpr` / `static const` 常量 |
| `out_` | **已禁止**：出参本身就不允许 |
| `p_` / `r_` | 只在 `ai/`、`usd/` 等移植自第三方的数学库中沿用，新代码不用 |

函数与类型一律 `snake_case`（含类名与工厂函数），不写 `GetValue()` 这类驼峰；例外只有对第三方接口的 `override`（Maya `MPxNode::compute`、Asio `async_*` 等按 SDK 规定）。私有成员函数同样带尾随 `_`。类型用 `_t` 后缀表达派生/别名，配置类结构体用 `_arg` / `_option` / `_info` 后缀（如 `export_fbx_arg`、`capture_option`、`camera_info`）。

### 3.2 接口形态

- **不用非常量引用/指针做出参，一律返回值。** 即使包一层结构体显得啰嗦也要用返回值。
  协程下返回 `awaitable<结果结构体>`（如 `awaitable<spec_branch_result>`），而不是 `awaitable<void>` + 非常量引用。
  读到一半看到 `T& out_xxx` / `T* out_xxx`：先读函数体——有一半其实是漏了 `const`。
- **纯输入参数用 `const T&`**（字符串、`FSys::path`、容器、结构体），小整数与枚举按值。不引入 `string_view` / `span` / GSL 改造既有签名，理由见 [docs/cpp_guidelines.md](docs/cpp_guidelines.md) §2。
- **结果结构体字段全部带默认值**（`std::int32_t width_{0};`），聚合初始化与 `{}` 都能用；只读结果用 `friend to_json` 输出，不另写一个"转 JSON"函数。
- 成员初始化优先用**类内默认值**，构造函数只做必须做的事。

### 3.3 失败一律走异常

Doodle **没有** `bool ok_` / `error_code_` / `message_` 三件套式的返回值，也不用 `std::optional` 表示失败原因。

- 出错直接抛 `doodle_error`；在 HTTP 路由或会影响响应码的地方用 `DOODLE_CHICK_HTTP(cond, status, fmt, ...)`，纯内部逻辑用 `DOODLE_CHICK(cond, fmt, ...)`（[exception.h](src/doodle_core/exception/exception.h)）。两者都支持 fmt 参数，不必先 `fmt::format`。
- 缺少状态码的异常一律落成 500 —— 前端拿到的「用户输入不合法」会伪装成「服务器炸了」，所以该有 4xx 的地方必须写 `DOODLE_CHICK_HTTP`。
- **不要吞异常转成布尔/错误码**；**不要**把 `catch (...)` 当作"把失败变成返回值"的手段。
- 允许捕获的场合只有：给它加上下文后再抛（`throw_exception`/`throw;`）、边界处必须有人兜底（HTTP 会话层 `http_session_data.cpp`、线程入口 `app_service.h`、协程里的线程池回调）、或纯日志降级（如流已经断开，丢一帧不如打一条 WARN）。
- **协程里不能 `co_await` 在 catch 块内**（标准禁止）。需要"记下原因再抛"时，先在 catch 里存到局部量，出了 catch 再处理，最后抛出。

### 3.4 枚举与 JSON

- 一律 `enum class`；`magic_enum` 做数值/名字转换，不要手写 `switch` 转字符串。
- **枚举 ↔ JSON 用 `DOODLE_JSON_SERIALIZE_ENUM_STRICT`**（[json_warp.h](src/doodle_core/lib_warp/json_warp.h)），紧跟在枚举定义之后，映射表列全所有取值：

  ```cpp
  enum class preview_file_statuses { processing, ready, broken };
  DOODLE_JSON_SERIALIZE_ENUM_STRICT(
      preview_file_statuses, {
                                 {preview_file_statuses::processing, "processing"},
                                 {preview_file_statuses::ready, "ready"},
                                 {preview_file_statuses::broken, "broken"},
                             }
  )
  ```

  映射表里**手写 JSON 字符串**（上游 `NLOHMANN_JSON_SERIALIZE_ENUM` 宏会把值序列化成数字，本项目不用它）；未映射的取值两个方向都抛 `out_of_range.410`，不要用"静默落到首项"的宏。JSON 键名一旦进契约就等同于接口，改名等于改接口——先搜前端、测试与 `docs/api/`。
- 结构体 ↔ JSON 用 `friend to_json` / `friend from_json`。
- **不要写 `nlohmann::json{x}` 把结构体转成 json**：花括号优先选 `initializer_list` 构造，结果是一个**数组**，值被塞进第一项。正确写法是拷贝初始化或赋值：

  ```cpp
  nlohmann::json l_json = l_status;                         // ✓ 对象
  l_json["session_id"]  = uuid_string(l_session_id);        // 之后可按对象补键
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = (co_await l_session->stop()));  // ✓ 表达式位
  ```

  `nlohmann::json{{"k", v}}` 才是对象字面量；`nlohmann::json{wrapper}` 也是数组，除非 `wrapper` 自带 `to_json` 且你是**故意**走拷贝初始化。

## 4. 分区域规则

工作涉及下列路径时，**先读对应的规则文件**：

- HTTP 处理器（`src/doodle_lib/http_method/**`）
  - [.github/instructions/http-fun-macro-signature.instructions.md](.github/instructions/http-fun-macro-signature.instructions.md) —— 声明用 `DOODLE_HTTP_FUN` / `DOODLE_HTTP_JWT_FUN` + `DOODLE_HTTP_FUN_OVERRIDE`，实现一律 `DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(类名, 方法名)`。
  - [.github/instructions/http-put-update-standard.instructions.md](.github/instructions/http-put-update-standard.instructions.md) —— `put` 必须用 `orm::update` 链式 + `run_sql`，禁止「读取实体 → 改写 → 整体写回」。
- UE 插件（`UnrealEngine/Engine/Plugins/Doodle/**`）
  - `E:\Doodle\UnrealEngine\Engine\Plugins\Doodle` 是指向 `E:\Doodle\script\uePlug\Doodle` 的 junction，两处路径等价、是同一份文件。用哪条路径读写都可以，改动只需做一次。

新增或修改 HTTP 路由后，确认它在 `kitsu.cpp` 一类的注册表里有对应 `reg_t<>`：未注册的 route 在运行期不可达，改了也不生效。

## 5. 超出本文件范围的事

功能语义、业务流程、各模块的域知识在 [README.md](README.md) 与 `docs/` 下；`docs/api/` 是接口文档。
`external/`（USD、libtorch 等）与 `vcpkg/` 是第三方内容，**不要改动**，它们各自带自己的 `AGENTS.md`。
