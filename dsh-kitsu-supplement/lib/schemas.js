// dsh-kitsu-supplement —— 配置与输出契约
//
// 只有这里依赖 @deepseek-ai/schemastery：Config、五个工具的 output schema 与 render。
// 纯逻辑常量与工具函数一律从 pure/ 取。

import z from "@deepseek-ai/schemastery";

import {
  DEFAULT_EXE_PATH,
  DEFAULT_GUIDANCE_ORDER,
  DEFAULT_MAX_OUTPUT_BYTES,
  DEFAULT_MODE,
  DEFAULT_READY_PATH,
  DEFAULT_STOP_TIMEOUT_MS,
  MODES,
  READY_TIMEOUT_MS,
} from "./pure/common.js";
import {
  PLAYWRIGHT_BASE_URL_ENV,
  PLAYWRIGHT_DEFAULT_CWD,
  PLAYWRIGHT_KILL_TIMEOUT_MS,
  PLAYWRIGHT_TIMEOUT_MS,
} from "./pure/playwright.js";

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
  playwrightCwd: z.string().default(PLAYWRIGHT_DEFAULT_CWD),
  playwrightNodePath: z.string().default(""),
  playwrightCliPath: z.string().default(""),
  playwrightDefaultBaseUrl: z.string().default(""),
  playwrightTimeoutMs: z.number().default(PLAYWRIGHT_TIMEOUT_MS),
  playwrightKillTimeoutMs: z.number().default(PLAYWRIGHT_KILL_TIMEOUT_MS),
  stateDir: z.string().default(""),
  instancesFile: z.string().default(""),
  stopTimeoutMs: z.number().default(DEFAULT_STOP_TIMEOUT_MS),
  maxOutputBytes: z.number().default(DEFAULT_MAX_OUTPUT_BYTES),
  guidanceOrder: z.number().default(DEFAULT_GUIDANCE_ORDER),
});

export const SERVER_PROPERTIES = {
  mode: { type: "string", enum: MODES, description: "启动模式" },
  port: { type: "number", description: "监听端口（就是启动时传入的 --port）" },
  pid: { type: "number", description: "服务进程 PID" },
  baseUrl: { type: "string", description: "http://127.0.0.1:<port>" },
  cwd: { type: "string", description: "服务进程工作目录" },
  exePath: { type: "string", description: "可执行文件" },
  configPath: { type: "string", description: "为本次启动生成的 config JSON 路径" },
  command: { type: "string", description: "等价命令行" },
};

export const DIAGNOSTIC_ITEM = {
  type: "object",
  additionalProperties: false,
  properties: {
    severity: { type: "string", enum: ["info", "warning", "error"], required: true },
    code: { type: "string", required: true },
    message: { type: "string", required: true },
  },
};

export const START_SCHEMA = {
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
        "slot-in-use",
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
    replaced: {
      type: "array",
      required: true,
      items: {
        type: "object",
        additionalProperties: false,
        properties: {
          serverId: { type: "string", required: true },
          port: { type: "number" },
          ok: { type: "boolean", required: true },
          reason: { type: "string", required: true },
        },
      },
    },
    diagnostics: { type: "array", required: true, items: DIAGNOSTIC_ITEM },
    warnings: { type: "array", required: true, items: { type: "string" } },
    statePath: { type: "string", description: "实例快照文件路径" },
    outputTail: { type: "string", description: "失败时的 stdout+stderr 尾巴" },
    spillPath: { type: "string", description: "输出溢出时的完整日志文件" },
  },
};

export const SERVER_STATUS_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    serverId: { type: "string", required: true },
    running: { type: "boolean", required: true },
    ...SERVER_PROPERTIES,
    uptimeMs: { type: "number", required: true },
    exitCode: { type: "number" },
    exitSignal: { type: "string" },
    adopted: { type: "boolean", description: "来自状态文件、本进程没有句柄的实例" },
  },
};

export const SLOT_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    serverId: { type: "string", required: true },
    port: { type: "number" },
    baseUrl: { type: "string" },
    pid: { type: "number" },
    uptimeMs: { type: "number", required: true },
  },
};

export const SLOTS_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    local: { oneOf: [SLOT_SCHEMA, { type: "null" }] },
    production: { oneOf: [SLOT_SCHEMA, { type: "null" }] },
  },
};

export const USAGE_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    local: { type: "number", required: true },
    production: { type: "number", required: true },
    running: { type: "number", required: true },
    limitPerMode: { type: "number", required: true },
  },
};

export const STATUS_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    ok: { type: "boolean", required: true },
    reason: { type: "string", required: true },
    servers: { type: "array", required: true, items: SERVER_STATUS_SCHEMA },
    slots: SLOTS_SCHEMA,
    usage: USAGE_SCHEMA,
    available: { type: "array", required: true, items: { type: "string" } },
    statePath: { type: "string" },
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

