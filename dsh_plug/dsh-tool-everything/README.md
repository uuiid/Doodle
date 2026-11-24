# dsh-tool-everything

Everything 驱动的文件/内容检索工具，作为 DSH（DeepSeek Harness）插件提供给模型。

模型侧得到一个工具 **`esearch`**：既能按文件名检索，也能按**文件内容**检索，走 Everything
索引（Windows 全盘文件名索引 + 可选内容索引），比遍历文件系统快几个数量级。

```
esearch(pattern="*.cpp", ext="cpp", path="E:\\Doodle\\src")
esearch(pattern="retarget_rotations", content=true, path="E:\\Doodle\\src")
esearch(pattern="content:<charge_on_violation task_status>", rawQuery=true, path="E:\\Doodle\\src")
```

## 为什么需要它（以及它防的坑）

Everything 的 `content:` 在**不限定路径**时会退化为实时全盘内容扫描。实测中这会把磁盘打满、
让机器卡死——不是"慢"，是"不可用"。所以本工具把"范围"做成硬要求：

| 情形 | 行为 |
|---|---|
| 未给 `path`，配置了 `scopes` | 用第一个 scope |
| 未给 `path`，未配置 `scopes` | 用会话 workspace（`process.cwd()`） |
| 三者都没有 | **报错拒绝**，绝不发出无范围查询 |
| `path` 是盘符根（`E:\`）或 UNC 根 | **报错拒绝**（等同全盘） |

范围只由参数与配置决定；模型若自己在 `pattern` 里写了 `path:`（Everything 原生语法），
则视为"已限定范围"直接透传，不再叠加。

## 安装

```powershell
# 安装到某个 profile（会自动加入该 profile 的 dsh.profile.bundles）
dsh plugin --profile web add 'file:E:\Doodle\dsh-tool-everything'
```

插件自带 `cordis.patch.yml`，其中已包含一份默认配置（`scopes: [E:\Doodle\src]`）；
安装时 DSH 会校验并把该 patch 写入 profile 的 patch 层。

**工具在宿主进程重启后才会进入模型可见的工具集**——装完需要重启 dsh（Web 端即重启宿主）。

> 改了插件源码（`lib/`）后必须重新 `dsh plugin add` 一次：profile 的 `node_modules` 里放的是
> **拷贝**（按 `package.json` 的 `files` 白名单），不是软链——直接改源文件不会生效。

## 配置

在 profile 的 `cordis.patch.yml` 里覆盖（或直接改插件自带的那份）：

```yaml
- id: tool-everything
  name: dsh-tool-everything
  config:
    endpoint: http://127.0.0.1:8099/   # Everything HTTP 服务
    esPath: E:\Doodle\lib\es.exe       # 可选：HTTP 失败时兜底（沙箱下可能被拒）
    scopes:                            # 模型未给 path 时的默认范围
      - E:\Doodle\src
    maxResults: 50                     # 单次返回上限（硬上限 500）
    timeoutMs: 30000                   # 单次查询预算
    guidanceOrder: 1390                # 系统提示词里本工具指引段的排序值
```

### 让模型优先用 esearch（优先级）

DSH 里“工具优先级”是三个互相独立的旋钮，本插件负责其中两个：

| 旋钮 | 位置 | 本插件的行为 |
|---|---|---|
| 系统提示词**指引段**的排序 | 插件内 `guidanceOrder` | 默认 `1390`：排在 `TOOL_EDIT`(1300) 之后、`TOOL_GLOB`(1400)/`TOOL_GREP`(1500) 之前 |
| **工具清单**（发给模型的 schema 列表）里的顺序 | 宿主 profile 的 `system-prompt` 条目 `toolOrder` | 未设置 → 按工具名字典序；`esearch` 天然排在 `glob`/`grep` 之前 |
| 指引文本本身的说服力 | 插件内（固定文案） | “用 esearch 而不是 glob/grep/命令行检索，每次都要给 `path`” |

插件注册一段名为 `tool:esearch` 的提示词指引（与内置 `tool:glob` / `tool:grep` 完全同机制，
`ctx.systemPrompt.section()`），并且**只在 `esearch` 真的注册进可见工具集时才渲染**；
文案会根据 `glob`/`grep` 是否存在自动调整要对比的工具名。

**把检索指引顶到所有工具指引之前**——`guidanceOrder` 设 `999`（`TOOL_BASH` 是 1000）：

```yaml
- id: tool-everything
  name: dsh-tool-everything
  config:
    guidanceOrder: 999
