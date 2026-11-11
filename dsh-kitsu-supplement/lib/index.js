/**
 * dsh-kitsu-supplement —— 拉起 doodle_kitsu_supplement.exe HTTP 服务（宿主端）
 *
 * 三个工具：
 *   kitsu_server_start  按 production / local 模式起服务，报告真实监听端口与 baseUrl
 *   kitsu_server_status 看本插件记录的实例，以及端口被谁占着
 *   kitsu_server_stop   停本插件拉起的实例
 *
 * 每条设计都对应服务端已核实的行为（行号写在下面对应的函数注释里）：
 *
 * 1) 端口必须显式给，且必须落在 50030-50330 之内（端口段由本机测试环境约定）。
 *    local 模式不给 --port 时服务端会 listen(0)（随机端口），
 *    而 local 分支同样不防重复实例（kitsu_supplement.cpp:212 的 stop_previous_instance 已注释），
 *    随机端口意味着每次重启地址都变，测试用例没法引用。所以缺 port 或越界都直接报错。
 * 2) 监听端口就是入参 port，不再去服务端的共享内存段（`doodle_http_listener_port_<pid>`）里找：
 *    宿主的 ctx.subprocess.spawn 不返回 pid，读段只能靠 netstat 反查 pid，而沙箱下 netstat
 *    看不到宿主拉起的进程，整条路在受限环境里必然失败。就绪与否直接对
 *    `http://127.0.0.1:<port><readyPath>` 轮询，任何 HTTP 状态码都算就绪，最多等 READY_TIMEOUT_MS。
 * 3) 固定端口先探占用。默认失败并给出占用 PID，只有显式 killExisting:true 才强杀。
 * 4) --config 由插件生成。服务端 config 解析失败会 return true 且**不启动 HTTP**，
 *    进程可能一闪而过，所以 JSON 只由插件写，模型的额外字段并进 extraConfig。
 *    带值开关一律写成 `--key=value` 单个 argv 项（与 .vscode/launch.json 的用法一致）：
 *    argh 把 `--key=value` 记进 params_，`arg_({"key"})` 先查 params_ 再查 flags_，能取到值；
 *    而 `--key value` 里只有 `--key` 是选项、value 掉进位置参数，arg_ 得到空串——
 *    服务端 `--port` 空串会 lexical_cast 抛异常（监听之前就退出），`--root` 空串会把会话目录落到 cwd。
 * 5) cwd 默认取 exe 所在目录（exe 依赖同目录的 dll）；运行路径也可由参数/配置显式传入。
 * 6) 不用服务端的 POST /api/doodle/stop-server：它要 admin JWT，且连接数不为 0 时拒绝退出
 *    （base_data.cpp:343-366），测试机上的连接数恰恰不为 0。这里直接终止进程树并等停稳。
 *
 * 本文件按 DSH 插件惯例不使用顶层 node: 静态导入；内置模块在用到的位置动态 import。
 */

import { defineTool } from "@deepseek-ai/dsh-tools";
import z from "@deepseek-ai/schemastery";

//#region 纯逻辑（校验、argv、端口解析、渲染）
// 本区段不依赖任何 import，test/pure.mjs 直接求值；下一个区段是配置与输出契约。

/** 启动模式；local 对应 argv 里的 `--local`。 */
const MODES = ["production", "local"];

/** 这些开关由参数生成，extraArgs 里再出现就是两个事实来源。 */
const OWNED_FLAGS = ["--local", "--config", "--port", "--root", "--dbPath"];

const DEFAULT_EXE_PATH = "E:\\build\\Ninja_debug\\bin\\doodle_kitsu_supplement.exe";
const DEFAULT_MODE = "production";
const DEFAULT_READY_PATH = "/";
/** 就绪等待上限：显式端口已定，剩下的只是等进程把监听建立起来。 */
const READY_TIMEOUT_MS = 100000;
const DEFAULT_STOP_TIMEOUT_MS = 5000;
const DEFAULT_MAX_OUTPUT_BYTES = 8 * 1024 * 1024;
const DEFAULT_MAX_TAIL_CHARS = 4000;
const DEFAULT_GUIDANCE_ORDER = 1396;
const MIN_OUTPUT_BYTES = 64 * 1024;
const MAX_OUTPUT_BYTES = 64 * 1024 * 1024;
const GUIDANCE_SECTION = "tool:kitsu-supplement";

/** 系统工具用绝对路径：提供方对裸名称按清理后的 PATH 解析，结果随宿主环境变化。 */
const NETSTAT_EXE = "C:\\Windows\\System32\\netstat.exe";
const TASKKILL_EXE = "C:\\Windows\\System32\\taskkill.exe";
/** 允许的监听端口闭区间；端口必须落在区间内，测试脚本才能引用固定地址。 */
const PORT_MIN = 50030;
const PORT_MAX = 50330;
/** 内存库标记：服务端没拿到 --dbPath 时落在 ":memory:"（kitsu_supplement.cpp:178）。 */
const MEMORY_DB = ":memory:";
/** sql 执行时等待写锁的上限，运行中的服务端可能正好在写。 */
const SQL_BUSY_TIMEOUT_MS = 5000;
const PORT_RANGE_TEXT = `${PORT_MIN}-${PORT_MAX}`;

/**
 * @param {unknown} value
 * @returns {boolean}
 */
