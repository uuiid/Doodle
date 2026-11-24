// dsh-kitsu-supplement —— 五个工具的注册
//
// 每个 register*Tool 只做 ctx.tools.register(defineTool(...))；执行体在 runtime/ 下。
// 共享状态由 apply() 组进 deps：ctx、解析后的配置、实例表、状态文件读写与探测函数。

import { defineTool } from "@deepseek-ai/dsh-tools";

import { MEMORY_DB, MODES, PORT_RANGE_TEXT, normalizePath, parsePort, renderArgv, sanitizeForJson, trimOrEmpty } from "./pure/common.js";
import { slotSummary } from "./pure/instances.js";
import { errorText } from "./pure/common.js";
import { validateSlotRequest } from "./pure/instances.js";
import { buildArgv, resolveSpec, startFailure, startWarnings } from "./pure/start.js";
import { EXE_STOP_NOTICE, playwrightCwdFailure } from "dsh-doodle-workspace";
import { resolveSqlTarget, runSql, validateSqlArgs } from "./pure/sql.js";
import {
  PLAYWRIGHT_SCHEMA,
  SQL_SCHEMA,
  START_SCHEMA,
  STATUS_SCHEMA,
  STOP_SCHEMA,
  emptyPlaywrightSummary,
  renderPlaywright,
  renderSql,
  renderStart,
  renderStatus,
  renderStop,
} from "./schemas.js";
import { describeServer } from "./runtime/describe.js";
import { allocatePlaywrightRunId, allocateServerId, getPlaywrightRunning, setPlaywrightRunning } from "./runtime/registry.js";
import { playwrightFailure, runPlaywrightTool } from "./runtime/playwright.js";
import { probePort, sleep } from "./runtime/process.js";
import { startServer, stopAdopted, stopRecord } from "./runtime/server.js";

let createUserMessageLoader = null;

/**
 * exe 解析失败时的收尾：返回失败结果、结束本轮、并把「停下等用户检查」送到对话里。
 *
 * 只在当前工具结束模型这一轮（concludeTurn），不会自动再发下一轮的请求；
 * 那条 user 消息随结果一起进历史，用户下一条消息时模型会先看到它。
 *
 * @param {any} exec
 * @param {any} spec
 * @param {string} detail
 * @returns {any}
 */
function stopStart(exec, spec, detail) {
  const message = `${detail} ${EXE_STOP_NOTICE}`;
  try {
    exec.concludeTurn();
  } catch {
  }
  const content = [{ type: "text", text: `<kitsu_exe_unresolved>\n${EXE_STOP_NOTICE}\n原因：${detail}\n</kitsu_exe_unresolved>` }];
  const deliver = (createUserMessage) => {
    try {
      exec.deferContext(createUserMessage({
        content,
        source: { kind: "kitsu-supplement", form: "notice", summary: "kitsu exe 解析失败，停止操作" },
      }));
    } catch {
    }
  };
  if (typeof createUserMessageLoader === "function") {
    loadCreateUserMessage().then(deliver, () => {});
  }
  return startFailure(spec, "invalid-args", message, {
    diagnostics: [{ severity: "error", code: "exe/unresolved", message }],
  });
}

/**
 * 延迟解析出 `createUserMessage`：它来自可选的 @deepseek-ai/dsh-llm peer 依赖，
 * 宿主没提供时 deferContext 只是不发，不影响本次失败结果。
 *
 * @returns {Promise<any>}
 */
function loadCreateUserMessage() {
  if (createUserMessageLoader === null) {
    createUserMessageLoader = () => import("@deepseek-ai/dsh-llm")
      .then((mod) => (typeof mod.createUserMessage === "function" ? mod.createUserMessage : null))
      .catch(() => null);
  }
  return createUserMessageLoader();
}

