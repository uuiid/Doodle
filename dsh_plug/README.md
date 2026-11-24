# dsh_plug

Doodle 的 DeepSeek Harness（DSH）插件集合，本目录同时是它们的 **pnpm workspace 根**。

这些插件以 `link:` 方式装进 DSH 的 `web` profile，给 agent 提供 CMake 构建、UE 构建、Kitsu 服务、Everything 检索和 VS Code 选区这几组工具。

---

## 目录结构

```
dsh_plug/
├─ package.json          workspace 根（name: doodle-plugins，private）
├─ pnpm-workspace.yaml   本目录下 6 个包的列表
├─ pnpm-lock.yaml        依赖锁文件
├─ node_modules/         由 pnpm install 生成，只含 dsh-doodle-workspace 的链接
│
├─ dsh-cmake/            CMake 配置 / 构建 / 清理
├─ dsh-kitsu-supplement/ Kitsu supplement 服务 + Playwright 测试
├─ dsh-tool-everything/  Everything 驱动的文件与内容检索
├─ dsh-ue/               Unreal Engine 构建与编辑器
├─ dsh-vscode-sel/       VS Code 选区 → DSH 输入框
└─ dsh-doodle-workspace/ 前两个插件共用的工作区解析库（不是插件）
```

6 个包全部平级，因为 `dsh-cmake` 和 `dsh-kitsu-supplement` 用裸名 `import 'dsh-doodle-workspace'` 引用它，靠 workspace 链接解析，多套一层目录会破坏这个关系。

---

## 包一览

| 目录 | 包名 | 版本 | 类型 | patch 行 id |
|---|---|---|---|---|
| `dsh-cmake` | `dsh-cmake` | 0.1.0 | 插件 | `cmake` |
| `dsh-kitsu-supplement` | `dsh-kitsu-supplement` | 0.1.0 | 插件 | `kitsu-supplement` |
| `dsh-tool-everything` | `dsh-tool-everything` | 0.2.0 | 插件 | `tool-everything` |
| `dsh-ue` | `dsh-ue` | 0.1.0 | 插件 | `ue-build` |
| `dsh-vscode-sel` | `dsh-vscode-sel` | 0.1.0 | 插件 | `vscode-sel` |
| `dsh-doodle-workspace` | `dsh-doodle-workspace` | 0.1.0 | **库** | — |

`dsh-doodle-workspace` 没有 `apply()`、不注册工具、不读 `ctx`，也没有 `cordis.patch.yml`，所以**不会**出现在 DSH 插件页里。这是正常的。

---

## 各插件说明

### dsh-cmake

通过 `build_script/build/set_env.ps1` 驱动 Doodle 项目的 CMake，wrapper 负责激活 MSVC 环境，插件负责传 preset 并把 MSVC/CMake 的诊断解析成结构化结果。

工具：`cmake_presets`、`cmake_configure`、`cmake_build`、`cmake_clean`

默认配置：`maxParallel: 16`（插件自身默认是 8，本 patch 覆盖为 16）。不要再往上调，`cl` 会抢写 obj 报 `Permission denied: cannot generate file`。

### dsh-kitsu-supplement

启动 doodle kitsu supplement 的后端服务，并托管 Playwright 契约测试。

工具：`kitsu_server_start`、`kitsu_server_status`、`kitsu_server_stop`、`kitsu_playwright_test`

可执行文件路径不写在配置里，每次按**发起调用的会话工作区**和 git 分支解析到 `<工作区上一级>/build/<分支>/Ninja_debug/bin/<exeName>`，与 `CMakePresets.json` 的 `binaryDir` 是同一个表达式。默认用 `doodle_kitsu_supplement_test.exe`（Maya / UE 走 stub，无需安装 Maya 或 UE）。

默认配置：

```
db_path:               C:/kitsu_new.database
kitsu_front_end_path:  D:/kitsu/dist
root:                  D:/kitsu_data
public_url_port:       38193
```

### dsh-tool-everything

把 Everything 的索引能力暴露成 `esearch` 工具，文件名和文件内容都能搜。强制要求 `path` 作用域，避免内容检索退化成全盘扫描。

工具：`esearch`

默认配置：`scopes: [E:\Doodle]`

### dsh-ue

通过 `Engine\Build\BatchFiles\Build.bat` 编译 UE 目标，或直接起编辑器。UBT 的失败经常仍以 exit code 0 结束，所以插件会解析 MSVC/UBT 日志给出权威结论。

工具：`ue_targets`、`ue_build`、`ue_editor`

默认配置：

```
repoRoot:       E:\Doodle
engineRoot:     E:\UnrealEngine
projectFile:    E:\ue_main\ue_main.uproject
defaultTarget:  UnrealEditor
```

### dsh-vscode-sel

在 VS Code 扩展和浏览器里运行的 DSH 页面之间做一次性的选区传递：扩展 POST 到 `/api/dsh-vscode-sel/push`，页面在窗口获得焦点时拉 `/api/dsh-vscode-sel/pending`，把 `@路径:行号` 插进光标处。

信箱是进程内内存，有长度上限、60 秒过期、第一次读取即清空。