function isPlainObject(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

/**
 * @param {unknown} value
 * @returns {string}
 */
function trimOrEmpty(value) {
  return typeof value === "string" ? value.trim() : "";
}

/**
 * 统一成反斜杠、去掉末尾分隔符（服务端两种斜杠都认，这里只为展示与去重一致）。
 *
 * @param {string} value
 * @returns {string}
 */
function normalizePath(value) {
  return value.replace(/\//g, "\\").replace(/\\+$/, "");
}

/**
 * 把 "50123" / 50123 解析成端口号；不合法返回 undefined。
 *
 * @param {unknown} value
 * @returns {number|undefined}
 */
function parsePort(value) {
  if (typeof value === "number" && Number.isInteger(value) && value >= 1 && value <= 65535) return value;
  if (typeof value === "string" && /^[0-9]+$/.test(value.trim())) {
    const parsed = Number.parseInt(value.trim(), 10);
    if (parsed >= 1 && parsed <= 65535) return parsed;
  }
  return undefined;
}

/**
 * 端口是否落在 [PORT_MIN, PORT_MAX] 闭区间内。
 *
 * @param {number|undefined} port
 * @returns {boolean}
 */
function portInRange(port) {
  return typeof port === "number" && port >= PORT_MIN && port <= PORT_MAX;
}

/**
 * 校验启动参数，返回全部问题（空数组表示通过）。
 *
 * @param {{mode:string, port:unknown, cwd?:string, root?:string, exePath?:string, readyPath?:string}} spec
 * @returns {string[]}
 */
function validateStartArgs({ mode, port, cwd, root, exePath, readyPath }) {
  /** @type {string[]} */
  const problems = [];
  if (!MODES.includes(mode)) {
    problems.push(`mode 只能是 ${MODES.join(" 或 ")}，收到 ${JSON.stringify(mode)}。`);
  }
  if (port === undefined || port === null || port === "") {
    problems.push(`必须显式指定 port，且必须落在 ${PORT_RANGE_TEXT} 之间；服务端不给 --port 时会监听随机端口，测试无法引用固定地址。`);
  } else if (parsePort(port) === undefined) {
    problems.push(`port 非法：必须是 1-65535 的整数，收到 ${JSON.stringify(port)}。`);
  } else if (!portInRange(parsePort(port))) {
    problems.push(`port 必须在 ${PORT_RANGE_TEXT} 之间，收到 ${JSON.stringify(port)}。`);
  }
  for (const [label, value] of [
    ["cwd", cwd],
    ["root", root],
    ["readyPath", readyPath],
  ]) {
    if (value !== undefined && (!(typeof value === "string") || value.trim() === "")) {
      problems.push(`${label} 必须是非空字符串。`);
    }
  }
  if (root !== undefined && !/^([A-Za-z]:[\\/]|\\\\[^\\/]+)/.test(root)) {
    problems.push(`root 必须是绝对路径，收到 ${JSON.stringify(root)}。`);
  }
  if (readyPath !== undefined && !readyPath.startsWith("/")) {
    problems.push(`readyPath 必须以 / 开头，收到 ${JSON.stringify(readyPath)}。`);
  }
  if (exePath !== undefined && !exePath.toLowerCase().endsWith(".exe")) {
    problems.push(`exePath 必须指向 .exe，收到 ${JSON.stringify(exePath)}。`);
  }
  return problems;
}

/**
 * 校验透传的 extraArgs：拒绝插件自己管理的开关。
 *
 * @param {unknown} extraArgs
 * @returns {string[]}
 */
function validateExtraArgs(extraArgs) {
  if (extraArgs === undefined) return [];
  if (!Array.isArray(extraArgs)) return ["extraArgs 必须是字符串数组。"];
  /** @type {string[]} */
  const problems = [];
  for (const item of extraArgs) {
    if (typeof item !== "string" || item.trim() === "") {
      problems.push(`extraArgs 含非法项 ${JSON.stringify(item)}（必须是非空字符串）。`);
      continue;
    }
    const flag = item.includes("=") ? item.slice(0, item.indexOf("=")) : item;
    if (OWNED_FLAGS.includes(flag.trim())) {
      problems.push(`extraArgs 不允许包含 ${flag.trim()}：它由参数决定（mode / port / root），不要在这里重复。`);
    }
  }
  return problems;
}

/**
 * 校验 extraConfig。
 *
 * @param {unknown} extraConfig
 * @returns {string[]}
 */
function validateExtraConfig(extraConfig) {
  if (extraConfig === undefined) return [];
  if (!isPlainObject(extraConfig)) return ["extraConfig 必须是 JSON 对象。"];
  const problems = [];
  if (Object.hasOwn(extraConfig, "port")) {
    problems.push("extraConfig 不允许包含 port：端口只由 port 参数决定（local 模式还会把它作为 --port 传给服务端）。");
  }
  return problems;
}

/**
 * 生成 config JSON：默认路径 -> extraConfig -> 参数覆盖；只保留有值的键。
 *
 * @param {{defaults:Record<string, unknown>, port:number, extraConfig?:Record<string, unknown>, overrides?:Record<string, unknown>}} spec
 * @returns {Record<string, unknown>}
 */
function buildConfig({ defaults, port, extraConfig, overrides }) {
  /** @type {Record<string, unknown>} */
  const merged = {};
  if (isPlainObject(defaults)) {
    for (const [key, value] of Object.entries(defaults)) {
      if (value !== undefined && value !== "") merged[key] = value;
    }
  }
  if (isPlainObject(extraConfig)) {
    for (const [key, value] of Object.entries(extraConfig)) {
      if (value !== undefined) merged[key] = value;
    }
  }
  if (isPlainObject(overrides)) {
    for (const [key, value] of Object.entries(overrides)) {
      if (value !== undefined && value !== "") merged[key] = value;
    }
  }
  merged.port = port;
  return merged;
}

/**
 * 按服务端的解析顺序拼 argv（可执行文件本身不在 argv 里）：
 * `--config=<json>` -> `--root=<path>` -> `--dbPath=<path>` -> `--local` -> `--port=<n>` -> extraArgs。
 *
 * 服务端用 argh 解析，`--key value` 里的 value 只会落进位置参数（argh.h:236-240），
 * 而 arg_({...}) 的 operator() 先查 params_ 再查 flags_（argh.h:345-364），
 * 所以带值开关必须写成 `--key=value`（argh.h:196-204 记进 params_）才拿得到值。
 *
 * @param {{mode:string, port:number, configPath?:string, root?:string, dbPath?:string, extraArgs?:string[]}} spec
 * @returns {string[]}
 */
function buildArgv({ mode, port, configPath, root, dbPath, extraArgs }) {
  /** @type {string[]} */
  const argv = [];
  if (configPath) argv.push(`--config=${configPath}`);
  if (root) argv.push(`--root=${root}`);
  if (dbPath) argv.push(`--dbPath=${dbPath}`);
  if (mode === "local") argv.push("--local");
  argv.push(`--port=${port}`);
  for (const item of extraArgs ?? []) argv.push(item);
  return argv;
}

/**
 * 把一段 sql 脚本切成单条语句。
 *
 * 轻量扫描而不是真解析：引号（' " ` 与 [ ] 标识符）、`--` 行注释、块注释里的分号都不算边界；
 * `CREATE TRIGGER ... BEGIN ... END;` 的内部语句靠 BEGIN/END 深度判断。
 * 分号之后只剩空白/注释的尾巴不算语句。
 *
 * @param {string} text
 * @returns {string[]}
 */
function splitSqlStatements(text) {
  /** @type {string[]} */
  const statements = [];
  const source = String(text ?? "");
  let current = "";
  let quote = "";
  let depth = 0;
  let isTrigger = false;
  let isWordStart = true;
  for (let i = 0; i < source.length; i += 1) {
    const ch = source[i];
    const next = source[i + 1];
    if (quote !== "") {
      current += ch;
      isWordStart = false;
      if (ch === quote) {
        if (next === quote) {
          current += next;
          i += 1;
        } else {
          quote = "";
        }
      }
      continue;
    }
    if (ch === "'" || ch === '"' || ch === "`") {
      quote = ch;
      current += ch;
      isWordStart = false;
      continue;
    }
    if (ch === "[") {
      quote = "]";
      current += ch;
      isWordStart = false;
      continue;
    }
    if (ch === "-" && next === "-") {
      const end = source.indexOf("\n", i);
      if (end === -1) break;
      current += " ";
      i = end;
      isWordStart = true;
      continue;
    }
    if (ch === "/" && next === "*") {
      const end = source.indexOf("*/", i + 2);
      if (end === -1) break;
      current += " ";
      i = end + 1;
      isWordStart = true;
      continue;
    }
    if (/[A-Za-z_]/.test(ch) && isWordStart) {
      let word = "";
      let j = i;
      while (j < source.length && /[A-Za-z0-9_]/.test(source[j])) {
        word += source[j];
        j += 1;
      }
      const upper = word.toUpperCase();
      if (upper === "CREATE" && current.trim() === "" && !isTrigger) isTrigger = true;
      else if (isTrigger && upper === "BEGIN") depth += 1;
      else if (isTrigger && upper === "END" && depth > 0) depth -= 1;
      current += word;
      i = j - 1;
      isWordStart = false;
      continue;
    }
    if (ch === ";" && depth === 0) {
      const trimmed = current.trim();
      if (trimmed !== "") statements.push(trimmed);
      current = "";
      isTrigger = false;
      isWordStart = true;
      continue;
    }
    current += ch;
    isWordStart = !/[A-Za-z0-9_$]/.test(ch);
  }
  const tail = current.trim();
  if (tail !== "") statements.push(tail);
  return statements;
}

/**
 * 语句首关键字，用于拒绝跨库操作。
 *
 * @param {string} statement
 * @returns {string}
 */
function sqlVerb(statement) {
  const match = /^[A-Za-z]+/.exec(statement.trim());
  return match === null ? "" : match[0].toUpperCase();
}

/**
 * 校验一条语句能否交给本工具执行。
 *
 * ATTACH/DETACH 会把写入引到另一个库文件，直接拒绝。
 *
 * @param {string} statement
 * @returns {string|undefined}
 */
function rejectSqlStatement(statement) {
  const verb = sqlVerb(statement);
  if (verb === "ATTACH" || verb === "DETACH") {
    return `不允许 ${verb}：本工具只写目标库文件，跨库操作请手工处理。`;
  }
  return undefined;
}

/**
 * `0.0.0.0:50025` / `[::]:50025` -> 50025。
 *
 * @param {string} local
 * @returns {number|undefined}
 */
function localPortOf(local) {
  const index = local.lastIndexOf(":");
  if (index < 0) return undefined;
  const text = local.slice(index + 1);
  if (!/^[0-9]+$/.test(text)) return undefined;
  return Number.parseInt(text, 10);
}

/**
 * 从 `netstat -ano` 输出里找出监听目标端口的 PID（去重升序）。
 *
 * @param {string} text
 * @param {number} port
 * @returns {number[]}
 */
function parseNetstatOwners(text, port) {
  /** @type {Set<number>} */
  const owners = new Set();
  for (const raw of String(text ?? "").split(/\r?\n/)) {
    const parts = raw.trim().split(/\s+/);
    if (parts.length < 5) continue;
    const [proto, local, , state, pid] = parts;
    if (proto.toUpperCase() !== "TCP") continue;
    if (state.toUpperCase() !== "LISTENING") continue;
    if (localPortOf(local) !== port) continue;
    if (!/^[0-9]+$/.test(pid)) continue;
    const value = Number.parseInt(pid, 10);
    if (value > 0) owners.add(value);
  }
  return [...owners].sort((a, b) => a - b);
}

/**
 * 命令行展示形式（仅用于 render / 日志，不参与执行）。
 *
 * @param {string[]} argv
 * @returns {string}
 */
function renderArgv(argv) {
  return argv.map((part) => (/[\s"]/.test(part) ? `"${part.replace(/"/g, '\\"')}"` : part)).join(" ");
}

/**
 * @param {number} port
 * @returns {string}
 */
function baseUrlOf(port) {
  return `http://127.0.0.1:${port}`;
}

/**
 * @param {number} port
 * @param {string} readyPath
 * @returns {string}
 */
function readyUrlOf(port, readyPath) {
  const path = String(readyPath ?? "/");
  return `${baseUrlOf(port)}${path.startsWith("/") ? path : `/${path}`}`;
}

/**
 * @param {unknown} error
 * @returns {string}
 */
function errorText(error) {
  if (error === null || error === undefined) return "unknown error";
  if (typeof error === "string") return error;
  if (typeof error.message === "string") return error.message;
  return String(error);
}

/**
 * @param {string} value
 * @returns {string}
 */
function stripLoneSurrogates(value) {
  return value.replace(/[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/g, "\uFFFD");
}

/**
 * 投影成「确定能无损序列化」的对象：丢掉 undefined 键，替换孤立代理项。
 * Host 对返回值做无损 JSON 校验，undefined 键或孤立代理项会让已经成功的调用整体失败。
 *
 * @param {any} value
 * @returns {any}
 */
function sanitizeForJson(value) {
  if (typeof value === "string") return stripLoneSurrogates(value);
  if (Array.isArray(value)) return value.map(sanitizeForJson);
  if (value && typeof value === "object") {
    /** @type {Record<string, any>} */
    const out = {};
    for (const [key, item] of Object.entries(value)) {
      if (item === undefined) continue;
      out[key] = sanitizeForJson(item);
    }
    return out;
  }
  return value;
}

/**
 * 从文本尾部截取 maxChars 个字符（行对齐）。
 *
 * @param {string} text
 * @param {number} maxChars
 * @returns {string}
 */
function tailText(text, maxChars) {
  const value = String(text ?? "");
  if (value.length <= maxChars) return value;
  const cut = value.slice(value.length - maxChars);
  const firstBreak = cut.indexOf("\n");
  return firstBreak >= 0 && firstBreak + 1 < cut.length ? cut.slice(firstBreak + 1) : cut;
}

/**
 * @param {unknown} value
 * @param {number} fallback
 * @returns {number}
 */
function positiveOrDefault(value, fallback) {
  return typeof value === "number" && Number.isFinite(value) && value > 0 ? value : fallback;
}

/**
 * @param {number} value
 * @param {number} min
 * @param {number} max
 * @returns {number}
 */
function clamp(value, min, max) {
  return Math.min(max, Math.max(min, Math.trunc(value)));
}

/**
 * 由配置与参数算出本次启动的全部事实。
 *
 * @param {object} deps
 * @param {any} deps.config
 * @param {any} deps.args
 * @param {(path:string) => Promise<boolean>} deps.exists
 * @returns {Promise<{problems:string[], spec:any}>}
 */
async function resolveSpec({ config, args, exists }) {
  const mode = trimOrEmpty(args.mode) || config.defaultMode || DEFAULT_MODE;
  const exePath = trimOrEmpty(args.exePath) || config.exePath || DEFAULT_EXE_PATH;
  const explicitCwd = trimOrEmpty(args.cwd) || trimOrEmpty(config.defaultCwd);
  const root = trimOrEmpty(args.root) || trimOrEmpty(config.defaultRoot);
  const readyPath = trimOrEmpty(args.readyPath) || config.readyPath || DEFAULT_READY_PATH;
  const port = parsePort(args.port);

  const problems = [
    ...validateStartArgs({
      mode,
      port: args.port,
      cwd: explicitCwd === "" ? undefined : explicitCwd,
      root: root === "" ? undefined : root,
      exePath,
      readyPath,
    }),
    ...validateExtraArgs(args.extraArgs),
    ...validateExtraConfig(args.extraConfig),
  ];

  let cwd = explicitCwd === "" ? "" : normalizePath(explicitCwd);
  if (cwd === "") {
    const index = exePath.lastIndexOf("\\");
    cwd = index > 0 ? exePath.slice(0, index) : "";
  }
  if (cwd === "") problems.push(`无法从 exePath 推断 cwd（${exePath}），请显式传 cwd。`);
  const normalizedRoot = root === "" ? undefined : normalizePath(root);

  /** @type {Record<string, unknown>} */
  const overrides = {};
  const dbPath = trimOrEmpty(args.dbPath);
  const frontEndPath = trimOrEmpty(args.frontEndPath);
  if (dbPath !== "" && dbPath !== MEMORY_DB) overrides.db_path = normalizePath(dbPath);
  if (frontEndPath !== "") overrides.kitsu_front_end_path = normalizePath(frontEndPath);
  if (args.publicUrlPort !== undefined && args.publicUrlPort !== "") overrides.public_url_port = args.publicUrlPort;

  const spec = {
    mode,
    port,
    exePath,
    cwd,
    root: normalizedRoot,
    dbPath: dbPath === "" ? undefined : dbPath === MEMORY_DB ? MEMORY_DB : normalizePath(dbPath),
    readyPath,
    extraArgs: Array.isArray(args.extraArgs) ? args.extraArgs : undefined,
    configValues: buildConfig({ defaults: config.defaultConfigPaths ?? {}, port, extraConfig: args.extraConfig, overrides }),
  };

  if (problems.length > 0 || port === undefined || cwd === "") return { problems, spec };

  if (!(await exists(exePath))) problems.push(`exe 不存在：${exePath}`);
  if (!(await exists(cwd))) problems.push(`cwd 不存在：${cwd}`);

  return { problems, spec };
}

/**
 * 启动成功/失败都要带上的提示。local 模式没给 dbPath 时会落进 ":memory:"，
 * 那个库只活在本进程里，kitsu_server_sql 写不进去。
 *
 * @param {{mode:string, dbPath?:string}} spec
 * @returns {string[]}
 */
function startWarnings(spec) {
  /** @type {string[]} */
  const warnings = [];
  if (spec.dbPath === undefined) {
    warnings.push(
      spec.mode === "local"
        ? "local 模式没有传 dbPath：服务端使用内存数据库（:memory:），进程退出即丢，kitsu_server_sql 无法写入；需要可写库请传 dbPath。"
        : "没有传 dbPath：服务端使用其内置默认库路径（C:/kitsu_new.database），kitsu_server_sql 会写入该文件。",
    );
  }
  return warnings;
}

/**
 * 拼一个「启动失败」的结果对象（键集合与启动成功时一致）。
 *
 * @param {any} spec
 * @param {string} code
 * @param {string} reason
 * @param {Record<string, any>} [extra]
 * @returns {any}
 */
function startFailure(spec, code, reason, extra = {}) {
  return sanitizeForJson({
    ok: false,
    code,
    reason,
    mode: spec.mode,
    port: spec.port,
    baseUrl: spec.port === undefined ? undefined : baseUrlOf(spec.port),
    cwd: spec.cwd,
    exePath: spec.exePath,
    configPath: spec.configPath,
    command: spec.command,
    killExisting: spec.killExisting === true ? true : undefined,
    readyMs: 0,
    readyPath: spec.readyPath,
    priorStopped: [],
    diagnostics: [],
    warnings: [],
    ...extra,
  });
}

/**
 * 校验 kitsu_server_sql 的参数。
 *
 * @param {any} args
 * @returns {string[]}
 */
function validateSqlArgs(args) {
  /** @type {string[]} */
  const problems = [];
  const hasSql = trimOrEmpty(args.sql) !== "";
  const files = Array.isArray(args.files) ? args.files : [];
  if (!hasSql && files.length === 0) problems.push("必须给 sql 片段或至少一个 files 路径。");
  if (args.files !== undefined && !Array.isArray(args.files)) problems.push("files 必须是字符串数组。");
  for (const file of files) {
    if (typeof file !== "string" || file.trim() === "") {
      problems.push(`files 含非法项 ${JSON.stringify(file)}（必须是非空绝对路径）。`);
    } else if (!/^([A-Za-z]:[\\/]|\\\\[^\\/]+)/.test(file)) {
      problems.push(`files 项必须是绝对路径，收到 ${JSON.stringify(file)}。`);
    } else if (!file.toLowerCase().endsWith(".sql")) {
      problems.push(`files 项必须以 .sql 结尾，收到 ${JSON.stringify(file)}。`);
    }
  }
  if (args.dbPath !== undefined) {
    const dbPath = trimOrEmpty(args.dbPath);
    if (dbPath === "") problems.push("dbPath 不能是空串；不给就沿用实例或插件默认值。");
    else if (dbPath === MEMORY_DB) problems.push(`${MEMORY_DB} 是进程内存库，外部写不进去；请给服务端传一个磁盘 dbPath。`);
    else if (!/^([A-Za-z]:[\\/]|\\\\[^\\/]+)/.test(dbPath)) problems.push(`dbPath 必须是绝对路径，收到 ${JSON.stringify(dbPath)}。`);
  }
  if (args.serverId !== undefined && trimOrEmpty(args.serverId) === "") problems.push("serverId 不能是空串。");
  return problems;
}

/**
 * 决定这次写入哪个库：参数 > 实例记录的 dbPath > 插件默认 db_path。
 *
 * @param {object} deps
 * @param {any} deps.args
 * @param {any} deps.config
 * @param {Map<string, any>} deps.registry
 * @returns {{dbPath:string|undefined, serverId:string|undefined, problem?:string}}
 */
function resolveSqlTarget({ args, config, registry }) {
  const wantedId = trimOrEmpty(args.serverId);
  /** @type {any} */
  let record;
  if (wantedId !== "") {
    record = registry.get(wantedId);
    if (record === undefined) return { dbPath: undefined, serverId: wantedId, problem: `找不到实例 ${wantedId}（先用 kitsu_server_status 查看）。` };
  }
  const explicit = trimOrEmpty(args.dbPath);
  const fromRecord = typeof record?.dbPath === "string" ? record.dbPath : "";
  const fallback = trimOrEmpty(config?.defaultConfigPaths?.db_path);
  const dbPath = explicit !== "" ? normalizePath(explicit) : fromRecord !== "" ? fromRecord : fallback;
  if (dbPath === "") {
    return {
      dbPath: undefined,
      serverId: wantedId === "" ? undefined : wantedId,
      problem: "无法确定数据库路径：请传 dbPath，或先起一个带 dbPath 的实例，或配置 defaultConfigPaths.db_path。",
    };
  }
  if (dbPath === MEMORY_DB) {
    return {
      dbPath,
      serverId: wantedId === "" ? undefined : wantedId,
      problem:
        `目标是内存库 ${MEMORY_DB}（服务端启动时没拿到 --dbPath，见 kitsu_supplement.cpp:175-178）：外部进程写不进去。` +
        "请用 dbPath 参数重启服务端后再填充数据。",
    };
  }
  return { dbPath, serverId: wantedId === "" ? undefined : wantedId };
}

/**
 * 在磁盘库上执行一批 sql。服务端对磁盘库调 journal_mode(wal)（sqlite_database.cpp:1335），
 * 但实测新库仍可能是 rollback journal；两种模式下外部提交的写入都不需要停服或重启。
 *
 * 连接级 PRAGMA 与服务端保持一致（sqlite_database.cpp:1310-1312），
 * 否则可能写出服务端读不了的库（尤其外键）。
 *
 * @param {{dbPath:string, batches:{label:string, sql:string}[], dryRun:boolean}} spec
 * @returns {Promise<{ok:boolean, statements:number, changes:number, results:any[], error?:string}>}
 */
async function runSql({ dbPath, batches, dryRun }) {
  const { DatabaseSync } = await import("node:sqlite");
  /** @type {any} */
  let db;
  try {
    db = new DatabaseSync(dbPath, { timeout: SQL_BUSY_TIMEOUT_MS });
  } catch (error) {
    return { ok: false, statements: 0, changes: 0, results: [], error: `打不开数据库 ${dbPath}：${errorText(error)}` };
  }
  /** @type {any[]} */
  const results = [];
  let statements = 0;
  let changes = 0;
  let inTransaction = false;
  try {
    db.exec("PRAGMA foreign_keys = ON;");
    db.exec("PRAGMA synchronous = NORMAL;");
    db.exec("PRAGMA recursive_triggers = ON;");
    db.exec("BEGIN IMMEDIATE;");
    inTransaction = true;
    for (const batch of batches) {
      const list = splitSqlStatements(batch.sql);
      for (let index = 0; index < list.length; index += 1) {
        const statement = list[index];
        const rejection = rejectSqlStatement(statement);
        if (rejection !== undefined) throw new Error(`${batch.label} 第 ${index + 1} 条：${rejection}`);
        statements += 1;
        try {
          const stmt = db.prepare(statement);
          const rowChanges = dryRun ? 0 : Number(stmt.run()?.changes ?? 0);
          changes += rowChanges;
          results.push({ source: batch.label, index: index + 1, verb: sqlVerb(statement), changes: rowChanges });
        } catch (error) {
          return {
            ok: false,
            statements,
            changes: 0,
            results,
            error: `${batch.label} 第 ${index + 1} 条失败：${errorText(error)}\nSQL: ${tailText(statement, 400)}`,
          };
        }
      }
    }
    db.exec(dryRun ? "ROLLBACK;" : "COMMIT;");
    inTransaction = false;
    return { ok: true, statements, changes, results };
  } catch (error) {
    return { ok: false, statements, changes: 0, results, error: errorText(error) };
  } finally {
    if (inTransaction) {
      try {
        db.exec("ROLLBACK;");
      } catch {
        /* 事务可能已经因错误被 SQLite 回滚 */
      }
    }
    try {
      db.close();
    } catch {
      /* 连接已经不可用，无需再抛 */
    }
  }
}

/**
 * 从 stdout 里找监听协程打印的端口行：第一行纯数字就是端口。
 *
 * 端口以入参为准（http_listener.cpp:35 仍会把真实端口打到 stdout），
 * 这里只用于失败诊断时把 stdout 里报告过的端口一并报给调用方。
 *
 * @param {string} text
 * @returns {number|undefined}
 */
function findPortLine(text) {
  for (const raw of String(text ?? "").split(/\r?\n/)) {
    const line = raw.trim();
    if (line === "") continue;
    if (!/^[0-9]{1,5}$/.test(line)) return undefined;
    const port = Number.parseInt(line, 10);
    return port >= 1 && port <= 65535 ? port : undefined;
  }
  return undefined;
}

//#endregion

//#region 配置与输出契约

export const name = "kitsu-supplement";

export const inject = ["tools", "subprocess", "systemPrompt"];

export const Config = z.object({
  exePath: z.string().default(DEFAULT_EXE_PATH),
  defaultCwd: z.string().default(""),
  defaultMode: z.string().default(DEFAULT_MODE),
  defaultRoot: z.string().default(""),
  defaultConfigPaths: z
    .object({
      db_path: z.string().default(""),
      kitsu_front_end_path: z.string().default(""),
      root: z.string().default(""),
      public_url_port: z.union([z.number(), z.string()]).default(""),
    })
    .default({}),
  readyPath: z.string().default(DEFAULT_READY_PATH),
  readyTimeoutMs: z.number().default(READY_TIMEOUT_MS),
  stopTimeoutMs: z.number().default(DEFAULT_STOP_TIMEOUT_MS),
  maxOutputBytes: z.number().default(DEFAULT_MAX_OUTPUT_BYTES),
  guidanceOrder: z.number().default(DEFAULT_GUIDANCE_ORDER),
});

const SERVER_PROPERTIES = {
  mode: { type: "string", enum: MODES, description: "启动模式" },
  port: { type: "number", description: "监听端口（就是启动时传入的 --port）" },
  pid: { type: "number", description: "服务进程 PID" },
  baseUrl: { type: "string", description: "http://127.0.0.1:<port>" },
  cwd: { type: "string", description: "服务进程工作目录" },
  exePath: { type: "string", description: "可执行文件" },
  configPath: { type: "string", description: "为本次启动生成的 config JSON 路径" },
  command: { type: "string", description: "等价命令行" },
};

const DIAGNOSTIC_ITEM = {
  type: "object",
  additionalProperties: false,
  properties: {
    severity: { type: "string", enum: ["info", "warning", "error"], required: true },
    code: { type: "string", required: true },
    message: { type: "string", required: true },
  },
};

const START_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    ok: { type: "boolean", required: true },
    code: {
      type: "string",
      required: true,
      enum: [
        "ok",
        "invalid-args",
        "port-in-use",
        "kill-failed",
        "spawn-failed",
        "exited-before-ready",
        "ready-timeout",
        "cancelled",
      ],
    },
    reason: { type: "string", required: true },
    serverId: { type: "string", description: "实例 id（kitsu-N），后续 status / stop 用" },
    ...SERVER_PROPERTIES,
    killExisting: { type: "boolean", description: "本次是否请求强杀端口上的旧进程（仅启动失败时回显）" },
    readyMs: { type: "number", required: true },
    readyPath: { type: "string", required: true },
    readyStatus: { type: "number" },
    priorStopped: { type: "array", required: true, items: { type: "number" } },
    diagnostics: { type: "array", required: true, items: DIAGNOSTIC_ITEM },
    warnings: { type: "array", required: true, items: { type: "string" } },
    outputTail: { type: "string", description: "失败时的 stdout+stderr 尾巴" },
    spillPath: { type: "string", description: "输出溢出时的完整日志文件" },
  },
};

const SERVER_STATUS_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    serverId: { type: "string", required: true },
    running: { type: "boolean", required: true },
    ...SERVER_PROPERTIES,
    uptimeMs: { type: "number", required: true },
    exitCode: { type: "number" },
    exitSignal: { type: "string" },
  },
};

const STATUS_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    ok: { type: "boolean", required: true },
    reason: { type: "string", required: true },
    servers: { type: "array", required: true, items: SERVER_STATUS_SCHEMA },
    portOwners: {
      type: "array",
      required: true,
      items: {
        type: "object",
        additionalProperties: false,
        properties: {
          port: { type: "number", required: true },
          pids: { type: "array", required: true, items: { type: "number" } },
          ours: { type: "array", required: true, items: { type: "string" } },
          error: { type: "string" },
        },
      },
    },
    diagnostics: { type: "array", required: true, items: DIAGNOSTIC_ITEM },
  },
};

const STOP_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    ok: { type: "boolean", required: true },
    reason: { type: "string", required: true },
    stopped: {
      type: "array",
      required: true,
      items: {
        type: "object",
        additionalProperties: false,
        properties: {
          serverId: { type: "string", required: true },
          pid: { type: "number" },
          port: { type: "number" },
          ok: { type: "boolean", required: true },
          reason: { type: "string", required: true },
          exitCode: { type: "number" },
          waitedMs: { type: "number" },
        },
      },
    },
    remaining: { type: "array", required: true, items: { type: "string" } },
    diagnostics: { type: "array", required: true, items: DIAGNOSTIC_ITEM },
  },
};

