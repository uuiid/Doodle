/**
 * dsh-kitsu-supplement —— 拉起 doodle_kitsu_supplement.exe HTTP 服务（宿主端）
 *
 * 五个工具：
 *   kitsu_server_start  按 production / local 模式起服务，报告真实监听端口与 baseUrl
 *   kitsu_server_status 看本插件记录的实例，以及端口被谁占着
 *   kitsu_server_stop   停本插件拉起的实例
 *   kitsu_server_sql    对磁盘库执行 SQL（不停服）
 *   kitsu_playwright_test 原样转发 test_js 的 playwright 参数
 *
 * 布局：
 *   lib/index.js          入口：name / inject / Config 的再导出 + apply()
 *   lib/pure/*.js         纯逻辑（零 import、零 ctx、零宿主 I/O），test/pure.mjs 直接求值
 *   lib/schemas.js        Config 与五个工具的 output schema、render
 *   lib/runtime/*.js      ctx 相关：进程探测、起停服、playwright 执行、共享状态
 *   lib/tools.js          五个 defineTool 的注册
 *
 * 每条设计都对应服务端已核实的行为（行号写在对应用户的注释里）：
 *
 * 1) 端口必须显式给，且必须落在 50030-50330 之内（端口段由本机测试环境约定）。
 *    local 模式不给 --port 时服务端会 listen(0)（随机端口），
 *    而 local 分支同样不防重复实例（kitsu_supplement.cpp:219 的 stop_previous_instance 已注释），
 *    随机端口意味着每次重启地址都变，测试用例没法引用。所以缺 port 或越界都直接报错。
 * 2) 服务端由 node:child_process 启动，pid 取自 child.pid 并随实例落盘。
 *    不用 ctx.subprocess.spawn 是因为它的 handle 不暴露 pid（SubprocessHandle / SubprocessOutcome
 *    都没有 pid 字段），而 pid 是状态展示与「DSH 重启后按 pid 接管」的唯一依据；
 *    本地 provider 内部同样调用 child_process.spawn，两者权限要求一致。
 *    不要改成按端口用 netstat 反查 pid：实测 netstat 看不到这类服务进程的 LISTENING 行。
 *    监听端口恒为入参 port，不读服务端的共享内存段（`doodle_http_listener_port_<pid>`）。
 *    就绪判定不依赖 pid：直接对 `http://127.0.0.1:<port><readyPath>` 轮询，任何 HTTP 状态码都算
 *    就绪，最多等 READY_TIMEOUT_MS。
 * 3) 固定端口先探占用。默认失败并给出占用 PID，只有显式 killExisting:true 才强杀。
 * 4) --config 由插件生成。服务端 config 解析失败会 return true 且**不启动 HTTP**，
 *    进程可能一闪而过，所以 JSON 只由插件写，模型的额外字段并进 extraConfig。
 *    带值开关一律写成 `--key=value` 单个 argv 项（与 .vscode/launch.json 的用法一致）：
 *    argh 把 `--key=value` 记进 params_，`arg_({"key"})` 先查 params_ 再查 flags_，能取到值；
 *    而 `--key value` 里只有 `--key` 是选项、value 掉进位置参数，arg_ 得到空串——
 *    服务端 `--port` 空串会 lexical_cast 抛异常（监听之前就退出），`--root` 空串会把 `p_root` 置成空路径。
 * 5) cwd 默认取 exe 所在目录（exe 依赖同目录的 dll）；运行路径也可由参数/配置显式传入。
 *    `--root` 同时是服务端 `core_set::p_root`，日志目录 `<root>\log` 由它决定（app_base 构造函数里先设
 *    p_root 再建 logger），所以 root 不只是数据目录，也决定了崩溃日志落在哪里。
 * 6) 不用服务端的 POST /api/doodle/stop-server：它要 admin JWT，且连接数不为 0 时拒绝退出
 *    （base_data.cpp:343-366），测试机上的连接数恰恰不为 0。这里直接终止进程树并等停稳。
 *
 * 本文件按 DSH 插件惯例不使用顶层 node: 静态导入；内置模块在用到的位置动态 import。
 */