/** @param {any} deps */
export function registerStartTool({ ctx, resolved, registry, stateWarnings, statePath, saveInstances, writeConfigFile, exists, workspaceRootOf, branchOf }) {
  ctx.tools.register(
    defineTool({
      name: "kitsu_server_start",
      description:
        "Start the kitsu supplement HTTP server for local testing and return a ready http://127.0.0.1:<port> " +
        "baseUrl. The executable is resolved from THIS SESSION'S WORKSPACE, never from a configured path: " +
        "`<parent of workspace>\\build\\<git branch>\\Ninja_debug\\bin\\<exeName>`, the same expression CMakePresets.json " +
        "uses for binaryDir. The default `exeName` is doodle_kitsu_supplement_test.exe, the primary way to run the " +
        "server: it serves the same routes as the production exe but replaces the Maya / UE child processes with stubs " +
        "(test_exe_runner), so no Maya, Unreal or plugin install is needed on this machine; use it for every test. " +
        "Pass `exeName: \"doodle_kitsu_supplement.exe\"` only when the real mayabatch / UnrealEditor-Cmd must actually run. " +
        "`mode: \"production\"` (default) lets the server read its registry config and listen on config.port; " +
        "`mode: \"local\"` adds `--local`, which " +
        "turns off the registry-only paths (in-memory database unless `dbPath` is given). " +
        `\`port\` is REQUIRED and must be an integer in ${PORT_RANGE_TEXT}: the server is started with that exact ` +
        "`--port`, and readiness is decided by polling " +
        "`http://127.0.0.1:<port><readyPath>` until any HTTP status comes back (up to 100 s). The generated config JSON carries dbPath (db_path), frontEndPath " +
        "(kitsu_front_end_path), publicUrlPort (public_url_port) and root; the working directory is `cwd` (default: the " +
        "exe's own directory, which holds its DLLs). When the port is already taken the call fails with code " +
        "`port-in-use` and reports the owning PIDs; pass `killExisting: true` only when those processes should be " +
        "force-killed first. The server keeps running after the call returns — it is tracked for `kitsu_server_status` " +
        "and `kitsu_server_stop`, and terminated when the plugin unloads. Each mode (local / production) allows " +
        "at most ONE running instance: starting a second one in the same mode fails with `slot-in-use` unless " +
        "`replaceExisting: true` is passed, which stops the previous instance first (reported in `replaced`). " +
        "The server writes its own logs to `<root>\\log` (root = the `root` parameter, else the configured " +
        "defaultRoot, else the server's built-in `%TEMP%\\Doodle`); the default logger goes to `doodle_lib.txt`, " +
        "plus `doodle_long_task.txt`, `socket_io.txt`, `http.txt` and `doodle_main_error.txt`, each a rotating " +
        "file capped at 1 MB. When a file from an earlier run already exists the server appends a UUID to the name " +
        "instead of overwriting, so a crashed run's log stays readable; read them with the local file tools after " +
        "the process is gone, because this tool reports only the last 4000 characters of a failed start's output.",
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
            "Absolute project-data root, passed as `--root`. It is the server's `p_root`: besides the data/cache root " +
            "it is where the server writes its log directory (`<root>\\log`), see the log files listed on this tool. " +
            "Without it the server falls back to `%TEMP%\\Doodle`, so pass a stable directory when the crash logs of " +
            "this run must be found later.",
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
        exeName: {
          type: "string",
          description:
            "File name of the executable, NOT a path. Default = doodle_kitsu_supplement_test.exe (stubbed Maya/UE). " +
            "`\"doodle_kitsu_supplement.exe\"` = production exe (real external processes). The full path comes from the " +
            "session workspace and its git branch; anything containing a path separator or a drive letter is rejected.",
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
        replaceExisting: {
          type: "boolean",
          description:
            "Every mode allows at most ONE running instance. When the target mode is already taken the call fails with " +
            "`slot-in-use`; pass `replaceExisting: true` to stop that instance first and start the new one in its place. " +
            "Default false.",
        },
      },
      output: { schema: START_SCHEMA, render: renderStart },
      isConcurrencySafe() {
        return false;
      },
      async execute(args, exec) {
        const requestedMode = trimOrEmpty(args.mode) || (args.local === true ? "local" : "") || resolved.defaultMode;
        const workspaceRoot = workspaceRootOf(exec);
        const branch = await branchOf(workspaceRoot, exec.signal);
        const plan = await resolveSpec({
          config: resolved,
          args: { ...args, mode: requestedMode },
          exists,
          workspaceRoot,
          branch,
        });
        if (plan.problems.length > 0) {
          return stopStart(exec, plan.spec, plan.problems.join(" "));
        }
        const spec = plan.spec;
        const replaced = [];
        const slot = validateSlotRequest({ records: registry, mode: spec.mode, replaceExisting: args.replaceExisting === true });
        if (slot.kind === "occupied") {
          return startFailure(spec, "slot-in-use", slot.problem, {
            serverId: slot.holderId,
            replaced,
            diagnostics: [{ severity: "error", code: "slot/in-use", message: slot.problem }],
            warnings: [...stateWarnings],
          });
        }
        if (slot.kind === "replace" && slot.holderId !== undefined) {
          const holder = registry.get(slot.holderId);
          if (holder !== undefined) {
            const stopped =
              holder.handle === null || holder.handle === undefined
                ? await stopAdopted(ctx, holder)
                : await stopRecord(ctx, holder, resolved.stopTimeoutMs);
            replaced.push({ serverId: slot.holderId, port: holder.port, ok: stopped.ok, reason: stopped.reason });
            if (!stopped.ok) {
              const detail = `替换 ${spec.mode} 模式的旧实例 ${slot.holderId} 失败：${stopped.reason}`;
              return startFailure(spec, "kill-failed", detail, {
                serverId: slot.holderId,
                replaced,
                diagnostics: [{ severity: "error", code: "kill/failed", message: detail }],
                warnings: [...stateWarnings],
              });
            }
            registry.delete(slot.holderId);
            await saveInstances();
          }
        }
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
          const result = await startServer(ctx, {
            serverId,
            exePath: spec.exePath,
            argv,
            cwd: spec.cwd,
            command: `${spec.exePath} ${renderArgv(argv)}`,
            mode: spec.mode,
            port: spec.port,
            exeKind: spec.exeKind,
            configPath,
            dbPath: spec.dbPath ?? MEMORY_DB,
            readyPath: spec.readyPath,
            killExisting: args.killExisting === true,
            readyTimeoutMs: resolved.readyTimeoutMs,
            stopTimeoutMs: resolved.stopTimeoutMs,
            maxOutputBytes: resolved.maxOutputBytes,
            warnings: [...startWarnings(spec), ...stateWarnings],
            signal: exec.signal,
          });
          if (result?.ok === true) await saveInstances();
          return { ...result, replaced, statePath };
        } finally {
          const fs = await import("node:fs/promises");
          await fs.unlink(configPath).catch(() => {});
        }
      },
    }),
  );
}