const SQL_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    ok: { type: "boolean", required: true },
    code: { type: "string", required: true },
    reason: { type: "string", required: true },
    dbPath: { type: "string" },
    serverId: { type: "string" },
    dryRun: { type: "boolean" },
    statements: { type: "number", required: true },
    changes: { type: "number", required: true },
    results: {
      type: "array",
      required: true,
      items: {
        type: "object",
        additionalProperties: false,
        properties: {
          source: { type: "string", required: true },
          index: { type: "number", required: true },
          verb: { type: "string", required: true },
          changes: { type: "number", required: true },
        },
      },
    },
    diagnostics: { type: "array", required: true, items: DIAGNOSTIC_ITEM },
  },
};

/**
 * @param {any} value
 * @returns {any[]}
 */
function renderStart(args, value) {
  /** @type {string[]} */
  const lines = [];
  if (value.ok === true) {
    lines.push(`已启动 ${value.serverId} (${value.mode})  ${value.baseUrl}`);
    if (value.readyMs !== undefined) {
      lines.push(`就绪等待 ${value.readyMs} ms（GET ${value.readyPath} -> ${value.readyStatus ?? "?"}）`);
    }
    if (value.configPath) lines.push(`config: ${value.configPath}`);
    if (value.cwd) lines.push(`cwd: ${value.cwd}`);
    if (value.command) lines.push(`command: ${value.command}`);
    if ((value.priorStopped ?? []).length > 0) lines.push(`已强杀旧进程 PID: ${value.priorStopped.join(", ")}`);
  } else {
    lines.push(`启动失败 [${value.code}]: ${value.reason}`);
    if (value.baseUrl) lines.push(`目标地址: ${value.baseUrl}`);
    if (value.command) lines.push(`command: ${value.command}`);
  }
  for (const item of value.diagnostics ?? []) lines.push(`${item.severity.toUpperCase()} ${item.code}: ${item.message}`);
  for (const warning of value.warnings ?? []) lines.push(`WARNING: ${warning}`);
  if (value.outputTail) lines.push("", "--- process output tail ---", value.outputTail);
  return [{ type: "text", text: lines.join("\n") }];
}

