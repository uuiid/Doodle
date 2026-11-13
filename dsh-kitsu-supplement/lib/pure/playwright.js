// dsh-kitsu-supplement —— kitsu_playwright_test：参数、env、输出解析
//
// 本文件只放纯逻辑：不依赖任何 import，也不访问 ctx / await 宿主资源。
// test/pure.mjs 读取 lib/pure/dispatch.js 里登记的文件逐个求值，契约与原先的纯逻辑区段一致。

import { normalizePath, renderArgv, tailText, trimOrEmpty } from "./common.js";

export const PLAYWRIGHT_BASE_URL_ENV = "DOODLE_BASE_URL";

export const PLAYWRIGHT_HTML_OPEN_ENV = "PLAYWRIGHT_HTML_OPEN";

export const PLAYWRIGHT_HTML_REPORT_ENV = "PLAYWRIGHT_HTML_REPORT";

export const PLAYWRIGHT_OUTPUT_DIR_ENV = "PLAYWRIGHT_OUTPUT_DIR";

export const PLAYWRIGHT_RESULTS_DIR = "test-results";

export const PLAYWRIGHT_REPORT_DIR = "playwright-report";

export const PLAYWRIGHT_DEFAULT_CWD = "E:\\Doodle\\test_js";

export const PLAYWRIGHT_CLI_RELATIVE = "node_modules/@playwright/test/cli.js";

export const PLAYWRIGHT_TIMEOUT_MS = 900000;

export const PLAYWRIGHT_KILL_TIMEOUT_MS = 5000;

export const PLAYWRIGHT_MAX_LISTED = 500;

/**
 * 校验 kitsu_playwright_test 的透传参数。
 *
 * playwright 自己的命令行以子命令开头（test / show-report / install ...），
 * 这里不做语义校验，只保证每一项能原样放进 argv。
 *
 * @param {unknown} args
 * @returns {string[]}
 */

/**
 * 校验 kitsu_playwright_test 的透传参数。
 *
 * playwright 自己的命令行以子命令开头（test / show-report / install ...），
 * 这里不做语义校验，只保证每一项能原样放进 argv。
 *
 * @param {unknown} args
 * @returns {string[]}
 */
export function validatePlaywrightArgs(args) {
  if (args === undefined) return ["args 必须给出：它是 playwright CLI 后面的参数，例如 [\"test\"]、[\"test\", \"face_capture.js\"]、[\"test\", \"--list\"]。"];
  if (!Array.isArray(args)) return ["args 必须是字符串数组。"];
  if (args.length === 0) return ["args 不能为空：至少要给 playwright 子命令，例如 [\"test\"]。"];
  /** @type {string[]} */
  const problems = [];
  for (const item of args) {
    if (typeof item !== "string" || item.trim() === "") {
      problems.push(`args 含非法项 ${JSON.stringify(item)}（必须是非空字符串）。`);
    }
  }
  return problems;
}

/**
 * 把 baseUrl 解析成 explicit / none / invalid。
 *
 * 插件不替调用方决定地址：不传就完全沿用父进程环境（playwright.config.js:17 自己兜底 localhost:50025）。
 *
 * @param {unknown} baseUrl
 * @returns {{kind:string, value?:string, problem?:string}}
 */

/**
 * 把 baseUrl 解析成 explicit / none / invalid。
 *
 * 插件不替调用方决定地址：不传就完全沿用父进程环境（playwright.config.js:17 自己兜底 localhost:50025）。
 *
 * @param {unknown} baseUrl
 * @returns {{kind:string, value?:string, problem?:string}}
 */
export function parseBaseUrlArg(baseUrl) {
  const text = trimOrEmpty(baseUrl);
  if (text === "") return { kind: "none" };
  const normalized = text.replace(/\/+$/, "");
  if (!/^https?:\/\/[^\s/]+$/i.test(normalized)) {
    return { kind: "invalid", problem: `baseUrl 必须是 http(s)://host[:port]，收到 ${JSON.stringify(normalized)}。` };
  }
  return { kind: "explicit", value: normalized };
}

/**
 * 修正环境变量的名字大小写：Windows 的环境变量不区分大小写，但传给 node 的对象是普通对象。
 *
 * @param {Record<string, string|undefined>} baseEnv
 * @param {string} name
 * @returns {string|undefined}
 */

/**
 * 修正环境变量的名字大小写：Windows 的环境变量不区分大小写，但传给 node 的对象是普通对象。
 *
 * @param {Record<string, string|undefined>} baseEnv
 * @param {string} name
 * @returns {string|undefined}
 */