### dsh-doodle-workspace

不是插件，是被 `dsh-cmake` 和 `dsh-kitsu-supplement` 共同 import 的一组纯函数：解析会话工作区、读 git 分支、算出构建目录、判断构建目录归属。

两处引用它的写法是**裸名**：

```js
import { ... } from 'dsh-doodle-workspace'
```

因此必须靠 workspace 链接让它可解析 —— 这就是本目录作为 workspace 根存在的原因。

---

## 安装与维护

### 装依赖

在本目录执行，**不要**在 `E:\Doodle` 执行（那里已经没有 workspace 根了）：

```powershell
cd E:\Doodle\dsh_plug
pnpm install
```

### 接入 DSH profile

`C:\Users\TD\.dsh\profiles\web\package.json` 里用 `link:` 引用这 6 个包：

```json
"dsh-cmake":              "link:E:/Doodle/dsh_plug/dsh-cmake",
"dsh-doodle-workspace":   "link:E:/Doodle/dsh_plug/dsh-doodle-workspace",
"dsh-kitsu-supplement":   "link:E:/Doodle/dsh_plug/dsh-kitsu-supplement",
"dsh-tool-everything":    "link:E:/Doodle/dsh_plug/dsh-tool-everything",
"dsh-ue":                 "link:E:/Doodle/dsh_plug/dsh-ue",
"dsh-vscode-sel":         "link:E:/Doodle/dsh_plug/dsh-vscode-sel"
```

5 个插件同时列在 `dsh.profile.bundles` 里（`dsh-doodle-workspace` 不在，因为它没有 patch 层）。

改完 profile 的依赖后要在 profile 目录跑一次 `pnpm install`，然后**重启** DSH 才会加载新的模块。

### 改代码之后

| 改了什么 | 需要做什么 |
|---|---|
| 插件的 `lib/` | 重启 DSH（新模块不会热替换） |
| 插件的 `cordis.patch.yml` | 重启 DSH |
| `package.json` 的依赖 | `cd E:\Doodle\dsh_plug && pnpm install`，再重启 |

### 自检

各包自带 Node 自检，不需要装 DSH 也能跑：

```powershell
cd E:\Doodle\dsh_plug\dsh-cmake              ; node test\load-graph.mjs
cd E:\Doodle\dsh_plug\dsh-kitsu-supplement   ; node test\load-graph.mjs
cd E:\Doodle\dsh_plug\dsh-doodle-workspace   ; node test\pure.mjs
cd E:\Doodle\dsh_plug\dsh-ue                 ; node test\pure.mjs
```

也可以在各包里跑 `pnpm test`。`dsh-cmake/dsh-kitsu-supplement/dsh-doodle-workspace` 的 `test/load-graph.mjs` 会用 `../../dsh-doodle-workspace/lib/workspace.js` 这种相对路径，所以这几个包必须保持平级。

`dsh-tool-everything/test/` 下是 Playwright 相关的端到端脚本和一个 LLM overlay 配置，需要真实环境才能跑。

---

## 机器相关配置

几个路径是**按这台机器写死**的，换机器要改：

| 位置 | 键 | 当前值 |
|---|---|---|
| `dsh-ue/cordis.patch.yml` | `repoRoot` | `E:\Doodle` |
| `dsh-ue/cordis.patch.yml` | `engineRoot` | `E:\UnrealEngine` |
| `dsh-ue/cordis.patch.yml` | `projectFile` | `E:\ue_main\ue_main.uproject` |
| `dsh-tool-everything/cordis.patch.yml` | `scopes` | `E:\Doodle` |
| `dsh-kitsu-supplement/cordis.patch.yml` | `db_path` / `root` / `kitsu_front_end_path` / `public_url_port` | 见上文 |

注意 `repoRoot` 和 `scopes` 指向的是**仓库根** `E:\Doodle`，不是插件目录，插件搬到 `dsh_plug/` 之后这两个值不需要改。

改这些配置有两个办法：

1. 直接改对应包的 `cordis.patch.yml`（改动会进 git）
2. 在 profile 的 `cordis.patch.yml` 里按行 id 覆盖（只影响本机）

用第 2 种办法时必须**整段重述** `config`，patch 是替换整个 `config` 值而不是逐键深合并：

```yaml
- id: ue-build
  name: dsh-ue
  config:
    repoRoot: D:\another\Doodle
    engineRoot: E:\UnrealEngine
```

---

## 与 Doodle 主仓库的关系

- 本目录在 `E:\Doodle` 仓库内，6 个包的源码**受 git 跟踪**。
- Doodle 的 C++ 部分（`src/`、`CMake/`、`build_script/`）与本目录**无依赖关系**：C++ 构建不读这里的任何文件，插件只调用仓库里已有的 `CMakePresets.json` 和 `build_script/build/set_env.ps1`。
- `test_js/` 是**独立安装**的 npm 项目（自带 `node_modules`），不属于本 workspace，`pnpm install` 不会碰它。
- `node_modules/` 和 `pnpm-lock.yaml` 由 pnpm 生成；`.gitignore` 里对 `node_modules/` 的忽略在任何层级都生效。