export const STOP_SCHEMA = {
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

export const SQL_SCHEMA = {
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
export function renderStart(args, value) {
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

export const PLAYWRIGHT_SUMMARY_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    passed: { type: "number", required: true },
    failed: { type: "number", required: true },
    skipped: { type: "number", required: true },
    didNotRun: { type: "number", required: true },
    flaky: { type: "number", required: true },
  },
};

export const PLAYWRIGHT_SCHEMA = {
  type: "object",
  additionalProperties: false,
  properties: {
    ok: { type: "boolean", required: true },
    code: {
      type: "string",
      required: true,
      enum: ["ok", "tests-failed", "busy", "timeout", "cancelled", "spawn-failed", "timeout-cli", "invalid-args", "exit-nonzero"],
    },
    reason: { type: "string", required: true },
    runId: { type: "string", required: true },
    command: { type: "string", required: true },
    args: { type: "array", required: true, items: { type: "string" } },
    cwd: { type: "string", required: true },
    cliPath: { type: "string", required: true },
    baseUrl: { type: "string", description: "子进程实际使用的 DOODLE_BASE_URL；空表示沿用父进程环境" },
    baseUrlSource: { type: "string", required: true, enum: ["param", "config", "env", "none"] },
    exitCode: { type: "number" },
    durationMs: { type: "number", required: true },
    summary: PLAYWRIGHT_SUMMARY_SCHEMA,
    failures: {
      type: "array",
      required: true,
      items: {
        type: "object",
        additionalProperties: false,
        properties: {
          title: { type: "string", required: true },
          error: { type: "string", required: true },
        },
      },
    },
    listed: {
      type: "object",
      additionalProperties: false,
      properties: {
        tests: { type: "number", required: true },
        files: { type: "number", required: true },
        titles: { type: "array", required: true, items: { type: "string" } },
      },
    },
    reportDir: { type: "string" },
    outputDir: { type: "string" },
    diagnostics: { type: "array", required: true, items: DIAGNOSTIC_ITEM },
    warnings: { type: "array", required: true, items: { type: "string" } },
    outputTail: { type: "string", description: "stdout + stderr 的尾部" },
    spillPath: { type: "string", description: "输出溢出时的完整日志文件" },
  },
};

/**
 * @returns {any}
 */
export function emptyPlaywrightSummary() {
  return { passed: 0, failed: 0, skipped: 0, didNotRun: 0, flaky: 0 };
}

/**
 * @param {any} args
 * @param {any} value
 * @returns {any[]}
 */
export function renderPlaywright(args, value) {
  /** @type {string[]} */
  const lines = [];
  lines.push(`${value.ok === true ? "OK" : "FAILED"} [${value.code}] ${value.reason}`);
  if (value.command) lines.push(`command: ${value.command}`);
  if (value.cwd) lines.push(`cwd: ${value.cwd}`);
  lines.push(`baseUrl: ${value.baseUrl || `（沿用父进程 ${PLAYWRIGHT_BASE_URL_ENV}）`}  [${value.baseUrlSource}]`);
  if (value.exitCode !== undefined) lines.push(`exit: ${value.exitCode}  耗时 ${value.durationMs} ms`);
  const summary = value.summary ?? emptyPlaywrightSummary();
  if (summary.passed + summary.failed + summary.skipped + summary.didNotRun + summary.flaky > 0) {
    const flaky = summary.flaky > 0 ? ` flaky=${summary.flaky}` : "";
    lines.push(`summary: passed=${summary.passed} failed=${summary.failed} skipped=${summary.skipped} didNotRun=${summary.didNotRun}${flaky}`);
  }
  if (value.listed !== undefined) {
    lines.push(`listing: ${value.listed.tests} 条用例 / ${value.listed.files} 个文件`);
  }
  for (const failure of value.failures ?? []) {
    lines.push(`FAILED  ${failure.title}`);
    if (failure.error) lines.push(`        ${failure.error}`);
  }
  for (const item of value.diagnostics ?? []) lines.push(`${item.severity.toUpperCase()} ${item.code}: ${item.message}`);
  for (const warning of value.warnings ?? []) lines.push(`WARNING: ${warning}`);
  if (value.reportDir) lines.push(`report: ${value.reportDir}`);
  if (value.outputTail) lines.push("", "--- playwright output tail ---", value.outputTail);
  if (value.spillPath) lines.push(`full log: ${value.spillPath}`);
  return [{ type: "text", text: lines.join("\n") }];
}

/**
 * @param {any} args
 * @param {any} value
 * @returns {any[]}
 */
export function renderStatus(args, value) {
  /** @type {string[]} */
  const lines = [value.reason];
  if (value.slots !== undefined) {
    for (const mode of MODES) {
      const slot = value.slots?.[mode];
      if (slot === null || slot === undefined) {
        lines.push(`${mode} 槽位: 空闲`);
        continue;
      }
      lines.push(`${mode} 槽位: ${slot.serverId} :${slot.port ?? "?"}  pid=${slot.pid ?? "?"}  已运行 ${slot.uptimeMs} ms`);
    }
    const free = Array.isArray(value.available) ? value.available : [];
    lines.push(`剩余槽位: ${free.length > 0 ? free.join(" / ") : "无"}`);
  }
  for (const server of value.servers ?? []) {
    const state = server.running === true ? "运行中" : `已退出(exit ${server.exitCode ?? "n/a"})`;
    const adopted = server.adopted === true ? "  [来自状态文件]" : "";
    lines.push("", `${server.serverId}  ${server.mode}  :${server.port}  ${state}${adopted}`);
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
export function renderStop(args, value) {
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
export function renderSql(args, value) {
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