/**
 * @param {any} args
 * @param {any} value
 * @returns {any[]}
 */
function renderStatus(args, value) {
  /** @type {string[]} */
  const lines = [value.reason];
  for (const server of value.servers ?? []) {
    const state = server.running === true ? "运行中" : `已退出(exit ${server.exitCode ?? "n/a"})`;
    lines.push("", `${server.serverId}  ${server.mode}  :${server.port}  ${state}`);
    if (server.baseUrl) lines.push(`  baseUrl: ${server.baseUrl}`);
    if (server.configPath) lines.push(`  config: ${server.configPath}`);
    if (server.cwd) lines.push(`  cwd: ${server.cwd}`);
  }
  for (const owner of value.portOwners ?? []) {
    const ours = owner.ours.length > 0 ? `  本插件实例: ${owner.ours.join(",")}` : "";
    if (owner.error !== undefined) {
      lines.push("", `端口 ${owner.port}: 占用检查不可用（${owner.error}）`);
      continue;
    }
    const state = owner.pids.length > 0 ? `LISTENING pid=${owner.pids.join(",")}` : "空闲";
    lines.push("", `端口 ${owner.port}: ${state}${ours}`);
  }
  for (const item of value.diagnostics ?? []) lines.push(`${item.severity.toUpperCase()} ${item.code}: ${item.message}`);
  return [{ type: "text", text: lines.join("\n") }];
}

