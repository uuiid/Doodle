// dsh-kitsu-supplement —— kitsu_server_start：参数校验、config、argv、spec 解析
//
// 本文件只放纯逻辑：不依赖任何 import，也不访问 ctx / await 宿主资源。
// test/pure.mjs 读取 lib/pure/dispatch.js 里登记的文件逐个求值，契约与原先的纯逻辑区段一致。

import { DEFAULT_EXE_PATH, DEFAULT_MODE, DEFAULT_READY_PATH, MEMORY_DB, MODES, OWNED_FLAGS, PORT_RANGE_TEXT, baseUrlOf, isPlainObject, normalizePath, parsePort, portInRange, sanitizeForJson, trimOrEmpty } from "./common.js";

/**
 * 校验启动参数，返回全部问题（空数组表示通过）。
 *
 * @param {{mode:string, port:unknown, cwd?:string, root?:string, exePath?:string, readyPath?:string}} spec
 * @returns {string[]}
 */
export function validateStartArgs({ mode, port, cwd, root, exePath, readyPath }) {
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

/**
 * 校验透传的 extraArgs：拒绝插件自己管理的开关。
 *
 * @param {unknown} extraArgs
 * @returns {string[]}
 */
export function validateExtraArgs(extraArgs) {
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

/**
 * 校验 extraConfig。
 *
 * @param {unknown} extraConfig
 * @returns {string[]}
 */
export function validateExtraConfig(extraConfig) {
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

/**
 * 生成 config JSON：默认路径 -> extraConfig -> 参数覆盖；只保留有值的键。
 *
 * @param {{defaults:Record<string, unknown>, port:number, extraConfig?:Record<string, unknown>, overrides?:Record<string, unknown>}} spec
 * @returns {Record<string, unknown>}
 */
export function buildConfig({ defaults, port, extraConfig, overrides }) {
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
export function buildArgv({ mode, port, configPath, root, dbPath, extraArgs }) {
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

/**
 * 由配置与参数算出本次启动的全部事实。
 *
 * @param {object} deps
 * @param {any} deps.config
 * @param {any} deps.args
 * @param {(path:string) => Promise<boolean>} deps.exists
 * @returns {Promise<{problems:string[], spec:any}>}
 */
export async function resolveSpec({ config, args, exists }) {
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

/**
 * 启动成功/失败都要带上的提示。local 模式没给 dbPath 时会落进 ":memory:"，
 * 那个库只活在本进程里，kitsu_server_sql 写不进去。
 *
 * @param {{mode:string, dbPath?:string}} spec
 * @returns {string[]}
 */
export function startWarnings(spec) {
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

/**
 * 拼一个「启动失败」的结果对象（键集合与启动成功时一致）。
 *
 * @param {any} spec
 * @param {string} code
 * @param {string} reason
 * @param {Record<string, any>} [extra]
 * @returns {any}
 */
export function startFailure(spec, code, reason, extra = {}) {
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