```

**让 `esearch` 在工具清单里也排第一**——这是宿主配置，写在 profile 的 `cordis.patch.yml`：

```yaml
- id: system-prompt
  name: '@deepseek-ai/dsh-system-prompt'
  config:
    toolOrder:
      - esearch
      - "<unlisted-tools>"
```

`<unlisted-tools>` 是必填的占位符（其余工具按字典序插在该位置）；列了未注册的工具名会让
提示词装配直接报错，所以只在确定该 profile 装了本插件时启用。

### 传输选择

1. **HTTP（默认）**：`GET {endpoint}?search=<query>&json=1&path_column=1&count=N`。
   0.1s 级，返回绝对路径；不需要额外进程，也不受命名管道限制。
2. **es.exe（可选兜底）**：仅当 HTTP 失败且配置了 `esPath` 时使用。es.exe 通过命名管道与
   Everything 通信，在受限沙箱下会"拒绝访问"，且在 DSH 里用 `spawn` 起子进程也可能被限制——
   所以它只是第二选择。

### 前置条件

Everything 1.5 需在运行，且 **HTTP 服务器已启用**（工具 → 选项 → HTTP 服务器，端口 8099）。

内容检索建议启用内容索引并**收窄范围**，否则首次索引会吃掉大量内存：

```ini
content_indexing_enabled=1
content_indexing_include_only_folders=E:\Doodle\src   ; 不要写 E:\Doodle 全仓库
content_indexing_exclude_folders=E:\Doodle\vcpkg;E:\Doodle\external;E:\Doodle\build;E:\Doodle\.git
content_indexing_include_only_files=*.c;*.cc;*.cpp;*.h;*.hpp;*.py;*.js;*.ts;*.md;...
content_indexing_max_size=16
content_indexing_max_size_unit=3
```

实测：`E:\Doodle\src` 下 628 个 cpp/h，索引建完仅占 905MB；而 `E:\Doodle` 全仓库
（含 vcpkg/external，约 46 万 cpp）会吃到 15GB。文件在 `%APPDATA%\Everything\Everything.ini`。

## 工具参数

| 参数 | 类型 | 说明 |
|---|---|---|
| `pattern` | string（必填） | 文件名模式，或 `content=true` 时的内容关键字。Everything 语法透传 |
| `path` | string | 搜索范围目录；缺省回退配置 scopes → 会话 workspace |
| `content` | boolean | 走内容检索（`content:`） |
| `regex` | boolean | 正则（Everything `regex:`；**必须带 `path`**，否则全盘扫描） |
| `caseSensitive` | boolean | 区分大小写（`case:`） |
| `ext` | string | 扩展名过滤，分号分隔（如 `cpp;h`） |
| `foldersOnly` | boolean | 只返回目录（`/ad`） |
| `rawQuery` | boolean | 把 `pattern` 当完整 Everything 查询，只补范围 |
| `maxResults` | number | 返回条数（默认 50，硬上限 500） |

输出为绝对路径（便于直接接 `read`/`grep`/`lsp`），并附带 `total` / `truncated` / `transport`。

## 验证

```powershell
# 纯函数单测（范围护栏、查询构造、结果归一化、FILETIME 转换等）
node test/pure.mjs

# 端到端：在临时 headless profile 里真实调用一次工具
./test/e2e.ps1
```

端到端脚本会创建一个临时 profile、挂载插件、跑一次会话并断言工具返回了期望文件与
两道护栏；用完自动删除临时 profile，不影响任何现有 profile。

## 已知限制

- 仅 Windows（依赖 Everything）。
- 内容索引未覆盖的旧文件走按需磁盘扫描——因此**务必**限定 `path`。
- `es.exe` 兜底在受限沙箱下不可用（命名管道受限）。
- 首次内容索引期间会占用较多内存与 IO（见上文"前置条件"）。