/**
 * @param {any} args
 * @param {any} value
 * @returns {any[]}
 */
function renderStop(args, value) {
  /** @type {string[]} */
  const lines = [value.reason];
  for (const item of value.stopped ?? []) {
    const exit = item.exitCode === undefined || item.exitCode === null ? "" : `  exit=${item.exitCode}`;
    lines.push(`${item.serverId}  pid=${item.pid}  :${item.port}  ${item.ok ? "已停止" : `停止失败: ${item.reason}`}${exit}`);
  }
  for (const item of value.diagnostics ?? []) lines.push(`${item.severity.toUpperCase()} ${item.code}: ${item.message}`);
  return [{ type: "text", text: lines.join("\n") }];
}

/**
 * @param {any} args
 * @param {any} value
 * @returns {any[]}
 */
function renderSql(args, value) {
  /** @type {string[]} */
  const lines = [];
  lines.push(`${value.ok ? "OK" : "FAILED"} [${value.code}] ${value.reason}`);
  if (value.dbPath) lines.push(`db: ${value.dbPath}`);
  if (value.serverId) lines.push(`server: ${value.serverId}（未停服、未重启，写完即可查）`);
  if (value.ok === true) {
    lines.push(`statements: ${value.statements}  changes: ${value.changes}${value.dryRun ? "  (dryRun: 已回滚)" : ""}`);
    const byVerb = new Map();
    for (const item of value.results ?? []) {
      byVerb.set(item.verb, (byVerb.get(item.verb) ?? 0) + 1);
    }
    if (byVerb.size > 0) {
      lines.push(`by verb: ${[...byVerb].map(([verb, count]) => `${verb}=${count}`).join(", ")}`);
    }
  } else {
    for (const item of value.diagnostics ?? []) lines.push(`${item.severity.toUpperCase()} ${item.code}: ${item.message}`);
  }
  return [{ type: "text", text: lines.join("\n") }];
}

//#endregion

//#region 插件入口

/** 本插件的实例表：serverId -> 实例记录。 */
const registry = new Map();

/** 实例 id 自增种子。 */
let nextServerId = 1;

/**
 * 分配实例 id；只增不复用，保证 stop 后同一 id 不会指向新进程。
 *
 * @returns {string}
 */
function allocateServerId() {
  const id = `kitsu-${nextServerId}`;
  nextServerId += 1;
  return id;
}

/**
 * 清空实例表（测试用）。
 */
export function resetRegistry() {
  registry.clear();
  nextServerId = 1;
}

/**
 * @param {string} serverId
 * @returns {any|undefined}
 */
export function getServer(serverId) {
  return registry.get(serverId);
}

/**
 * 按端口找仍存活的实例。
 *
 * @param {number} port
 * @returns {string[]}
 */
function serversOnPort(port) {
  /** @type {string[]} */
  const ids = [];
  for (const [id, server] of registry) {
    if (server.exited === true || server.port !== port) continue;
    ids.push(id);
  }
  return ids;
}

/**
 * 读取某条流的当前可见文本。
 *
 * @param {any} reader
 * @returns {{text:string, spillPath?:string}}
 */
function readAll(reader) {
  if (!reader) return { text: "" };
  let text = "";
  let offset = 0;
  /** @type {string|undefined} */
  let spillPath;
  for (let guard = 0; guard < 10000; guard += 1) {
    const chunk = reader.readFrom(offset);
    if (!chunk) break;
    text += typeof chunk.text === "string" ? chunk.text : "";
    if (typeof chunk.spillPath === "string") spillPath = chunk.spillPath;
    if (chunk.nextOffset === offset) break;
    offset = chunk.nextOffset;
  }
  return spillPath === undefined ? { text } : { text, spillPath };
}

/**
 * 等一下（Windows 上 setTimeout 只有毫秒精度）。
 *
 * @param {number} ms
 * @returns {Promise<void>}
 */
function sleep(ms) {
  return new Promise((resolve) => {
    setTimeout(resolve, ms);
  });
}

/**
 * 跑一次 `netstat.exe -ano`。
 *
 * 刻意用绝对路径而不是 PATH 上的 `netstat`：提供方对裸名称按清理后的 PATH 解析，
 * 结果随宿主进程环境影响。
 *
 * @param {any} ctx
 * @param {AbortSignal} [signal]
 * @returns {Promise<{text:string, error?:string}>}
 */
async function netstatText(ctx, signal) {
  /** @type {any} */
  let handle;
  try {
    handle = ctx.subprocess.spawn({
      argv: [NETSTAT_EXE, "-ano"],
      cwd: "C:\\Windows\\System32",
      stdio: {
        stdin: "ignore",
        stdout: { maxBytes: 8 * 1024 * 1024 },
        stderr: { maxBytes: 64 * 1024 },
      },
      graceMs: 2000,
      signal,
    });
  } catch (error) {
    return { text: "", error: errorText(error) };
  }
  try {
    const outcome = await handle.done;
    const exitCode = typeof outcome?.exitCode === "number" ? outcome.exitCode : undefined;
    if (exitCode !== undefined && exitCode !== 0) {
      return { text: "", error: `netstat 退出码 ${exitCode}` };
    }
    return { text: readAll(handle.collected?.stdout).text };
  } catch (error) {
    return { text: "", error: errorText(error) };
  }
}

/**
 * 探测端口占用：先看本插件实例表，再问 netstat。
 *
 * @param {any} ctx
 * @param {number} port
 * @param {AbortSignal} [signal]
 * @returns {Promise<{owners:number[], ours:string[], error?:string}>}
 */
async function probePort(ctx, port, signal) {
  const ours = serversOnPort(port);
  const netstat = await netstatText(ctx, signal);
  if (netstat.error !== undefined) return { owners: [], ours, error: netstat.error };
  return { owners: parseNetstatOwners(netstat.text, port), ours };
}

/**
 * 强杀 PID 树。只在显式 killExisting 时使用。
 *
 * 走 `ctx.subprocess` 而不是 node:child_process：沙箱下宿主进程起原生子进程会被拒，
 * 而受管的子进程通道始终可用。
 *
 * @param {any} ctx
 * @param {number[]} pids
 * @returns {Promise<{killed:number[], failed:string[]}>}
 */
async function killPids(ctx, pids) {
  /** @type {number[]} */
  const killed = [];
  /** @type {string[]} */
  const failed = [];
  for (const pid of pids) {
    /** @type {any} */
    let handle;
    try {
      handle = ctx.subprocess.spawn({
        argv: [TASKKILL_EXE, "/F", "/T", "/PID", String(pid)],
        cwd: "C:\\Windows\\System32",
        stdio: { stdin: "ignore", stdout: { maxBytes: 16384 }, stderr: { maxBytes: 16384 } },
        graceMs: 2000,
      });
      await handle.done;
    } catch (error) {
      failed.push(`taskkill /F /T /PID ${pid} 失败：${errorText(error)}`);
      continue;
    }
    const errorTextValue = readAll(handle.collected?.stderr).text.trim();
    if (errorTextValue !== "") {
      failed.push(`taskkill /F /T /PID ${pid} 报告：${tailText(errorTextValue, 500)}`);
      continue;
    }
    killed.push(pid);
  }
  return { killed, failed };
}

/**
 * 轮询直到服务端能应答 HTTP、进程退出或超时。
 *
 * 任何 HTTP 状态码都算就绪：服务端对未注册路由也会回状态码，不必预知具体路径。
 *
 * @param {object} deps
 * @param {number} deps.port
 * @param {string} deps.readyPath
 * @param {() => boolean} deps.isDead
 * @param {number} deps.timeoutMs
 * @param {AbortSignal} [deps.signal]
 * @returns {Promise<{ready:boolean, status?:number, error?:string, waitedMs:number, dead:boolean, aborted:boolean, answered:boolean}>}
 */