import {
  DEFAULT_GUIDANCE_ORDER,
  DEFAULT_MAX_OUTPUT_BYTES,
  DEFAULT_MODE,
  DEFAULT_READY_PATH,
  DEFAULT_STOP_TIMEOUT_MS,
  GUIDANCE_SECTION,
  MAX_OUTPUT_BYTES,
  MIN_OUTPUT_BYTES,
  MODES,
  PORT_RANGE_TEXT,
  READY_TIMEOUT_MS,
  clamp,
  errorText,
  isPlainObject,
  positiveOrDefault,
  sanitizeForJson,
  trimOrEmpty,
} from "./pure/common.js";
import { PLAYWRIGHT_KILL_TIMEOUT_MS, PLAYWRIGHT_TIMEOUT_MS } from "./pure/playwright.js";
import { instancesPathOf, mergeInstancesState, parseInstancesState, serializeInstances } from "./pure/instances.js";
import { exeKindOfPath } from "./pure/start.js";
import {
  chooseWorkspaceRoot,
  parentOf,
  parseGitBranch,
  playwrightCwdFailure,
  sessionCwdOf,
} from "dsh-doodle-workspace";

import { registry, advanceServerIdPast, resetRegistry, setPlaywrightRunning } from "./runtime/registry.js";
import { probePort } from "./runtime/process.js";
import {
  registerPlaywrightTool,
  registerSqlTool,
  registerStartTool,
  registerStatusTool,
  registerStopTool,
} from "./tools.js";

export { Config, inject, name } from "./schemas.js";

export { errorText, sanitizeForJson } from "./pure/common.js";

export { getServer, resetRegistry } from "./runtime/registry.js";

// 跨文件导入会变成加载期绑定：宿主把模块图求值后它可能丢掉，
// 变成「MODES is not defined」这类运行期错误。这里按值取一次再持有，
// 后续逻辑只用本文件的局部常量，不再依赖导入绑定。
const MODE_NAMES = [...MODES];

/**
 * 把配置规整成运行期用的 resolved 对象。
 *
 * @param {any} config
 * @returns {any}
 */
function resolveConfig(config) {
  const resolved = {
    defaultCwd: trimOrEmpty(config?.defaultCwd),
    defaultMode: MODE_NAMES.includes(config?.defaultMode) ? config.defaultMode : DEFAULT_MODE,
    defaultRoot: trimOrEmpty(config?.defaultRoot),
    defaultConfigPaths: isPlainObject(config?.defaultConfigPaths) ? config.defaultConfigPaths : {},
    readyPath: trimOrEmpty(config?.readyPath) || DEFAULT_READY_PATH,
    readyTimeoutMs: positiveOrDefault(config?.readyTimeoutMs, READY_TIMEOUT_MS),
    stopTimeoutMs: positiveOrDefault(config?.stopTimeoutMs, DEFAULT_STOP_TIMEOUT_MS),
    maxOutputBytes: clamp(config?.maxOutputBytes ?? DEFAULT_MAX_OUTPUT_BYTES, MIN_OUTPUT_BYTES, MAX_OUTPUT_BYTES),
    guidanceOrder: config?.guidanceOrder ?? DEFAULT_GUIDANCE_ORDER,
    playwrightCwd: trimOrEmpty(config?.playwrightCwd),
    playwrightNodePath: trimOrEmpty(config?.playwrightNodePath),
    playwrightCliPath: trimOrEmpty(config?.playwrightCliPath),
    playwrightDefaultBaseUrl: trimOrEmpty(config?.playwrightDefaultBaseUrl).replace(/\/+$/, ""),
    playwrightTimeoutMs: positiveOrDefault(config?.playwrightTimeoutMs, PLAYWRIGHT_TIMEOUT_MS),
    playwrightKillTimeoutMs: positiveOrDefault(config?.playwrightKillTimeoutMs, PLAYWRIGHT_KILL_TIMEOUT_MS),
    stateDir: trimOrEmpty(config?.stateDir),
    instancesFile: trimOrEmpty(config?.instancesFile),
  };
  if (!Number.isFinite(resolved.guidanceOrder)) {
    throw new Error(`kitsu-supplement: guidanceOrder must be a finite number, got ${String(config?.guidanceOrder)}`);
  }
  return resolved;
}

