// dsh-kitsu-supplement —— kitsu_playwright_test 的执行体
//
// 原样转发 playwright CLI 参数；不走 test_js/run-playwright.mjs 的 fork->spawn 降级。

import { DEFAULT_MAX_TAIL_CHARS, errorText, isPlainObject, normalizePath, positiveOrDefault, sanitizeForJson, tailText, trimOrEmpty } from "../pure/common.js";
import {
  PLAYWRIGHT_BASE_URL_ENV,
  PLAYWRIGHT_HTML_OPEN_ENV,
  PLAYWRIGHT_HTML_REPORT_ENV,
  PLAYWRIGHT_OUTPUT_DIR_ENV,
  defaultPlaywrightCli,
  interpretPlaywrightExit,
  lookupEnv,
  mergePlaywrightEnv,
  parseBaseUrlArg,
  parsePlaywrightFailures,
  parsePlaywrightListing,
  parsePlaywrightSummary,
  playwrightCommandText,
  playwrightLockPath,
  playwrightOutputDirs,
  validatePlaywrightArgs,
} from "../pure/playwright.js";

import { emptyPlaywrightSummary } from "../schemas.js";
import { allocatePlaywrightRunId, getPlaywrightRunning, setPlaywrightRunning } from "./registry.js";
import { readAll, sleep } from "./process.js";


/**
 * 写入「本机正在跑 playwright」的标记文件；写不进去不影响执行。
 *
 * @param {string} lockPath
 * @param {any} info
 * @returns {Promise<string>} 空串表示成功，否则是失败原因
 */
export async function writePlaywrightLock(lockPath, info) {
  const fs = await import("node:fs/promises");
  try {
    await fs.writeFile(lockPath, `${JSON.stringify(info, null, 2)}\n`, "utf8");
    return "";
  } catch (error) {
    return errorText(error);
  }
}

/**
 * @param {string} lockPath
 * @returns {Promise<void>}
 */
export async function removePlaywrightLock(lockPath) {
  const fs = await import("node:fs/promises");
  try {
    await fs.unlink(lockPath);
  } catch {
    // 锁文件可能已被删除；残留由下一次运行覆盖
  }
}

/**
 * 写完结果前统一的「失败」形状：键集合与成功时一致。
 *
 * @param {any} base
 * @param {string} code
 * @param {string} reason
 * @param {any[]} diagnostics
 * @returns {any}
 */
export function playwrightFailure(base, code, reason, diagnostics) {
  return sanitizeForJson({
    ...base,
    ok: false,
    code,
    reason,
    diagnostics,
  });
}

/**
 * @param {any} handle
 * @param {number} timeoutMs
 * @returns {Promise<boolean>} true 表示在预算内退出
 */
export async function waitPlaywrightExit(handle, timeoutMs) {
  if (typeof handle.waitForExit !== "function") return true;
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), timeoutMs);
  try {
    const settled = await handle.waitForExit(controller.signal);
    return settled !== false;
  } catch {
    return false;
  } finally {
    clearTimeout(timer);
  }
}

/**
 * kitsu_playwright_test 的实现：原样转发 playwright CLI 参数。
 *
 * 不走 test_js/run-playwright.mjs 的 fork->spawn 降级——插件用的是宿主子进程通道，
 * 那个脚本服务于 pwsh 沙箱，与本插件无关。
 *
 * @param {any} ctx
 * @param {any} config
 * @param {any} args
 * @param {any} exec
 * @returns {Promise<any>}
 */
