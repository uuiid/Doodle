// dsh-kitsu-supplement —— 通用：常量、路径/端口解析、argv、URL 与输出清洗
//
// 本文件只放纯逻辑：不依赖任何 import，也不访问 ctx / await 宿主资源。
// 常量在本文件唯一定义并导出，其余文件从这里 import（test/pure.mjs 不允许文件间同名导出）。
// test/pure.mjs 读取 lib/pure/dispatch.js 里登记的文件逐个求值，契约与原先的纯逻辑区段一致。


export const MODES = ["production", "local"];

/** 这些开关由参数生成，extraArgs 里再出现就是两个事实来源。 */

export const OWNED_FLAGS = ["--local", "--config", "--port", "--root", "--dbPath"];


export const DEFAULT_EXE_PATH = "E:\\build\\Ninja_debug\\bin\\doodle_kitsu_supplement_test.exe";

/** 显式传这个值表示仍要跑生产 exe；其余任何非空 exePath 都按字面路径使用。 */

export const PRODUCTION_EXE = "supplement-prod";

export const DEFAULT_PRODUCTION_EXE_PATH = "E:\\build\\Ninja_debug\\bin\\doodle_kitsu_supplement.exe";

export const DEFAULT_MODE = "production";

export const DEFAULT_READY_PATH = "/";
/** 就绪等待上限：显式端口已定，剩下的只是等进程把监听建立起来。 */

export const READY_TIMEOUT_MS = 100000;

export const DEFAULT_STOP_TIMEOUT_MS = 5000;

export const DEFAULT_MAX_OUTPUT_BYTES = 8 * 1024 * 1024;

export const DEFAULT_MAX_TAIL_CHARS = 4000;

export const DEFAULT_GUIDANCE_ORDER = 1396;

export const MIN_OUTPUT_BYTES = 64 * 1024;

export const MAX_OUTPUT_BYTES = 64 * 1024 * 1024;

export const GUIDANCE_SECTION = "tool:kitsu-supplement";

/** 系统工具用绝对路径：提供方对裸名称按清理后的 PATH 解析，结果随宿主环境变化。 */

export const NETSTAT_EXE = "C:\\Windows\\System32\\netstat.exe";

export const TASKKILL_EXE = "C:\\Windows\\System32\\taskkill.exe";
/** 允许的监听端口闭区间；端口必须落在区间内，测试脚本才能引用固定地址。 */

export const PORT_MIN = 50030;

export const PORT_MAX = 50330;
/** 内存库标记：服务端没拿到 --dbPath 时落在 ":memory:"（kitsu_supplement.cpp:178）。 */

export const MEMORY_DB = ":memory:";
/** sql 执行时等待写锁的上限，运行中的服务端可能正好在写。 */

export const SQL_BUSY_TIMEOUT_MS = 5000;

export const PORT_RANGE_TEXT = `${PORT_MIN}-${PORT_MAX}`;

/**
 * @param {unknown} value
 * @returns {boolean}
 */

/**
 * @param {unknown} value
 * @returns {boolean}
 */