export function lookupEnv(baseEnv, name) {
  const wanted = name.toUpperCase();
  for (const key of Object.keys(baseEnv ?? {})) {
    if (key.toUpperCase() === wanted) return baseEnv[key];
  }
  return undefined;
}

/**
 * 在继承的环境上覆盖若干键：显式 env 优先于插件内部注入，且同名键只保留一个。
 *
 * @param {{baseEnv:Record<string, string|undefined>, injected:Record<string, string>, explicit?:Record<string, string>}} spec
 * @returns {Record<string, string>}
 */

/**
 * 在继承的环境上覆盖若干键：显式 env 优先于插件内部注入，且同名键只保留一个。
 *
 * @param {{baseEnv:Record<string, string|undefined>, injected:Record<string, string>, explicit?:Record<string, string>}} spec
 * @returns {Record<string, string>}
 */
export function mergePlaywrightEnv({ baseEnv, injected, explicit }) {
  /** @type {Record<string, string>} */
  const result = {};
  /** @type {Map<string, string>} */
  const renamed = new Map();
  for (const [key, value] of Object.entries(baseEnv ?? {})) {
    if (typeof value !== "string") continue;
    const upper = key.toUpperCase();
    if (renamed.has(upper)) {
      result[renamed.get(upper)] = value;
      continue;
    }
    renamed.set(upper, key);
    result[key] = value;
  }
  for (const [key, value] of Object.entries(injected ?? {})) {
    if (typeof value !== "string") continue;
    const upper = key.toUpperCase();
    const existing = renamed.get(upper);
    if (existing !== undefined) {
      result[existing] = value;
      continue;
    }
    renamed.set(upper, key);
    result[key] = value;
  }
  for (const [key, value] of Object.entries(explicit ?? {})) {
    if (typeof value !== "string" || key.trim() === "") continue;
    const upper = key.toUpperCase();
    const wanted = key.trim();
    const existing = renamed.get(upper);
    if (existing !== undefined) {
      delete result[existing];
      renamed.set(upper, wanted);
    } else {
      renamed.set(upper, wanted);
    }
    result[wanted] = value;
  }
  return result;
}

/**
 * playwright 报告目录与产物目录的落点。
 *
 * @param {{cwd:string, runId:string, isolate:boolean, tmpRoot:string}} spec
 * @returns {{reportDir:string, outputDir:string}}
 */

/**
 * playwright 报告目录与产物目录的落点。
 *
 * @param {{cwd:string, runId:string, isolate:boolean, tmpRoot:string}} spec
 * @returns {{reportDir:string, outputDir:string}}
 */
export function playwrightOutputDirs({ cwd, runId, isolate, tmpRoot }) {
  if (!isolate) {
    return {
      reportDir: `${cwd}/${PLAYWRIGHT_REPORT_DIR}`,
      outputDir: `${cwd}/${PLAYWRIGHT_RESULTS_DIR}`,
    };
  }
  return {
    reportDir: `${tmpRoot}/${runId}/${PLAYWRIGHT_REPORT_DIR}`,
    outputDir: `${tmpRoot}/${runId}/${PLAYWRIGHT_RESULTS_DIR}`,
  };
}

/**
 * 入口脚本的兜底路径。`@playwright/test` 是纯 ESM 包，不能 require.resolve，
 * 只按 hoisted 布局拼一个路径，不存在时由调用方决定怎么报。
 *
 * @param {string} cwd
 * @returns {string}
 */

/**
 * 入口脚本的兜底路径。`@playwright/test` 是纯 ESM 包，不能 require.resolve，
 * 只按 hoisted 布局拼一个路径，不存在时由调用方决定怎么报。
 *
 * @param {string} cwd
 * @returns {string}
 */
export function defaultPlaywrightCli(cwd) {
  return `${normalizePath(cwd)}\\${PLAYWRIGHT_CLI_RELATIVE.replace(/\//g, "\\")}`;
}

/**
 * @param {string} cwd
 * @returns {string}
 */

/**
 * @param {string} cwd
 * @returns {string}
 */
export function playwrightLockPath(cwd) {
  return `${normalizePath(cwd)}\\.playwright-run.lock`;
}

/**
 * 命令行展示形式（argv 里可执行文件在第一位）。
 *
 * @param {string[]} argv
 * @returns {string}
 */

/**
 * 命令行展示形式（argv 里可执行文件在第一位）。
 *
 * @param {string[]} argv
 * @returns {string}
 */
export function playwrightCommandText(argv) {
  return renderArgv(argv);
}

/**
 * 从 playwright 输出里抓通过/失败/跳过计数。
 *
 * 只认 line / list 报告器的措辞（` 3 passed `、` 1 failed `、` 2 skipped `、` 1 did not run `），
 * 不重新实现报告器。
 *
 * @param {string} text
 * @returns {{passed:number, failed:number, skipped:number, didNotRun:number, flaky:number}}
 */