async function waitForReady({ port, readyPath, isDead, timeoutMs, signal }) {
  const url = readyUrlOf(port, readyPath);
  const started = Date.now();
  /** @type {string|undefined} */
  let lastError;
  /** @type {number|undefined} */
  let lastStatus;
  let answered = false;
  for (;;) {
    if (signal?.aborted === true) {
      return { ready: answered, status: lastStatus, error: lastError, waitedMs: Date.now() - started, dead: isDead(), aborted: true, answered };
    }
    if (isDead()) {
      return { ready: false, status: lastStatus, error: lastError, waitedMs: Date.now() - started, dead: true, aborted: false, answered };
    }
    try {
      const response = await fetch(url, { method: "GET", signal: AbortSignal.timeout(2000) });
      answered = true;
      lastStatus = response.status;
      if (response.body) {
        await response.body.cancel().catch(() => {});
      }
      return { ready: true, status: response.status, error: undefined, waitedMs: Date.now() - started, dead: false, aborted: false, answered: true };
    } catch (error) {
      lastError = errorText(error);
    }
    if (Date.now() - started >= timeoutMs) {
      return { ready: answered, status: lastStatus, error: lastError, waitedMs: Date.now() - started, dead: isDead(), aborted: false, answered };
    }
    await sleep(Math.min(250, Math.max(1, timeoutMs - (Date.now() - started))));
  }
}

/**
 * 等进程退出（不抛）。
 *
 * @param {any} handle
 * @param {number} timeoutMs
 * @returns {Promise<{ok:boolean, reason:string}>}
 */
async function waitForExit(handle, timeoutMs) {
  if (typeof handle.waitForExit !== "function") {
    await handle.done.catch(() => {});
    return { ok: true, reason: "done" };
  }
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), Math.max(1, timeoutMs));
  try {
    const settled = await handle.waitForExit(controller.signal);
    return settled === false ? { ok: false, reason: `${timeoutMs} ms 内进程未退出` } : { ok: true, reason: "exited" };
  } catch (error) {
    return { ok: false, reason: `等待退出失败：${errorText(error)}` };
  } finally {
    clearTimeout(timer);
  }
}

/**
 * 终止实例并等停稳。
 *
 * @param {any} record
 * @param {number} timeoutMs
 * @returns {Promise<{ok:boolean, reason:string, exitCode?:number|null, waitedMs:number}>}
 */
async function stopRecord(record, timeoutMs) {
  const started = Date.now();
  if (record.exited === true) {
    return { ok: true, reason: "进程已退出", exitCode: record.exitCode ?? null, waitedMs: 0 };
  }
  try {
    record.handle.terminate();
  } catch (error) {
    return { ok: false, reason: `terminate 抛错：${errorText(error)}`, waitedMs: Date.now() - started };
  }
  const exited = await waitForExit(record.handle, timeoutMs);
  if (!exited.ok) return { ok: false, reason: exited.reason, waitedMs: Date.now() - started };
  return { ok: true, reason: "已终止", exitCode: record.exitCode ?? null, waitedMs: Date.now() - started };
}

/**
 * 启动一个实例并等它就绪。
 *
 * @param {any} ctx
 * @param {object} spec
 * @param {string} spec.exePath
 * @param {string[]} spec.argv
 * @param {string} spec.cwd
 * @param {string} spec.command
 * @param {string} spec.mode
 * @param {number} spec.port
 * @param {string} spec.configPath
 * @param {string} spec.readyPath
 * @param {boolean} spec.killExisting
 * @param {number} spec.readyTimeoutMs
 * @param {number} spec.stopTimeoutMs
 * @param {number} spec.maxOutputBytes
 * @param {string[]} spec.warnings
 * @param {AbortSignal} [spec.signal]
 * @returns {Promise<any>}
 */
async function startServer(ctx, spec) {
  /** @type {string[]} */
  const warnings = [...spec.warnings];
  /** @type {any[]} */
  const diagnostics = [];
  /** @type {number[]} */
  const priorStopped = [];

  const busy = await probePort(ctx, spec.port, spec.signal);
  if (busy.error !== undefined) {
    warnings.push(`端口占用检查不可用（${busy.error}）：继续启动，端口冲突会以进程退出码的形式暴露。`);
  }
  if (busy.owners.length > 0 || busy.ours.length > 0) {
    if (spec.killExisting !== true) {
      const parts = [];
      if (busy.ours.length > 0) parts.push(`本插件实例 ${busy.ours.join(", ")} 正在监听`);
      if (busy.owners.length > 0) parts.push(`PID ${busy.owners.join(", ")} 正在监听`);
      const detail = `端口 ${spec.port} 已被占用：${parts.join("；")}。先 kitsu_server_stop 停掉，或传 killExisting: true 强杀后重启。`;
      return startFailure(spec, "port-in-use", detail, {
        diagnostics: [{ severity: "error", code: "port/in-use", message: detail }],
      });
    }
    for (const id of busy.ours) {
      const record = registry.get(id);
      if (record === undefined) continue;
      const result = await stopRecord(record, spec.stopTimeoutMs);
      if (result.ok) {
        registry.delete(id);
      } else {
        diagnostics.push({ severity: "error", code: "kill/failed", message: `停止本插件实例 ${id} 失败：${result.reason}` });
      }
    }
    const ownPids = new Set([...registry.values()].map((record) => record.pid));
    const foreign = busy.owners.filter((pid) => !ownPids.has(pid));
    if (foreign.length > 0) {
      const killed = await killPids(ctx, foreign);
      priorStopped.push(...killed.killed);
      for (const failure of killed.failed) {
        diagnostics.push({ severity: "error", code: "kill/failed", message: failure });
      }
    }
    if (diagnostics.some((item) => item.severity === "error")) {
      return startFailure(spec, "kill-failed", "端口仍被占用：强杀旧进程失败。", { diagnostics, priorStopped });
    }
    warnings.push(`killExisting: 端口 ${spec.port} 上的旧进程已被强杀（PID ${priorStopped.join(", ") || "无"}）。`);
  }

  /** @type {any} */
  let handle;
  try {
    handle = ctx.subprocess.spawn({
      argv: [spec.exePath, ...spec.argv],
      cwd: spec.cwd,
      stdio: {
        stdin: "ignore",
        stdout: { maxBytes: spec.maxOutputBytes },
        stderr: { maxBytes: spec.maxOutputBytes },
      },
      graceMs: 5000,
      signal: spec.signal,
    });
  } catch (error) {
    return startFailure(spec, "spawn-failed", `无法启动 ${spec.exePath}：${errorText(error)}`, {
      diagnostics: [{ severity: "error", code: "spawn/failed", message: errorText(error) }],
      priorStopped,
    });
  }

  const serverId = spec.serverId;
  /** @type {any} */
  const record = {
    serverId,
    handle,
    mode: spec.mode,
    port: spec.port,
    readyPath: spec.readyPath,
    startedAt: Date.now(),
    exited: false,
    exitCode: null,
    exitSignal: null,
    outputOffset: 0,
    exePath: spec.exePath,
    cwd: spec.cwd,
    configPath: spec.configPath,
    dbPath: spec.dbPath,
    command: spec.command,
  };
  handle.done.then(
    (outcome) => {
      record.exited = true;
      record.exitCode = typeof outcome?.exitCode === "number" ? outcome.exitCode : null;
      record.exitSignal = typeof outcome?.signal === "string" ? outcome.signal : null;
    },
    () => {
      record.exited = true;
      record.exitCode = null;
      record.exitSignal = null;
      record.providerFailed = true;
    },
  );

  const port = spec.port;
  const stdoutText = () => readAll(handle.collected?.stdout);
  const stderrText = () => readAll(handle.collected?.stderr);
  const outputTail = () =>
    tailText([stdoutText().text, stderrText().text].filter((text) => text.length > 0).join("\n"), DEFAULT_MAX_TAIL_CHARS) || undefined;

  registry.set(serverId, record);

  const ready = await waitForReady({
    port,
    readyPath: spec.readyPath,
    isDead: () => record.exited === true,
    timeoutMs: spec.readyTimeoutMs,
    signal: spec.signal,
  });

  if (!ready.ready) {
    await stopRecord(record, spec.stopTimeoutMs);
    registry.delete(serverId);
    diagnostics.push({
      severity: "error",
      code: "ready/failed",
      message: ready.error ?? "没有取得 HTTP 响应",
    });
    if (spec.configPath) {
      diagnostics.push({
        severity: "info",
        code: "config/hint",
        message: `服务端 --config 解析失败时会直接退出且不启动 HTTP（kitsu_supplement.cpp:158-163），请检查 ${spec.configPath}。`,
      });
    }
    const stdoutPort = findPortLine(stdoutText().text);
    if (stdoutPort !== undefined && stdoutPort !== port) {
      diagnostics.push({
        severity: "warning",
        code: "listen/stdout-mismatch",
        message: `stdout 里报告监听端口 ${stdoutPort}，与入参 port ${port} 不一致——说明服务端没有按 --port 监听（经包装器启动？参数被改写？）。`,
      });
    }
    return startFailure(
      spec,
      ready.dead ? "exited-before-ready" : ready.aborted ? "cancelled" : "ready-timeout",
      ready.dead
        ? `进程在就绪前退出（exit ${record.exitCode ?? "n/a"}）。`
        : ready.aborted
          ? `已取消：就绪等待被中止，进程已终止。`
          : `${spec.readyTimeoutMs} ms 内 GET ${readyUrlOf(port, spec.readyPath)} 没有响应，已终止。`,
      {
        serverId,
        port,
        baseUrl: baseUrlOf(port),
        readyMs: ready.waitedMs,
        diagnostics,
        outputTail: outputTail(),
        priorStopped,
      },
    );
  }

  return sanitizeForJson({
    ok: true,
    code: "ok",
    reason: `已启动并监听端口 ${port}（${spec.mode} 模式）。`,
    serverId,
    mode: spec.mode,
    port,
    baseUrl: baseUrlOf(port),
    cwd: spec.cwd,
    exePath: spec.exePath,
    configPath: spec.configPath,
    command: spec.command,
    readyMs: ready.waitedMs,
    readyPath: spec.readyPath,
    readyStatus: ready.status,
    priorStopped,
    diagnostics,
    warnings,
  });
}