export function isPlainObject(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

/**
 * @param {unknown} value
 * @returns {string}
 */

/**
 * @param {unknown} value
 * @returns {string}
 */
export function trimOrEmpty(value) {
  return typeof value === "string" ? value.trim() : "";
}

/**
 * 统一成反斜杠、去掉末尾分隔符（服务端两种斜杠都认，这里只为展示与去重一致）。
 *
 * @param {string} value
 * @returns {string}
 */

/**
 * 统一成反斜杠、去掉末尾分隔符（服务端两种斜杠都认，这里只为展示与去重一致）。
 *
 * @param {string} value
 * @returns {string}
 */
export function normalizePath(value) {
  return value.replace(/\//g, "\\").replace(/\\+$/, "");
}

/**
 * 把 "50123" / 50123 解析成端口号；不合法返回 undefined。
 *
 * @param {unknown} value
 * @returns {number|undefined}
 */

/**
 * 把 "50123" / 50123 解析成端口号；不合法返回 undefined。
 *
 * @param {unknown} value
 * @returns {number|undefined}
 */
export function parsePort(value) {
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

/**
 * 端口是否落在 [PORT_MIN, PORT_MAX] 闭区间内。
 *
 * @param {number|undefined} port
 * @returns {boolean}
 */
export function portInRange(port) {
  return typeof port === "number" && port >= PORT_MIN && port <= PORT_MAX;
}

/**
 * 校验启动参数，返回全部问题（空数组表示通过）。
 *
 * @param {{mode:string, port:unknown, cwd?:string, root?:string, exePath?:string, readyPath?:string}} spec
 * @returns {string[]}
 */

/**
 * `0.0.0.0:50025` / `[::]:50025` -> 50025。
 *
 * @param {string} local
 * @returns {number|undefined}
 */
export function localPortOf(local) {
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

/**
 * 从 `netstat -ano` 输出里找出监听目标端口的 PID（去重升序）。
 *
 * @param {string} text
 * @param {number} port
 * @returns {number[]}
 */
export function parseNetstatOwners(text, port) {
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

/**
 * 命令行展示形式（仅用于 render / 日志，不参与执行）。
 *
 * @param {string[]} argv
 * @returns {string}
 */
export function renderArgv(argv) {
  return argv.map((part) => (/[\s"]/.test(part) ? `"${part.replace(/"/g, '\\"')}"` : part)).join(" ");
}

/**
 * @param {number} port
 * @returns {string}
 */

/**
 * @param {number} port
 * @returns {string}
 */
export function baseUrlOf(port) {
  return `http://127.0.0.1:${port}`;
}

/**
 * @param {number} port
 * @param {string} readyPath
 * @returns {string}
 */

/**
 * @param {number} port
 * @param {string} readyPath
 * @returns {string}
 */
export function readyUrlOf(port, readyPath) {
  const path = String(readyPath ?? "/");
  return `${baseUrlOf(port)}${path.startsWith("/") ? path : `/${path}`}`;
}

/**
 * @param {unknown} error
 * @returns {string}
 */

/**
 * @param {unknown} error
 * @returns {string}
 */
export function errorText(error) {
  if (error === null || error === undefined) return "unknown error";
  if (typeof error === "string") return error;
  if (typeof error.message === "string") return error.message;
  return String(error);
}

/**
 * @param {string} value
 * @returns {string}
 */

/**
 * @param {string} value
 * @returns {string}
 */
export function stripLoneSurrogates(value) {
  return value.replace(/[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/g, "\uFFFD");
}

/**
 * 投影成「确定能无损序列化」的对象：丢掉 undefined 键，替换孤立代理项。
 * Host 对返回值做无损 JSON 校验，undefined 键或孤立代理项会让已经成功的调用整体失败。
 *
 * @param {any} value
 * @returns {any}
 */

/**
 * 投影成「确定能无损序列化」的对象：丢掉 undefined 键，替换孤立代理项。
 * Host 对返回值做无损 JSON 校验，undefined 键或孤立代理项会让已经成功的调用整体失败。
 *
 * @param {any} value
 * @returns {any}
 */
export function sanitizeForJson(value) {
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

/**
 * 从文本尾部截取 maxChars 个字符（行对齐）。
 *
 * @param {string} text
 * @param {number} maxChars
 * @returns {string}
 */
export function tailText(text, maxChars) {
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

/**
 * @param {unknown} value
 * @param {number} fallback
 * @returns {number}
 */
export function positiveOrDefault(value, fallback) {
  return typeof value === "number" && Number.isFinite(value) && value > 0 ? value : fallback;
}

/**
 * @param {number} value
 * @param {number} min
 * @param {number} max
 * @returns {number}
 */

/**
 * @param {number} value
 * @param {number} min
 * @param {number} max
 * @returns {number}
 */
export function clamp(value, min, max) {
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