export async function runPlaywrightTool(ctx, config, args, exec) {
  const runId = allocatePlaywrightRunId();
  const startedAt = Date.now();
  const fs = await import("node:fs/promises");
  const os = await import("node:os");
  const path = await import("node:path");

  const passthrough = Array.isArray(args.args) ? args.args.map((item) => String(item)) : [];
  const cwd = normalizePath(trimOrEmpty(args.cwd) || config.playwrightCwd);
  const cliPath = trimOrEmpty(args.cliPath) || config.playwrightCliPath || defaultPlaywrightCli(cwd);
  const nodePath = trimOrEmpty(args.nodePath) || config.playwrightNodePath || "node";
  const isolate = args.isolateOutput === true;
  const timeoutMs = positiveOrDefault(args.timeoutMs, config.playwrightTimeoutMs);
  const killTimeoutMs = config.playwrightKillTimeoutMs;
  const reportDirs = playwrightOutputDirs({
    cwd,
    runId,
    isolate,
    tmpRoot: path.join(os.tmpdir(), "dsh-kitsu-supplement", "playwright"),
  });
  const command = playwrightCommandText([nodePath, cliPath, ...passthrough]);

  /** @type {string[]} */
  const problems = [...validatePlaywrightArgs(args.args)];
  const baseArg = parseBaseUrlArg(args.baseUrl);
  if (baseArg.kind === "invalid") problems.push(baseArg.problem);
  if (args.env !== undefined && !isPlainObject(args.env)) problems.push("env 必须是字符串键值对对象。");

  if (problems.length === 0) {
    /** @type {any} */
    let stat;
    try {
      stat = await fs.stat(cwd);
    } catch (error) {
      problems.push(`cwd 不存在或不可访问：${cwd}（${errorText(error)}）`);
    }
    if (stat !== undefined && stat.isDirectory() !== true) problems.push(`cwd 不是目录：${cwd}`);
  }

  /** @type {string} */
  let baseUrl = "";
  /** @type {string} */
  let baseUrlSource = "none";
  if (problems.length === 0) {
    if (baseArg.kind === "explicit") {
      baseUrl = baseArg.value;
      baseUrlSource = "param";
    } else if (config.playwrightDefaultBaseUrl !== "") {
      baseUrl = config.playwrightDefaultBaseUrl;
      baseUrlSource = "config";
    } else {
      const inherited = trimOrEmpty(lookupEnv(process.env, PLAYWRIGHT_BASE_URL_ENV));
      if (inherited !== "") {
        baseUrl = inherited.replace(/\/+$/, "");
        baseUrlSource = "env";
      }
    }
  }

  /** @type {Record<string, string>} */
  const injected = { [PLAYWRIGHT_HTML_OPEN_ENV]: "never" };
  if (baseUrl !== "") injected[PLAYWRIGHT_BASE_URL_ENV] = baseUrl;
  if (isolate) {
    injected[PLAYWRIGHT_HTML_REPORT_ENV] = reportDirs.reportDir;
    injected[PLAYWRIGHT_OUTPUT_DIR_ENV] = reportDirs.outputDir;
  }
  const env = mergePlaywrightEnv({
    baseEnv: process.env,
    injected,
    explicit: isPlainObject(args.env) ? args.env : undefined,
  });

  const base = {
    runId,
    command,
    args: passthrough,
    cwd,
    cliPath,
    baseUrl,
    baseUrlSource,
    durationMs: 0,
    summary: emptyPlaywrightSummary(),
    failures: [],
    reportDir: reportDirs.reportDir,
    outputDir: reportDirs.outputDir,
    diagnostics: [],
    warnings: [],
  };

  try {
    await fs.access(cliPath);
  } catch {
    return playwrightFailure(
      { ...base, durationMs: Date.now() - startedAt },
      "timeout-cli",
      `找不到 playwright 入口：${cliPath}。确认 cwd 下装好了 @playwright/test，或用 cliPath 参数 / playwrightCliPath 配置指向真实的 cli.js。`,
      [{ severity: "error", code: "cli/missing", message: `playwright 入口不存在：${cliPath}（cwd=${cwd}）` }],
    );
  }

  if (problems.length > 0) {
    return playwrightFailure(
      base,
      "invalid-args",
      problems.join(" "),
      problems.map((message) => ({ severity: "error", code: "args/invalid", message })),
    );
  }

  const lockPath = playwrightLockPath(cwd);
  if (isolate) await fs.mkdir(reportDirs.reportDir, { recursive: true }).catch(() => {});

  /** @type {any} */
  let handle;
  try {
    handle = ctx.subprocess.spawn({
      argv: [nodePath, cliPath, ...passthrough],
      cwd,
      env,
      stdio: {
        stdin: "ignore",
        stdout: { maxBytes: config.maxOutputBytes },
        stderr: { maxBytes: config.maxOutputBytes },
      },
      graceMs: killTimeoutMs,
      signal: exec.signal,
    });
  } catch (error) {
    return playwrightFailure(
      { ...base, durationMs: Date.now() - startedAt },
      "spawn-failed",
      `无法启动 ${nodePath}：${errorText(error)}`,
      [{ severity: "error", code: "spawn/failed", message: errorText(error) }],
    );
  }

  const lockWarning = await writePlaywrightLock(lockPath, {
    runId,
    argv: [nodePath, cliPath, ...passthrough],
    startedAt,
  });
  /** @type {string[]} */
  const warnings = [];
  if (lockWarning !== "") warnings.push(`标记文件 ${lockPath} 写入失败（不影响执行）：${lockWarning}`);

  /** @type {any} */
  let settled;
  let timedOut = false;
  let runError = "";
  try {
    settled = await Promise.race([
      handle.done.catch((error) => ({ exitCode: null, error: errorText(error) })),
      sleep(timeoutMs).then(() => ({ timedOut: true })),
    ]);
    if (settled !== null && settled !== undefined && settled.timedOut === true) {
      timedOut = true;
      try {
        handle.terminate();
      } catch {
        // 进程可能已经退出，下面等停稳是兜底
      }
      await waitPlaywrightExit(handle, killTimeoutMs).catch(() => false);
      settled = { exitCode: null };
    }
  } catch (error) {
    runError = errorText(error);
  } finally {
    await removePlaywrightLock(lockPath);
  }
  const aborted = exec.signal?.aborted === true;
  if (runError !== "") {
    return playwrightFailure(
      { ...base, durationMs: Date.now() - startedAt },
      aborted ? "cancelled" : "exit-nonzero",
      aborted ? "调用被中止。" : `等待 playwright 结束时出错：${runError}`,
      [{ severity: "error", code: "run/error", message: runError }],
    );
  }

  const stdout = readAll(handle.collected?.stdout);
  const stderr = readAll(handle.collected?.stderr);
  const combined = [stdout.text, stderr.text].filter((text) => text.length > 0).join("\n");
  const summary = parsePlaywrightSummary(combined);
  const failures = parsePlaywrightFailures(combined);
  const listed = parsePlaywrightListing(combined);
  const exitCode = typeof settled?.exitCode === "number" ? settled.exitCode : null;
  const code = interpretPlaywrightExit({ exitCode, summary, failures, aborted, timedOut });

  /** @type {any[]} */
  const diagnostics = [];
  if (timedOut) {
    diagnostics.push({
      severity: "error",
      code: "timeout/killed",
      message: `超过 ${timeoutMs} ms：已 terminate，${killTimeoutMs} ms 内未退出时由宿主结束进程树。`,
    });
  }
  if (aborted) diagnostics.push({ severity: "warning", code: "run/cancelled", message: "调用被中止，进程已终止。" });
  if (settled?.error !== undefined) diagnostics.push({ severity: "error", code: "run/error", message: String(settled.error) });
  if (exitCode !== null && exitCode !== 0) {
    diagnostics.push({ severity: "error", code: "run/exit", message: `playwright 退出码 ${exitCode}` });
  }

  const durationMs = Date.now() - startedAt;
  const counts = `passed=${summary.passed} failed=${summary.failed} skipped=${summary.skipped}`;
  let reason;
  if (code === "ok") reason = `playwright 通过：${counts}（${durationMs} ms）。`;
  else if (code === "tests-failed") reason = `playwright 有失败用例：${counts}。`;
  else if (code === "timeout") reason = `超过 ${timeoutMs} ms，已终止。`;
  else if (code === "cancelled") reason = "调用被中止。";
  else reason = `playwright 退出码 ${exitCode ?? "n/a"}，输出里没有用例计数：先看 outputTail 确认命令与配置。`;

  return sanitizeForJson({
    ...base,
    ok: code === "ok",
    code,
    reason,
    exitCode: exitCode ?? undefined,
    durationMs,
    summary,
    failures,
    listed,
    diagnostics,
    warnings,
    outputTail: tailText(combined, DEFAULT_MAX_TAIL_CHARS) || undefined,
    spillPath: stdout.spillPath ?? stderr.spillPath,
  });
}
