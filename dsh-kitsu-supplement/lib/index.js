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
 * 2) 真实端口只认 stdout 第一行（http_listener.cpp:35）；共享内存里的端口是全局单例，
 *    会被后启动的实例覆盖，不能当作某次启动的事后判据。
 * 3) 固定端口先探占用。默认失败并给出占用 PID，只有显式 killExisting:true 才强杀。
 * 4) --config 由插件生成。服务端 config 解析失败会 return true 且**不启动 HTTP**，
 *    进程可能一闪而过，所以 JSON 只由插件写，模型的额外字段并进 extraConfig。
 *    开关一律写成 `--key <value>` 两个 argv 项：argh 把 `--key=value` 只记进 params_，
 *    而服务端用 arg_({...}) 查 flags_，`--config=x` 会被静默当成没给。
 * 5) cwd 默认取 exe 所在目录（exe 依赖同目录的 dll）；运行路径也可由参数/配置显式传入。
 * 6) 不用服务端的 POST /api/doodle/stop-server：它要 admin JWT，且连接数不为 0 时拒绝退出
 *    （base_data.cpp:343-366），测试机上的连接数恰恰不为 0。这里直接终止进程树并等停稳。
 *
 * 本文件按 DSH 插件惯例不使用顶层 node: 静态导入；内置模块在用到的位置动态 import。
 */

import { defineTool } from "@deepseek-ai/dsh-tools";
import z from "@deepseek-ai/schemastery";

//#region 纯逻辑（校验、argv、端口行解析、渲染）
// 本区段不依赖任何 import，test/pure.mjs 直接求值；下一个区段是配置与输出契约。

/** 启动模式；local 对应 argv 里的 `--local`。 */
const MODES = ["production", "local"];

/** 这些开关由参数生成，extraArgs 里再出现就是两个事实来源。 */
const OWNED_FLAGS = ["--local", "--config", "--port", "--root"];

const DEFAULT_EXE_PATH = "E:\\build\\Ninja_debug\\bin\\doodle_kitsu_supplement.exe";
const DEFAULT_MODE = "production";
const DEFAULT_READY_PATH = "/";
const DEFAULT_READY_TIMEOUT_MS = 30000;
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
/** 等 stdout 端口行的上限（与服务端 listen 的耗时无关，只用来看住"进程没打印就退出"）。 */
const PORT_LINE_TIMEOUT_MS = 10000;
/** 允许的监听端口闭区间；端口必须落在区间内，测试脚本才能引用固定地址。 */
const PORT_MIN = 50030;
const PORT_MAX = 50330;
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
 * `--config <json>` -> `--root <path>` -> `--local` -> `--port <n>` -> extraArgs。
 *
 * 服务端用 argh 解析，`--key=value` 只会落进 params_（argh.h:196-204），
 * 而 arg_({...}) / arg_[...] 查的是 flags_，所以 `--config=x` 会被当成"没给"。
 *
 * @param {{mode:string, port:number, configPath?:string, root?:string, extraArgs?:string[]}} spec
 * @returns {string[]}
 */
function buildArgv({ mode, port, configPath, root, extraArgs }) {
  /** @type {string[]} */
  const argv = [];
  if (configPath) argv.push("--config", configPath);
  if (root) argv.push("--root", root);
  if (mode === "local") argv.push("--local");
  argv.push("--port", String(port));
  for (const item of extraArgs ?? []) argv.push(item);
  return argv;
}

/**
 * 从 stdout 里找监听协程打印的端口行：第一行纯数字就是端口。
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
  if (dbPath !== "") overrides.db_path = normalizePath(dbPath);
  if (frontEndPath !== "") overrides.kitsu_front_end_path = normalizePath(frontEndPath);
  if (args.publicUrlPort !== undefined && args.publicUrlPort !== "") overrides.public_url_port = args.publicUrlPort;

  const spec = {
    mode,
    port,
    exePath,
    cwd,
    root: normalizedRoot,
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
  readyTimeoutMs: z.number().default(DEFAULT_READY_TIMEOUT_MS),
  stopTimeoutMs: z.number().default(DEFAULT_STOP_TIMEOUT_MS),
  maxOutputBytes: z.number().default(DEFAULT_MAX_OUTPUT_BYTES),
  guidanceOrder: z.number().default(DEFAULT_GUIDANCE_ORDER),
});

const SERVER_PROPERTIES = {
  mode: { type: "string", enum: MODES, description: "启动模式" },
  port: { type: "number", description: "实际监听端口（来自 stdout 端口行）" },
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
        "exited-before-listen",
        "no-port-line",
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

/**
 * @param {any} value
 * @returns {any[]}
 */
