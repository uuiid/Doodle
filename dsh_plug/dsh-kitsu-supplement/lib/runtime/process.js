// dsh-kitsu-supplement —— 进程、端口与就绪探测
//
// 服务端用 node:child_process 启动（pid 只能从这里拿）；netstat/taskkill 走 ctx.subprocess 的纳管通道。
// 两者底层同是 child_process.spawn，能否起进程取决于调用方进程的完整性级别，与选哪个 API 无关。

import { NETSTAT_EXE, TASKKILL_EXE, errorText, parseNetstatOwners, readyUrlOf, tailText } from "../pure/common.js";

import { serversOnPort } from "./registry.js";

export function readAll(reader) {
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
export function sleep(ms) {
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
export async function netstatText(ctx, signal) {
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
export async function probePort(ctx, port, signal) {
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
export async function killPids(ctx, pids) {
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
export async function waitForReady({ port, readyPath, isDead, timeoutMs, signal }) {
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
export async function waitForExit(handle, timeoutMs) {
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
 * 起一个服务端进程并拿回 pid。
 *
 * 走 node:child_process 而不是 ctx.subprocess：后者的 handle 不暴露 pid，
 * 而 pid 是「重启 DSH 后按 pid 接管」与状态展示的唯一依据。
 * 本地 provider 内部同样调用 child_process.spawn，二者权限要求一致。
 *
 * @param {{exePath:string, argv:string[], cwd:string, maxOutputBytes:number, signal?:AbortSignal}} spec
 * @returns {Promise<any>}
 */
export async function spawnServer(spec) {
  const { spawn } = await import("node:child_process");
  const child = spawn(spec.exePath, spec.argv, {
    cwd: spec.cwd,
    stdio: ["ignore", "pipe", "pipe"],
    windowsHide: true,
    signal: spec.signal,
  });

  /** @type {{stdout?:{text:string,spillPath?:string}, stderr?:{text:string,spillPath?:string}}} */
  const collected = { stdout: { text: "" }, stderr: { text: "" } };
  const cap = spec.maxOutputBytes;
  const attach = (stream, target) => {
    stream.setEncoding("utf8");
    stream.on("data", (chunk) => {
      target.text += chunk;
      if (target.text.length > cap) target.text = target.text.slice(target.text.length - cap);
    });
  };
  if (child.stdout) attach(child.stdout, collected.stdout);
  if (child.stderr) attach(child.stderr, collected.stderr);

  const done = new Promise((resolve, reject) => {
    child.once("error", (error) => reject(error));
    child.once("close", (code, signal) => resolve({ exitCode: typeof code === "number" ? code : null, signal: signal ?? null }));
  });
  done.catch(() => {});

  const reader = (target) => ({
    readFrom(fromByte) {
      if (fromByte >= target.text.length) return undefined;
      return { text: target.text.slice(fromByte), nextOffset: target.text.length, lossy: false };
    },
  });

  return {
    pid: child.pid,
    done,
    collected: {
      stdout: reader(collected.stdout),
      stderr: reader(collected.stderr),
    },
    terminate() {
      if (child.exitCode !== null || child.signalCode !== null) return;
      try {
        child.kill("SIGTERM");
      } catch {}
    },
    async waitForExit(signal) {
      if (child.exitCode !== null || child.signalCode !== null) return true;
      return new Promise((resolve) => {
        const finish = (value) => {
          clearTimeout(timer);
          signal?.removeEventListener("abort", onAbort);
          resolve(value);
        };
        const onAbort = () => finish(false);
        if (signal?.aborted) {
          finish(false);
          return;
        }
        signal?.addEventListener("abort", onAbort, { once: true });
        const timer = setTimeout(() => finish(false), 5 * 60_000);
        child.once("close", () => finish(true));
      });
    },
  };
}
