// dsh-kitsu-supplement —— kitsu_server_start / kitsu_server_stop 的执行体
//
// 起服务、等就绪、登记实例；停服时终止进程树并等停稳。

import { baseUrlOf, errorText, sanitizeForJson, tailText } from "../pure/common.js";
import { startFailure } from "../pure/start.js";

import { registry } from "./registry.js";
import { findPortLine } from "../pure/sql.js";
import { killPids, probePort, readAll, spawnServer, waitForExit, waitForReady } from "./process.js";

/**
 * 终止实例并等停稳。
 *
 * @param {any} record
 * @param {number} timeoutMs
 * @returns {Promise<{ok:boolean, reason:string, exitCode?:number|null, waitedMs:number}>}
 */
export async function stopRecord(ctx, record, timeoutMs) {
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
  if (!exited.ok) {
    if (typeof record.pid === "number") {
      const killed = await killPids(ctx, [record.pid]);
      if (killed.killed.includes(record.pid)) {
        await waitForExit(record.handle, timeoutMs);
        return { ok: true, reason: "已按 pid 结束进程树", exitCode: record.exitCode ?? null, waitedMs: Date.now() - started };
      }
      const reason = killed.failed.length > 0 ? killed.failed.join("；") : `PID ${record.pid} 结束失败`;
      return { ok: false, reason, waitedMs: Date.now() - started };
    }
    return { ok: false, reason: exited.reason, waitedMs: Date.now() - started };
  }
  return { ok: true, reason: "已终止", exitCode: record.exitCode ?? null, waitedMs: Date.now() - started };
}

/**
 * 停掉从状态文件接管的实例：本进程没有句柄，只能按 pid 结束进程树。
 *
 * @param {any} ctx
 * @param {any} record
 * @returns {Promise<{ok:boolean, reason:string, exitCode?:number|null, waitedMs:number}>}
 */
export async function stopAdopted(ctx, record) {
  const started = Date.now();
  if (typeof record?.pid !== "number") {
    return { ok: false, reason: "记录里没有 pid，无法结束进程：请手工处理后重试", waitedMs: 0 };
  }
  const killed = await killPids(ctx, [record.pid]);
  if (killed.killed.includes(record.pid)) {
    return { ok: true, reason: "已按 pid 结束进程树", waitedMs: Date.now() - started };
  }
  const reason = killed.failed.length > 0 ? killed.failed.join("；") : `PID ${record.pid} 结束失败`;
  return { ok: false, reason, waitedMs: Date.now() - started };
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
export async function startServer(ctx, spec) {
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
      const result = await stopRecord(ctx, record, spec.stopTimeoutMs);
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
    handle = await spawnServer(spec);
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
    pid: handle.pid,
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
    await stopRecord(ctx, record, spec.stopTimeoutMs);
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
        pid: record.pid,
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
    pid: record.pid,
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