/**
 * 从 playwright 输出里抓通过/失败/跳过计数。
 *
 * 只认 line / list 报告器的措辞（` 3 passed `、` 1 failed `、` 2 skipped `、` 1 did not run `），
 * 不重新实现报告器。
 *
 * @param {string} text
 * @returns {{passed:number, failed:number, skipped:number, didNotRun:number, flaky:number}}
 */
export function parsePlaywrightSummary(text) {
  const source = String(text ?? "");
  const count = (pattern) => {
    let total = 0;
    for (const match of source.matchAll(pattern)) {
      const value = Number.parseInt(match[1], 10);
      if (Number.isFinite(value)) total += value;
    }
    return total;
  };
  return {
    passed: count(/(\d+)\s+passed\b/g),
    failed: count(/(\d+)\s+failed\b/g),
    skipped: count(/(\d+)\s+skipped\b/g),
    didNotRun: count(/(\d+)\s+did not run\b/g),
    flaky: count(/(\d+)\s+flaky\b/g),
  };
}

/**
 * 从 line 报告器输出里抓失败用例标题与错误首行。
 *
 * @param {string} text
 * @returns {{title:string, error:string}[]}
 */

/**
 * 从 line 报告器输出里抓失败用例标题与错误首行。
 *
 * @param {string} text
 * @returns {{title:string, error:string}[]}
 */
export function parsePlaywrightFailures(text) {
  /** @type {{title:string, error:string}[]} */
  const failures = [];
  const lines = String(text ?? "").split(/\r?\n/);
  for (let i = 0; i < lines.length; i += 1) {
    const head = /^\s*(?:\u2718|\u00d7|\u25cf)\s+(\d+)\s+\[[^\]]*\]\s*(.+?)\s*(?:\(\d+(?:\.\d+)?m?s\))?\s*$/.exec(lines[i]);
    if (head === null) continue;
    let error = "";
    for (let j = i + 1; j < lines.length && j <= i + 12; j += 1) {
      const candidate = lines[j].trim();
      if (candidate !== "") {
        error = candidate;
        break;
      }
    }
    failures.push({ title: head[2].trim(), error: tailText(error, 300) });
    if (failures.length >= 50) break;
  }
  return failures;
}

/**
 * 从 `playwright test --list` 的输出里抓用例总数与文件数。
 *
 * @param {string} text
 * @returns {{tests:number, files:number, titles:string[]}|undefined}
 */

/**
 * 从 `playwright test --list` 的输出里抓用例总数与文件数。
 *
 * @param {string} text
 * @returns {{tests:number, files:number, titles:string[]}|undefined}
 */
export function parsePlaywrightListing(text) {
  const source = String(text ?? "");
  const total = /Total:\s*(\d+)\s+tests?\s+in\s+(\d+)\s+files?/i.exec(source);
  if (total === null) {
    const only = /Total:\s*(\d+)\s+tests?/i.exec(source);
    if (only === null) return undefined;
    return { tests: Number.parseInt(only[1], 10), files: 0, titles: [] };
  }
  /** @type {string[]} */
  const titles = [];
  for (const raw of source.split(/\r?\n/)) {
    const line = raw.trim();
    if (/^(Listing tests:|Total:)/i.test(line)) continue;
    if (!/›/.test(line)) continue;
    titles.push(line);
    if (titles.length >= PLAYWRIGHT_MAX_LISTED) break;
  }
  return { tests: Number.parseInt(total[1], 10), files: Number.parseInt(total[2], 10), titles };
}

/**
 * 退出码 + 输出 -> 结论码。先看插件自己判定的终态，再看输出里的失败计数，最后看退出码。
 *
 * @param {{exitCode:number|null, summary:{failed:number, passed:number}, failures:{title:string, error:string}[], aborted?:boolean, timedOut?:boolean}} spec
 * @returns {string}
 */

/**
 * 退出码 + 输出 -> 结论码。先看插件自己判定的终态，再看输出里的失败计数，最后看退出码。
 *
 * @param {{exitCode:number|null, summary:{failed:number, passed:number}, failures:{title:string, error:string}[], aborted?:boolean, timedOut?:boolean}} spec
 * @returns {string}
 */
export function interpretPlaywrightExit({ exitCode, summary, failures, aborted, timedOut }) {
  if (timedOut === true) return "timeout";
  if (aborted === true) return "cancelled";
  if (exitCode === null || exitCode === undefined) return "exit-nonzero";
  if (exitCode === 0) return "ok";
  if (summary.failed > 0 || failures.length > 0) return "tests-failed";
  return "exit-nonzero";
}