function renderStart(args, value) {
  /** @type {string[]} */
  const lines = [];
  if (value.ok === true) {
    lines.push(`已启动 ${value.serverId} (${value.mode})  ${value.baseUrl}  pid=${value.pid}`);
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
    lines.push("", `${server.serverId}  ${server.mode}  :${server.port}  pid=${server.pid}  ${state}`);
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
 * 读取 offset 之后新增的输出文本（偏移式、非消费式）。
 *
 * @param {any} reader
 * @param {number} offset
 * @returns {{text:string, offset:number}}
 */
function readNew(reader, offset) {
    if (!reader) return { text: "", offset };
    const chunk = reader.readFrom(offset);
    if (!chunk) return { text: "", offset };
    return {
      text: typeof chunk.text === "string" ? chunk.text : "",
      offset: typeof chunk.nextOffset === "number" ? chunk.nextOffset : offset,
    };
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
        if (typeof record.handle?.pid === "number") priorStopped.push(record.handle.pid);
      } else {
        diagnostics.push({ severity: "error", code: "kill/failed", message: `停止本插件实例 ${id} 失败：${result.reason}` });
      }
    }
    const ownPids = new Set([...registry.values()].map((record) => record.handle?.pid));
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
    portBuffer: "",
    portLine: undefined,
    exePath: spec.exePath,
    cwd: spec.cwd,
    configPath: spec.configPath,
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

  const portDeadline = Date.now() + Math.min(PORT_LINE_TIMEOUT_MS, spec.readyTimeoutMs);
  /** @type {number|undefined} */
  let portLine;
  while (portLine === undefined && record.exited !== true && Date.now() < portDeadline) {
    const read = readNew(handle.collected?.stdout, record.outputOffset);
    if (read.text.length > 0) {
      record.portBuffer += read.text;
      record.outputOffset = read.offset;
      portLine = findPortLine(record.portBuffer);
    }
    if (portLine === undefined) await sleep(50);
  }
  // 先认端口：stdio 的 tick 可能晚于 done 落到 record.exited，此时进程其实已经监听。
  const exitedEarly = record.exited === true;

  const stdoutText = () => readAll(handle.collected?.stdout);
  const stderrText = () => readAll(handle.collected?.stderr);
  const outputTail = () =>
    tailText([stdoutText().text, stderrText().text].filter((text) => text.length > 0).join("\n"), DEFAULT_MAX_TAIL_CHARS) || undefined;

  if (portLine === undefined) {
    await stopRecord(record, spec.stopTimeoutMs);
    const alive = !exitedEarly;
    diagnostics.push({
      severity: "error",
      code: "listen/unknown",
      message: "stdout 第一行应当是监听端口（http_listener.cpp:35 `std::cout << l_port`），但没有读出来。",
    });
    if (spec.configPath) {
      diagnostics.push({
        severity: "info",
        code: "config/hint",
        message: `服务端 --config 解析失败时会直接退出且不启动 HTTP（kitsu_supplement.cpp:158-163），请检查 ${spec.configPath}。`,
      });
    }
    return startFailure(
      spec,
      alive ? "no-port-line" : "exited-before-listen",
      alive
        ? `进程已启动（pid=${handle.pid}），但 ${Math.min(PORT_LINE_TIMEOUT_MS, spec.readyTimeoutMs)} ms 内没有打印监听端口，已终止。`
        : `进程在打印监听端口前就退出了（exit ${record.exitCode ?? "n/a"}）。`,
      {
        serverId,
        pid: handle.pid,
        diagnostics,
        outputTail: outputTail(),
        priorStopped,
      },
    );
  }

  record.port = portLine;
  record.portLine = portLine;
  registry.set(serverId, record);

  const ready = await waitForReady({
    port: portLine,
    readyPath: spec.readyPath,
    isDead: () => exitedEarly,
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
    return startFailure(
      spec,
      ready.dead ? "exited-before-ready" : ready.aborted ? "cancelled" : "ready-timeout",
      ready.dead
        ? `端口 ${portLine} 已监听，但进程随后退出（exit ${record.exitCode ?? "n/a"}）。`
        : ready.aborted
          ? `已取消：端口 ${portLine} 已监听，但就绪等待被中止，进程已终止。`
          : `端口 ${portLine} 已监听，但 ${spec.readyTimeoutMs} ms 内 GET ${readyUrlOf(portLine, spec.readyPath)} 没有响应，已终止。`,
      {
        serverId,
        pid: handle.pid,
        port: portLine,
        baseUrl: baseUrlOf(portLine),
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
    reason: `已启动并监听端口 ${portLine}（${spec.mode} 模式）。`,
    serverId,
    mode: spec.mode,
    port: portLine,
    pid: handle.pid,
    baseUrl: baseUrlOf(portLine),
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
    pid: record.handle?.pid,
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
    readyTimeoutMs: positiveOrDefault(config?.readyTimeoutMs, DEFAULT_READY_TIMEOUT_MS),
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
            "`kitsu_server_start` 拉起本机 doodle_kitsu_supplement.exe HTTP 服务（测试前准备后端），返回的 `port` / `baseUrl` 取自进程 stdout 的真实监听端口，测试脚本应引用它而不是猜端口。",
            `\`port\` 必须显式传，且必须是 ${PORT_RANGE_TEXT} 之间的整数；端口被占用时工具默认失败并给出占用 PID，只有显式传 \`killExisting: true\` 才强杀。\`mode: \"local\"\`（或 \`local: true\`）会给进程加 \`--local\`，服务端在该模式下忽略 dbPath/root 覆盖并改用内置数据库。`,
            "`kitsu_server_status` 查看本插件的实例与端口占用；`kitsu_server_stop` 停掉本插件拉起的实例；本插件不杀别人启动的进程。",
          ].join(" "),
  });

  ctx.tools.register(
    defineTool({
      name: "kitsu_server_start",
      description:
        "Start the doodle_kitsu_supplement HTTP server for local testing and return the REAL listening port (parsed " +
        "from the process stdout) plus a ready http://127.0.0.1:<port> baseUrl. `mode: \"production\"` (default) lets " +
        "the server read its registry config and listen on config.port; `mode: \"local\"` adds `--local`, which makes " +
        "the server use its built-in database and ignore dbPath (the local branch only accepts the explicit --port value). " +
        `\`port\` is REQUIRED and must be an integer in ${PORT_RANGE_TEXT}: without --port the local branch listens on a ` +
        "random port, and this node needs a stable, test-referable address. The generated config JSON carries dbPath (db_path), frontEndPath " +
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
          description: "SQLite database file for the server config (`db_path`). Ignored by the local branch.",
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
            "Extra argv items appended after the generated flags. Generated flags use the `--flag value` form " +
            "(`--config`, `--root`, `--port`); `--local`, `--config`, `--port`, `--root` are " +
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
        const argv = buildArgv({ mode: spec.mode, port: spec.port, configPath, root: spec.root, extraArgs: spec.extraArgs });
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
            readyPath: spec.readyPath,
            killExisting: args.killExisting === true,
            readyTimeoutMs: resolved.readyTimeoutMs,
            stopTimeoutMs: resolved.stopTimeoutMs,
            maxOutputBytes: resolved.maxOutputBytes,
            warnings:
              spec.mode === "local"
                ? ["local 模式会忽略 dbPath / root：服务端该分支使用内置数据库与硬编码 root，config 里只生效其它键。"]
                : [],
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
        "List the doodle_kitsu_supplement instances this plugin started (id, mode, port, PID, running/exited, baseUrl, " +
        "config path) and, for running instances, who is LISTENING on their ports (netstat). Pass `port` to also check " +
        "an arbitrary port. The shared-memory listener port is a global slot that any later instance overwrites, so it " +
        "is never used as evidence here.",
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
            pid: record.handle?.pid,
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
}

//#endregion

export { errorText, sanitizeForJson };