/** @param {any} deps */
export function registerStatusTool({ ctx, resolved, registry, stateWarnings, statePath, saveInstances, writeConfigFile, exists }) {
  ctx.tools.register(
    defineTool({
      name: "kitsu_server_status",
      description:
        "List the doodle_kitsu_supplement instances this plugin started (id, mode, port, running/exited, baseUrl, " +
        "config path) and, for running instances, who is LISTENING on their ports (netstat). Pass `port` to also check " +
        "an arbitrary port. The port is always the explicitly requested one, never derived from the process. " +
        "An instance whose process died is reported as `running: false` with its `exitCode` and stays in the list " +
        "until `kitsu_server_stop` drops it, while its slot is already free. This tool does not return process " +
        "output: to see why a run failed, read the server's own log directory `<root>\\log` " +
        "(`doodle_lib.txt` first, plus `doodle_long_task.txt` / `socket_io.txt` / `http.txt` / " +
        "`doodle_main_error.txt`; names carry a UUID suffix when the plain name already existed).",
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
          if (record.exited !== true && typeof record.port === "number" && !ports.includes(record.port)) ports.push(record.port);
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
        const summary = slotSummary(registry);
        for (const duplicate of summary.duplicates) {
          diagnostics.push({
            severity: "error",
            code: "slot/duplicate",
            message:
              `${duplicate.mode} 模式有 ${duplicate.serverIds.length} 个实例同时在跑（${duplicate.serverIds.join(", ")}），` +
              "超过每模式 1 个的上限：先 kitsu_server_stop 停掉多余的，这里只报告、不自动杀。",
          });
        }
        for (const warning of stateWarnings) {
          diagnostics.push({ severity: "warning", code: "state/read-failed", message: warning });
        }
        const running = servers.filter((server) => server.running === true).length;
        const describeSlot = (mode) => {
          const slot = summary.slots[mode];
          if (summary.duplicates.some((item) => item.mode === mode)) return `${mode} 重复占用`;
          return slot === null || slot === undefined ? `${mode} 空闲` : `${mode} ${slot.serverId}(:${slot.port ?? "?"})`;
        };
        const availableText = summary.available.length === 0 ? "没有空位" : `还能起 ${summary.available.join(" / ")}`;
        const reason =
          servers.length === 0
            ? `本插件没有记录任何实例。每模式最多 1 个，${availableText}。`
            : `记录 ${servers.length} 个实例，${running} 个在运行：${describeSlot("local")}，${describeSlot("production")}，${availableText}。`;
        return {
          ok: summary.duplicates.length === 0,
          reason,
          slots: summary.slots,
          usage: summary.usage,
          available: summary.available,
          statePath,
          servers,
          portOwners,
          diagnostics,
        };
      },
    }),
  );
}