/**
 * @param {string} serverId
 * @param {any} record
 * @returns {any}
 */
function describeServer(serverId, record) {
  const alive = record.exited !== true;
  return sanitizeForJson({
    serverId,
    running: alive,
    mode: record.mode,
    port: record.port,
    pid: record.pid,
    baseUrl: baseUrlOf(record.port),
    cwd: record.cwd,
    exePath: record.exePath,
    configPath: record.configPath,
    command: record.command,
    uptimeMs: alive ? Date.now() - record.startedAt : 0,
    exitCode: record.exited !== true ? null : (record.exitCode ?? null),
    exitSignal: record.exited === true && record.exitSignal !== null ? record.exitSignal : undefined,
  });
}

/**
 * @param {any} ctx
 * @param {any} config
 */
export function apply(ctx, config) {
  const resolved = {
    exePath: trimOrEmpty(config?.exePath) || DEFAULT_EXE_PATH,
    defaultCwd: trimOrEmpty(config?.defaultCwd),
    defaultMode: MODES.includes(config?.defaultMode) ? config.defaultMode : DEFAULT_MODE,
    defaultRoot: trimOrEmpty(config?.defaultRoot),
    defaultConfigPaths: isPlainObject(config?.defaultConfigPaths) ? config.defaultConfigPaths : {},
    readyPath: trimOrEmpty(config?.readyPath) || DEFAULT_READY_PATH,
    readyTimeoutMs: positiveOrDefault(config?.readyTimeoutMs, READY_TIMEOUT_MS),
    stopTimeoutMs: positiveOrDefault(config?.stopTimeoutMs, DEFAULT_STOP_TIMEOUT_MS),
    maxOutputBytes: clamp(config?.maxOutputBytes ?? DEFAULT_MAX_OUTPUT_BYTES, MIN_OUTPUT_BYTES, MAX_OUTPUT_BYTES),
    guidanceOrder: config?.guidanceOrder ?? DEFAULT_GUIDANCE_ORDER,
  };
  if (!Number.isFinite(resolved.guidanceOrder)) {
    throw new Error(`kitsu-supplement: guidanceOrder must be a finite number, got ${String(config?.guidanceOrder)}`);
  }

  const exists = async (path) => {
    const fs = await import("node:fs/promises");
    try {
      await fs.access(path);
      return true;
    } catch {
      return false;
    }
  };

  const writeConfigFile = async (serverId, value) => {
    const fs = await import("node:fs/promises");
    const os = await import("node:os");
    const path = await import("node:path");
    const dir = path.join(os.tmpdir(), "dsh-kitsu-supplement");
    await fs.mkdir(dir, { recursive: true });
    const file = path.join(dir, `${serverId}.json`);
    await fs.writeFile(file, `${JSON.stringify(value, null, 2)}\n`, "utf8");
    return file;
  };

  ctx.effect(() => () => {
    for (const [id, record] of registry) {
      registry.delete(id);
      if (record.exited === true) continue;
      try {
        record.handle.terminate();
      } catch {
        // 卸载时尽力而为：未停稳的进程仍由子进程提供方的清理兜底
      }
    }
  });

  ctx.systemPrompt.section({
    name: GUIDANCE_SECTION,
    order: resolved.guidanceOrder,
    text: ({ scope }) =>
      ctx.tools.get("kitsu_server_start", scope) === undefined
        ? ""
        : [
            "`kitsu_server_start` 拉起本机 doodle_kitsu_supplement.exe HTTP 服务（测试前准备后端），返回的 `port` 就是传入的显式端口，就绪判定是对 `http://127.0.0.1:<port><readyPath>` 轮询 HTTP（任何状态码都算就绪，最多等 100 秒），测试脚本应引用返回的 `baseUrl` 而不是猜端口。",
            `\`port\` 必须显式传，且必须是 ${PORT_RANGE_TEXT} 之间的整数；端口被占用时工具默认失败并给出占用 PID，只有显式传 \`killExisting: true\` 才强杀。\`mode: \"local\"\`（或 \`local: true\`）会给进程加 \`--local\`，并把 \`dbPath\` 作为 \`--dbPath\` 传给服务端；不传 dbPath 时 local 分支落进内存库（\`:memory:\`），重启即丢。`,
            "`kitsu_server_status` 查看本插件的实例与端口占用；`kitsu_server_stop` 停掉本插件拉起的实例；本插件不杀别人启动的进程。",
            "`kitsu_server_sql` 对磁盘库执行 sql 文件或片段（填测试数据），写完运行中的进程立刻能查到，**不要**为它停服或重启；执行前不确定就先 `dryRun: true` 走一遍。",
          ].join(" "),
  });

  ctx.tools.register(
    defineTool({
      name: "kitsu_server_start",
      description:
        "Start the doodle_kitsu_supplement HTTP server for local testing and return a ready http://127.0.0.1:<port> " +
        "baseUrl. `mode: \"production\"` (default) lets " +
        "the server read its registry config and listen on config.port; `mode: \"local\"` adds `--local`, which also " +
        "turns off the registry-only paths (in-memory database unless `dbPath` is given). " +
        `\`port\` is REQUIRED and must be an integer in ${PORT_RANGE_TEXT}: the server is started with that exact ` +
        "`--port`, and readiness is decided by polling " +
        "`http://127.0.0.1:<port><readyPath>` until any HTTP status comes back (up to 100 s). The generated config JSON carries dbPath (db_path), frontEndPath " +
        "(kitsu_front_end_path), publicUrlPort (public_url_port) and root; the working directory is `cwd` (default: the " +
        "exe's own directory, which holds its DLLs). When the port is already taken the call fails with code " +
        "`port-in-use` and reports the owning PIDs; pass `killExisting: true` only when those processes should be " +
        "force-killed first. The server keeps running after the call returns — it is tracked for `kitsu_server_status` " +
        "and `kitsu_server_stop`, and terminated when the plugin unloads.",
      parameters: {
        port: {
          type: "number",
          required: true,
          description:
            `Listening port, required and must be an integer within ${PORT_RANGE_TEXT} (no random-port fallback). ` +
            "Required in both modes; values outside the range are rejected.",
        },
        mode: {
          type: "string",
          enum: MODES,
          description:
            "`production` (default): commits go into the DB and the registry config supplies public_url / proxy. " +
            "`local`: the exe also receives `--local` (in-memory DB, dbPath ignored).",
        },
        local: {
          type: "boolean",
          description: "Shortcut: true is the same as mode=\"local\". Ignored when mode is given.",
        },
        cwd: {
          type: "string",
          description:
            "Absolute working directory of the server process. Defaults to the configured defaultCwd, then to the " +
            "exe's own directory (keep it there unless you know the exe needs another root).",
        },
        dbPath: {
          type: "string",
          description:
            "SQLite database file for the server config (`db_path`), also passed on the command line as `--dbPath`. " +
            "In local mode omitting it means the in-memory database (`:memory:`), which no external process can write.",
        },
        root: {
          type: "string",
          description:
            "Absolute project-data root. Passed as `--root` and used as config fallback for the server's root. " +
            "The local branch currently hardcodes its own root and ignores it.",
        },
        frontEndPath: {
          type: "string",
          description: "Kitsu front-end dist directory for the server config (`kitsu_front_end_path`).",
        },
        publicUrlPort: {
          type: "string",
          description: "Public URL port for the server config (`public_url_port`).",
        },
        extraConfig: {
          type: "object",
          additionalProperties: true,
          description:
            "Extra keys merged verbatim into the generated server config JSON. `port` is rejected: the `port` " +
            "parameter is the only source of truth.",
        },
        extraArgs: {
          type: "array",
          items: { type: "string" },
          description:
            "Extra argv items appended after the generated flags. Generated flags use the `--flag=value` form " +
            "(`--config=<path>`, `--root=<path>`, `--port=<n>`); `--local`, `--config`, `--port`, `--root` are " +
            "rejected because the parameters own them.",
        },
        exePath: {
          type: "string",
          description: "Per-call override of the executable path (defaults to the plugin config).",
        },
        readyPath: {
          type: "string",
          description: "HTTP path polled until the server answers (default `/`). Any status code counts as ready.",
        },
        killExisting: {
          type: "boolean",
          description:
            "Force-kill whatever listens on `port` before starting. Default false: the call fails with `port-in-use`.",
        },
      },
      output: { schema: START_SCHEMA, render: renderStart },
      isConcurrencySafe() {
        return false;
      },
      async execute(args, exec) {
        const requestedMode = trimOrEmpty(args.mode) || (args.local === true ? "local" : "") || resolved.defaultMode;
        const plan = await resolveSpec({ config: resolved, args: { ...args, mode: requestedMode }, exists });
        if (plan.problems.length > 0) {
          return startFailure(plan.spec, "invalid-args", plan.problems.join(" "), {
            diagnostics: plan.problems.map((message) => ({ severity: "error", code: "args/invalid", message })),
          });
        }
        const spec = plan.spec;
        const serverId = allocateServerId();
        const configPath = await writeConfigFile(serverId, spec.configValues);
        const argv = buildArgv({
          mode: spec.mode,
          port: spec.port,
          configPath,
          root: spec.root,
          dbPath: spec.dbPath,
          extraArgs: spec.extraArgs,
        });
        try {
          return await startServer(ctx, {
            serverId,
            exePath: spec.exePath,
            argv,
            cwd: spec.cwd,
            command: `${spec.exePath} ${renderArgv(argv)}`,
            mode: spec.mode,
            port: spec.port,
            configPath,
            dbPath: spec.dbPath ?? MEMORY_DB,
            readyPath: spec.readyPath,
            killExisting: args.killExisting === true,
            readyTimeoutMs: resolved.readyTimeoutMs,
            stopTimeoutMs: resolved.stopTimeoutMs,
            maxOutputBytes: resolved.maxOutputBytes,
            warnings: startWarnings(spec),
            signal: exec.signal,
          });
        } finally {
          const fs = await import("node:fs/promises");
          await fs.unlink(configPath).catch(() => {});
        }
      },
    }),
  );

  ctx.tools.register(
    defineTool({
      name: "kitsu_server_status",
      description:
        "List the doodle_kitsu_supplement instances this plugin started (id, mode, port, running/exited, baseUrl, " +
        "config path) and, for running instances, who is LISTENING on their ports (netstat). Pass `port` to also check " +
        "an arbitrary port. The port is always the explicitly requested one, never derived from the process.",
      parameters: {
        port: { type: "number", description: "Additional port to check for listeners." },
      },
      output: { schema: STATUS_SCHEMA, render: renderStatus },
      isConcurrencySafe() {
        return true;
      },
      async execute(args, exec) {
        /** @type {any[]} */
        const servers = [];
        /** @type {number[]} */
        const ports = [];
        for (const [id, record] of registry) {
          servers.push(describeServer(id, record));
          if (record.exited !== true && !ports.includes(record.port)) ports.push(record.port);
        }
        const extra = parsePort(args.port);
        if (extra !== undefined && !ports.includes(extra)) ports.push(extra);

        /** @type {any[]} */
        const portOwners = [];
        /** @type {any[]} */
        const diagnostics = [];
        for (const port of ports) {
          const probe = await probePort(ctx, port, exec.signal);
          /** @type {any} */
          const entry = { port, pids: probe.owners, ours: probe.ours };
          if (probe.error !== undefined) {
            entry.error = probe.error;
            diagnostics.push({ severity: "warning", code: "netstat/unavailable", message: `端口 ${port} 占用检查失败：${probe.error}` });
          }
          portOwners.push(entry);
        }
        const running = servers.filter((server) => server.running === true).length;
        return {
          ok: true,
          reason: servers.length === 0 ? "本插件没有记录任何实例。" : `本插件记录 ${servers.length} 个实例，${running} 个在运行。`,
          servers,
          portOwners,
          diagnostics,
        };
      },
    }),
  );

  ctx.tools.register(
    defineTool({
      name: "kitsu_server_stop",
      description:
        "Stop doodle_kitsu_supplement instances started by this plugin: terminate the process tree, wait for it to " +
        "settle, and drop it from the tracked list. With neither `serverId` nor `port`, every tracked instance is " +
        "stopped. Processes this plugin did not start are never killed.",
      parameters: {
        serverId: { type: "string", description: "Instance id from kitsu_server_start / kitsu_server_status (kitsu-N)." },
        port: { type: "number", description: "Stop every tracked instance listening on this port." },
      },
      output: { schema: STOP_SCHEMA, render: renderStop },
      isConcurrencySafe() {
        return false;
      },
      async execute(args, exec) {
        const wantedId = trimOrEmpty(args.serverId);
        const wantedPort = parsePort(args.port);
        /** @type {any[]} */
        const stopped = [];
        /** @type {any[]} */
        const diagnostics = [];
        for (const [id, record] of [...registry]) {
          if (wantedId !== "" && id !== wantedId) continue;
          if (wantedId === "" && wantedPort !== undefined && record.port !== wantedPort) continue;
          const result = await stopRecord(record, resolved.stopTimeoutMs);
          stopped.push({
            serverId: id,
            pid: record.pid,
            port: record.port,
            ok: result.ok,
            reason: result.reason,
            exitCode: result.exitCode ?? undefined,
            waitedMs: result.waitedMs,
          });
          if (result.ok) {
            registry.delete(id);
          } else {
            diagnostics.push({ severity: "error", code: "stop/failed", message: `${id}: ${result.reason}` });
            if (exec.signal?.aborted === true) break;
          }
        }
        if (stopped.length === 0) {
          diagnostics.push({ severity: "info", code: "stop/nothing", message: "没有匹配的实例。" });
        }
        return sanitizeForJson({
          ok: !diagnostics.some((item) => item.severity === "error"),
          reason: stopped.length === 0 ? "没有匹配的实例。" : `已处理 ${stopped.length} 个实例。`,
          stopped: stopped.map((item) => ({ ...item, exitCode: item.exitCode ?? null })),
          remaining: [...registry.keys()],
          diagnostics,
        });
      },
    }),
  );

  ctx.tools.register(
    defineTool({
      name: "kitsu_server_sql",
      description:
        "Run a SQL file or inline SQL against the doodle_kitsu_supplement database to change it or seed test data. " +
        "A running instance sees committed writes immediately: this tool never " +
        "stops or restarts the server. Sources run in order (`files[0]`, `files[1]`, ... then `sql`); everything runs in " +
        "a single transaction with `BEGIN IMMEDIATE`, so a failure rolls the whole batch back and reports the source " +
        "name, the statement number and the SQLite error. The target database is `dbPath`, else the dbPath of " +
        "`serverId`'s instance, else the plugin default. Connection pragmas mirror the server (`foreign_keys = ON`, " +
        "`synchronous = NORMAL`, `recursive_triggers = ON`) and the write lock waits up to 5s. ATTACH/DETACH are " +
        "rejected. `dryRun: true` executes and rolls back, so you can verify a script without persisting it. A " +
        "`:memory:` target (server started without dbPath) is refused with code `memory-db`.",
      parameters: {
        sql: {
          type: "string",
          description: "Inline SQL. May contain multiple statements; runs after every `files` entry.",
        },
        files: {
          type: "array",
          items: { type: "string" },
          description: "Absolute paths of SQL files, executed in the given order before `sql`.",
        },
        dbPath: {
          type: "string",
          description:
            "Target SQLite file. Defaults to the dbPath of `serverId`'s instance, then the plugin default. " +
            "`:memory:` is rejected.",
        },
        serverId: {
          type: "string",
          description: "Instance whose database is targeted (and noted in the result). It is not stopped or restarted.",
        },
        dryRun: {
          type: "boolean",
          description: "Run the whole batch inside a transaction and roll it back; nothing is persisted.",
        },
      },
      output: { schema: SQL_SCHEMA, render: renderSql },
      isConcurrencySafe() {
        return false;
      },
      async execute(args, exec) {
        const problems = validateSqlArgs(args);
        const fs = await import("node:fs/promises");
        /** @type {{label:string, sql:string}[]} */
        const batches = [];
        const fileList = Array.isArray(args.files) ? args.files : [];
        for (const file of fileList) {
          if (typeof file !== "string" || file.trim() === "") continue;
          const absolute = normalizePath(file);
          try {
            batches.push({ label: absolute, sql: await fs.readFile(absolute, "utf8") });
          } catch (error) {
            problems.push(`读不到 SQL 文件 ${absolute}：${errorText(error)}`);
          }
        }
        if (trimOrEmpty(args.sql) !== "") batches.push({ label: "<inline sql>", sql: String(args.sql) });

        const target = resolveSqlTarget({ args, config: resolved, registry });
        if (target.problem !== undefined) problems.push(target.problem);
        if (problems.length > 0) {
          return sanitizeForJson({
            ok: false,
            code: "invalid-args",
            reason: problems.join(" "),
            dbPath: target.dbPath,
            serverId: target.serverId,
            statements: 0,
            changes: 0,
            results: [],
            diagnostics: problems.map((message) => ({ severity: "error", code: "args/invalid", message })),
          });
        }
        if (batches.length === 0) {
          const message = "SQL 里没有任何可执行语句（文件为空或只有注释）。";
          return sanitizeForJson({
            ok: false,
            code: "empty",
            reason: message,
            dbPath: target.dbPath,
            serverId: target.serverId,
            statements: 0,
            changes: 0,
            results: [],
            diagnostics: [{ severity: "error", code: "sql/empty", message }],
          });
        }

        const outcome = await runSql({
          dbPath: target.dbPath,
          batches,
          dryRun: args.dryRun === true,
        });
        return sanitizeForJson({
          ok: outcome.ok,
          code: outcome.ok ? "ok" : "sql-failed",
          reason: outcome.ok
            ? `${args.dryRun === true ? "已校验并回滚" : "已提交"} ${outcome.statements} 条语句，影响 ${outcome.changes} 行（${target.dbPath}）。`
            : outcome.error,
          dbPath: target.dbPath,
          serverId: target.serverId,
          dryRun: args.dryRun === true ? true : undefined,
          statements: outcome.statements,
          changes: outcome.changes,
          results: outcome.results.slice(0, 200),
          diagnostics: outcome.ok
            ? []
            : [{ severity: "error", code: "sql/failed", message: String(outcome.error) }],
        });
      },
    }),
  );
}

//#endregion

export { errorText, sanitizeForJson };