/**
 * @param {any} ctx
 * @param {any} config
 */
export async function apply(ctx, config) {
  const resolved = resolveConfig(config);

  const os = await import("node:os");
  const statePath = instancesPathOf({
    stateDir: resolved.stateDir,
    instancesFile: resolved.instancesFile,
    tmpRoot: os.tmpdir(),
  });
  /** @type {string[]} */
  const stateWarnings = [];

  /**
   * 解析发起者的会话工作区根目录；都拿不到就返回空串，由调用方报错。
   *
   * @param {any} subject agent 或会话（tools 的 exec.agent、systemPrompt 的 scope.agent）
   * @returns {string}
   */
  const workspaceRootOf = (subject) => {
    const agent = subject?.agent ?? subject;
    const service = ctx.get("workingDirectory");
    return chooseWorkspaceRoot({
      workingDirectory: agent === undefined || service === undefined ? "" : service.get(agent.session ?? agent),
      sessionCwd: sessionCwdOf(agent) || sessionCwdOf(subject),
      defaultDirectory: ctx.get("workingDirectory")?.defaultDirectory,
    });
  };

  /**
   * 跑一条 git 命令并返回 stdout；失败返回空串。
   *
   * @param {string} repoRoot
   * @param {string[]} gitArgs
   * @param {AbortSignal} signal
   * @returns {Promise<string>}
   */
  const runGit = async (repoRoot, gitArgs, signal) => {
    const handle = ctx.subprocess.spawn({
      argv: ["git", "-C", repoRoot, ...gitArgs],
      cwd: repoRoot,
      stdio: { stdin: "ignore", stdout: { maxBytes: 262144 }, stderr: { maxBytes: 65536 } },
      graceMs: 2000,
      signal,
    });
    try {
      await handle.done;
    } catch {
      return "";
    }
    const reader = handle.collected?.stdout;
    if (reader === undefined) return "";
    let text = "";
    let offset = 0;
    for (let guard = 0; guard < 1000; guard += 1) {
      const chunk = reader.readFrom(offset);
      if (chunk === undefined || chunk.nextOffset === offset) break;
      text += chunk.text;
      offset = chunk.nextOffset;
    }
    return text;
  };

  /**
   * 取会话所在分支名。与 set_env.ps1 同源：先看环境变量，再问 git，游离 HEAD 用短提交号。
   *
   * @param {string} repoRoot
   * @param {AbortSignal} signal
   * @returns {Promise<string>}
   */
  const branchOf = async (repoRoot, signal) => {
    if (repoRoot === "") return "";
    const fromEnv = parseGitBranch(process.env.DOODLE_GIT_BRANCH);
    if (fromEnv.kind === "branch") return fromEnv.name;
    const head = parseGitBranch(await runGit(repoRoot, ["rev-parse", "--abbrev-ref", "HEAD"], signal));
    if (head.kind === "branch") return head.name;
    if (head.kind === "detached") {
      const short = parseGitBranch(await runGit(repoRoot, ["rev-parse", "--short", "HEAD"], signal));
      return short.kind === "branch" ? short.name : "";
    }
    return "";
  };

  /**
   * 会话工作区里装着 @playwright/test 的 test_js 目录；找不到时 cwd 为空串。
   *
   * @param {any} subject
   * @param {AbortSignal} signal
   * @returns {Promise<{cwd:string, workspaceRoot:string, lookedAt:string[]}>}
   */
  const playwrightCwdOf = async (subject, signal) => {
    const fs = await import("node:fs/promises");
    const path = await import("node:path");
    const workspaceRoot = workspaceRootOf(subject);
    const fallbackDirs = [resolved.playwrightCwd];
    const parent = parentOf(workspaceRoot);
    if (parent !== "") fallbackDirs.push(`${parent}\\Doodle\\test_js`);
    const hasDependency = (dir) => {
      try {
        return fs.statSync(path.join(dir, "node_modules", "@playwright", "test")).isDirectory();
      } catch {
        return false;
      }
    };
    signal?.throwIfAborted();
    const chosen = choosePlaywrightCwd({ workspaceRoot, fallbackDirs, hasDependency });
    return { cwd: chosen.cwd, workspaceRoot, lookedAt: chosen.lookedAt };
  };

  const saveInstances = async () => {
    const fs = await import("node:fs/promises");
    const path = await import("node:path");
    try {
      await fs.mkdir(path.dirname(statePath), { recursive: true });
      await fs.writeFile(statePath, `${JSON.stringify(serializeInstances(registry), null, 2)}\n`, "utf8");
      return "";
    } catch (error) {
      const message = `实例快照写入失败（${statePath}）：${errorText(error)}`;
      if (!stateWarnings.includes(message)) stateWarnings.push(message);
      return message;
    }
  };

  const loadInstances = async () => {
    const fs = await import("node:fs/promises");
    /** @type {string} */
    let text;
    try {
      text = await fs.readFile(statePath, "utf8");
    } catch (error) {
      if (error?.code !== "ENOENT") stateWarnings.push(`实例快照读取失败（${statePath}）：${errorText(error)}`);
      return;
    }
    const parsed = parseInstancesState(text);
    if (parsed.problem !== undefined) {
      stateWarnings.push(`实例快照被忽略（${statePath}）：${parsed.problem}`);
      return;
    }
    /** @type {Map<number, number[]>} */
    const livePidsByPort = new Map();
    /** @type {number[]} */
    const ports = [];
    for (const item of parsed.records) {
      if (item.exited === true) continue;
      if (typeof item.port !== "number" || ports.includes(item.port)) continue;
      ports.push(item.port);
    }
    for (const port of ports) {
      const probe = await probePort(ctx, port);
      if (probe.error === undefined) livePidsByPort.set(port, probe.owners);
    }
    for (const item of mergeInstancesState({ saved: parsed.records, live: registry, livePidsByPort })) {
      if (registry.has(item.serverId)) continue;
      registry.set(item.serverId, {
        serverId: item.serverId,
        handle: null,
        mode: item.mode,
        port: item.port,
        pid: item.pid,
        readyPath: resolved.readyPath,
        startedAt: item.startedAt,
        exited: item.exited === true,
        exitCode: item.exited === true ? (item.exitCode ?? null) : null,
        exitSignal: null,
        exePath: item.exePath,
        exeKind: exeKindOfPath(item.exePath),
        cwd: item.cwd,
        configPath: undefined,
        dbPath: item.dbPath,
        command: item.command,
        adopted: true,
      });
      advanceServerIdPast(item.serverId);
    }
  };

  await loadInstances();

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
    const os2 = await import("node:os");
    const path = await import("node:path");
    const dir = path.join(os2.tmpdir(), "dsh-kitsu-supplement");
    await fs.mkdir(dir, { recursive: true });
    const file = path.join(dir, `${serverId}.json`);
    await fs.writeFile(file, `${JSON.stringify(value, null, 2)}\n`, "utf8");
    return file;
  };

  ctx.effect(() => () => {
    setPlaywrightRunning(null);
    for (const [id, record] of registry) {
      registry.delete(id);
      if (record.exited === true) continue;
      if (record.handle === null || record.handle === undefined) continue;
      try {
        record.handle.terminate();
      } catch {
        // 卸载时尽力而为：未停稳的进程仍由子进程提供方的清理兜底
      }
    }
    void saveInstances();
  });

  ctx.systemPrompt.section({
    name: GUIDANCE_SECTION,
    order: resolved.guidanceOrder,
    text: ({ scope }) =>
      ctx.tools.get("kitsu_server_start", scope) === undefined
        ? ""
        : [
            "`kitsu_server_start` 是准备测试后端的**唯一主要启动方式**：默认拉起 `doodle_kitsu_supplement_test.exe`（测试版，路由与生产一致，但 Maya / UE 外部进程被替换成 stub，不需要本机装 Maya / UE / 插件），返回的 `port` 就是传入的显式端口，就绪判定是对 `http://127.0.0.1:<port><readyPath>` 轮询 HTTP（任何状态码都算就绪，最多等 100 秒），测试脚本应引用返回的 `baseUrl` 而不是猜端口。可执行文件路径由**当前会话的工作区与 git 分支**推出（`<工作区上一级>\\build\\<分支>\\Ninja_debug\\bin\\<exeName>`，与 CMakePresets.json 的 binaryDir 同一表达式），参数只收 `exeName`（默认测试 exe，传 `doodle_kitsu_supplement.exe` 才是生产 exe），**不接受任何路径**；工作区、分支或 exe 解析不出来时工具直接失败并结束本轮，此时必须停下来交给用户检查，不要改传别的路径重试。返回里的 `exeKind`（test / production）标出这次跑的是哪一个。",
            `\`port\` 必须显式传，且必须是 ${PORT_RANGE_TEXT} 之间的整数；端口被占用时工具默认失败并给出占用 PID，只有显式传 \`killExisting: true\` 才强杀。\`mode: \"local\"\`（或 \`local: true\`）会给进程加 \`--local\`，并把 \`dbPath\` 作为 \`--dbPath\` 传给服务端；不传 dbPath 时 local 分支落进内存库（\`:memory:\`），重启即丢。`,
            "`kitsu_server_status` 查看本插件的实例、每个模式的槽位占用与端口占用；`kitsu_server_stop` 停掉本插件拉起的实例（含 DSH 重启后从状态文件认领的）；本插件不杀别人启动的进程。local 与 production 各最多 1 个在跑实例，同模式重复启动直接返回 `slot-in-use`，要替换就显式传 `replaceExisting: true`（先停旧的，结果在 `replaced`）。",
            "`kitsu_server_sql` 对磁盘库执行 sql 文件或片段（填测试数据），写完运行中的进程立刻能查到，**不要**为它停服或重启；执行前不确定就先 `dryRun: true` 走一遍。",
            "`kitsu_playwright_test` 跑 test_js 里的 Playwright 用例：`args` 是 `playwright` 之后那串参数、原样转发（如 `[\"test\", \"face_capture.js\"]`、`[\"test\", \"--list\"]`）；工作目录取**当前会话工作区**里装着 `node_modules/@playwright/test` 的 test_js（会话工作区没有依赖时才回落到插件配置的目录），找不到就失败并结束本轮、交给用户检查，不要改传 `cwd` 到别处。`baseUrl` 请直接传 `kitsu_server_start` 返回的 `baseUrl`（本工具不会自己去查实例表）。本工具不可并发：已经在跑时立刻返回 `busy`，不排队。",
            `服务端自己的日志写在 \`<root>\\log\`（root 即 \`kitsu_server_start\` 的 \`root\` 参数，不传则用配置的 defaultRoot，再退回服务端内置的 \`%TEMP%\\Doodle\`）：主日志 \`doodle_lib.txt\`，另有 \`doodle_long_task.txt\` / \`socket_io.txt\` / \`http.txt\` / \`doodle_main_error.txt\`，每个都是 1 MB 轮转文件；同名文件已存在时服务端会在文件名里补 UUID 而不是覆盖，所以崩溃那一轮的日志不会被下一次启动冲掉。本插件不转发这些日志：启动失败最多回显 4000 字符输出尾巴，查崩溃原因要按上面的路径用本地文件工具读 \`<root>\\log\`。`,
          ].join(" "),
  });

  const deps = { ctx, resolved, registry, stateWarnings, statePath, saveInstances, writeConfigFile, exists, workspaceRootOf, branchOf, playwrightCwdOf };

  registerStartTool(deps);
  registerStatusTool(deps);
  registerStopTool(deps);
  registerSqlTool(deps);
  registerPlaywrightTool(deps);
}