/** @param {any} deps */
export function registerStopTool({ ctx, resolved, registry, stateWarnings, statePath, saveInstances, writeConfigFile, exists }) {
  ctx.tools.register(
    defineTool({
      name: "kitsu_server_stop",
      description:
        "Stop doodle_kitsu_supplement instances started by this plugin: terminate the process tree, wait for it to " +
        "settle, and drop it from the tracked list. With neither `serverId` nor `port`, every tracked instance is " +
        "stopped. Processes this plugin did not start are never killed. This call is what removes an already-crashed " +
        "instance (`running: false`) from the list, and it succeeds with reason `进程已退出` for one. It leaves the " +
        "server's `<root>\\log` directory intact for reading afterwards, and per AGENTS.md it must run before " +
        "rebuilding `doodle_lib`, because the live process holds `bin\\doodle_lib.dll`.",
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
          const result = record.handle === null || record.handle === undefined ? await stopAdopted(ctx, record) : await stopRecord(ctx, record, resolved.stopTimeoutMs);
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
        await saveInstances();
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

/** @param {any} deps */
export function registerSqlTool({ ctx, resolved, registry, stateWarnings, statePath, saveInstances, writeConfigFile, exists }) {
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

/** @param {any} deps */
export function registerPlaywrightTool({ ctx, resolved, registry, stateWarnings, statePath, saveInstances, writeConfigFile, exists, playwrightCwdOf }) {
  ctx.tools.register(
    defineTool({
      name: "kitsu_playwright_test",
      description:
        "Run the Playwright test suite under test_js and PASS ITS ARGUMENTS THROUGH VERBATIM: `args` is everything " +
        "after the `playwright` executable (`[\"test\"]`, `[\"test\", \"face_capture.js\", \"--reporter=line\"]`, " +
        "`[\"test\", \"--list\"]`, `[\"test\", \"-g\", \"创建设备\"]`, `[\"show-report\"]`). The child is started as " +
        "`node <cwd>/node_modules/@playwright/test/cli.js <args...>` with cwd = the test_js directory of THIS " +
        "SESSION'S WORKSPACE (the session's workspace root first, then the plugin's fallback directory), so no " +
        "run-playwright.mjs shim is needed. When no such directory holds node_modules/@playwright/test the call " +
        "fails and tells you to STOP and leave the check to the user. `baseUrl` only decides " +
        "whether DOODLE_BASE_URL is set for the child: omit it to inherit the parent environment " +
        "(playwright.config.js then falls back to http://localhost:50025), pass a URL to point the suite at a " +
        "specific server (the baseUrl returned by kitsu_server_start). " +
        "The tool is NEVER concurrent: a second call while one is running is rejected at once with code `busy` " +
        "(nothing is queued). A run is killed once `timeoutMs` expires (terminate, then the host ends the process " +
        "tree). The result carries the pass/fail/skip summary parsed from the output, per-failure titles with their " +
        "first error line, --list totals when present, the report/output directories and the output tail.",
      parameters: {
        args: {
          type: "array",
          items: { type: "string" },
          required: true,
          description:
            "Everything after `playwright`, forwarded unchanged ([\"test\"], [\"test\", \"--list\"], " +
            "[\"show-report\"], ...). Not interpreted by the tool: sub-commands, -g, --reporter, --project and -- " +
            "reach playwright as given.",
        },
        cwd: {
          type: "string",
          description:
            "Directory holding node_modules/@playwright/test. Defaults to the test_js directory found from the " +
            "session workspace root (the plugin's fallback directory is only used when the workspace has no " +
            "installed dependencies). Must exist and be a directory.",
        },
        baseUrl: {
          type: "string",
          description:
            "http(s)://host[:port] to set DOODLE_BASE_URL for the child (use the baseUrl returned by " +
            "kitsu_server_start). Omitted: the plugin config default (`playwrightDefaultBaseUrl`), else the inherited " +
            "DOODLE_BASE_URL, else nothing (playwright.config.js falls back to localhost:50025). `\"auto\"` is not " +
            "supported: the tool never looks up running instances itself.",
        },
        env: {
          type: "object",
          additionalProperties: true,
          description:
            "Extra environment variables for the child (e.g. FACE_CAPTURE_TRANSCODE_E2E: \"1\"). An explicit key " +
            "wins over the plugin's own injection (DOODLE_BASE_URL / PLAYWRIGHT_HTML_OPEN).",
        },
        isolateOutput: {
          type: "boolean",
          description:
            "When true, point PLAYWRIGHT_HTML_REPORT and PLAYWRIGHT_OUTPUT_DIR at " +
            "%TEMP%\\dsh-kitsu-supplement\\playwright\\<runId> so the run does not overwrite " +
            "test_js\\playwright-report and test_js\\test-results. Default false: the project config decides.",
        },
        timeoutMs: {
          type: "number",
          description:
            "Kill the run after this many milliseconds (default: configured playwrightTimeoutMs, 900000). The " +
            "per-test timeout stays the one in playwright.config.js (60000).",
        },
        nodePath: {
          type: "string",
          description:
            "Node executable used to run playwright (default: `node` from PATH, or configured playwrightNodePath).",
        },
        cliPath: {
          type: "string",
          description:
            "Path to @playwright/test/cli.js (default: <cwd>/node_modules/@playwright/test/cli.js, or the " +
            "configured playwrightCliPath).",
        },
      },
      output: { schema: PLAYWRIGHT_SCHEMA, render: renderPlaywright },
      isConcurrencySafe() {
        return false;
      },
      async execute(args, exec) {
        const runId = allocatePlaywrightRunId();
        const resolvedCwd = trimOrEmpty(args.cwd) === ""
          ? await playwrightCwdOf(exec, exec.signal)
          : { cwd: trimOrEmpty(args.cwd), workspaceRoot: "", lookedAt: [] };
        const inFlight = getPlaywrightRunning();
        if (inFlight !== null) {
          const running = inFlight;
          const elapsed = Date.now() - running.startedAt;
          /** @type {any} */
          const busyBase = {
            runId: running.runId,
            command: "",
            args: running.args,
            cwd: running.cwd,
            cliPath: "",
            baseUrl: "",
            baseUrlSource: "none",
            durationMs: elapsed,
            summary: emptyPlaywrightSummary(),
            failures: [],
            diagnostics: [],
            warnings: [],
          };
          const busyReason = `kitsu_playwright_test 已经在跑（${running.runId}，${elapsed} ms）：本工具不可并发，等它结束后再调用。`;
          const busyItem = "同一时刻只允许一个 playwright 运行，避免 test-results 与报告目录互相覆盖。";
          return playwrightFailure(busyBase, "busy", busyReason, [{ severity: "error", code: "run/busy", message: busyItem }]);
        }
        setPlaywrightRunning({
          runId,
          startedAt: Date.now(),
          args: Array.isArray(args.args) ? args.args.map((item) => String(item)) : [],
          cwd: resolvedCwd.cwd,
        });
        try {
          return await runPlaywrightTool(
            ctx,
            resolved,
            { ...args, cwd: resolvedCwd.cwd === "" ? undefined : resolvedCwd.cwd },
            exec,
            resolvedCwd.cwd === "" ? playwrightCwdFailure(resolvedCwd) : undefined,
          );
        } finally {
          setPlaywrightRunning(null);
        }      },
    }),
  );
}
