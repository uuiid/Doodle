// dsh-ue —— 让 DSH 通过 UE 自带的 Engine\Build\BatchFiles\Build.bat 编译 Unreal Engine 的工程，
// 并启动 UnrealEditor.exe / UnrealEditor-Cmd.exe。
//
// 工具清单：
//   ue_build   编译一个 UE 目标（游戏工程 / 编辑器 / 引擎）
//   ue_editor  运行 UE 编辑器（配置的 .uproject 固定为第一个参数，其余参数透传）
//   ue_targets 只读列出当前配置下可编译的目标与可运行的编辑器
//
// 硬约束（见 cordis-plugin-development 技能）：
//   * 顶层不 import node: 内置模块，需要时在 apply 内动态 import。
//   * 所有注册走 ctx.effect。
//   * 本插件只需要 tools.register 与 systemPrompt.section。

import { defineTool } from '@deepseek-ai/dsh-tools';
import z from '@deepseek-ai/schemastery';

import {
  buildLaunchArgv,
  dirOf,
  editorExeName,
  editorExePath,
  EDITOR_EXE_KINDS,
  readEngineAssociation,
  renderCommandLine,
  resolveEditor,
  resolveProjectPath,
  resolveTarget,
} from './target.js';
import { invalid, runUeBuild, sanitizeForJson } from './runner.js';

export const name = 'ue-build';

export const inject = ['tools', 'subprocess', 'systemPrompt'];

export const Config = z.object({
  repoRoot: z.string().default('E:\\Doodle'),
  engineRoot: z.string().default('E:\\UnrealEngine'),
  engineBuildBat: z.string().default('Engine/Build/BatchFiles/Build.bat'),
  projectFile: z.string().default(''),
  pluginBuildScript: z.string().default(''),
  editorProjectFile: z.string().default(''),
  comspec: z.string().default('C:\\WINDOWS\\system32\\cmd.exe'),
  defaultPlatform: z.string().default('Win64'),
  defaultConfiguration: z.string().default('Development'),
  defaultTarget: z.string().default(''),
  buildTimeoutMs: z.number().default(5400000),
  editorTimeoutMs: z.number().default(1800000),
  maxOutputBytes: z.number().default(33554432),
  outputHeadBytes: z.number().default(6144),
  outputTailBytes: z.number().default(20480),
  guidanceOrder: z.number().default(1394),
});

const DIAGNOSTICS_SCHEMA = {
  type: 'array',
  items: {
    type: 'object',
    additionalProperties: false,
    properties: {
      severity: { type: 'string' },
      code: { type: 'string' },
      file: { type: 'string' },
      line: { type: 'integer' },
      column: { type: 'integer' },
      message: { type: 'string' },
    },
  },
};

const LOG_INFO_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  properties: {
    logDir: { type: 'string' },
    logFile: { type: 'string' },
    exists: { type: 'boolean' },
  },
};

const OUTCOME_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  properties: {
    ok: { type: 'boolean' },
    reason: { type: 'string' },
    code: { type: 'string' },
    durationMs: { type: 'integer' },
    exitCode: { oneOf: [{ type: 'integer' }, { type: 'null' }] },
    command: { type: 'string' },
    exe: { type: 'string' },
    ubtResult: { type: 'string' },
    ubtCompileFailed: { type: 'boolean' },
    ubtTotalTimeSeconds: { type: 'number' },
    target: {
      type: 'object',
      additionalProperties: false,
      properties: {
        mode: { type: 'string' },
        target: { type: 'string' },
        platform: { type: 'string' },
        configuration: { type: 'string' },
        projectFile: { type: 'string' },
        cwd: { type: 'string' },
        engineRoot: { type: 'string' },
        repoRoot: { type: 'string' },
        buildBat: { type: 'string' },
        exePath: { type: 'string' },
        log: LOG_INFO_SCHEMA,
      },
    },
    diagnostics: DIAGNOSTICS_SCHEMA,
    warnings: DIAGNOSTICS_SCHEMA,
    output: {
      type: 'object',
      additionalProperties: false,
      properties: {
        head: { type: 'string' },
        tail: { type: 'string' },
        truncated: { type: 'boolean' },
        totalBytes: { type: 'integer' },
        keyLines: { type: 'array', items: { type: 'string' } },
        keyLineCount: { type: 'integer' },
        totalLines: { type: 'integer' },
      },
    },
    spillPath: { type: 'string' },
  },
};

const TARGETS_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  properties: {
    ok: { type: 'boolean' },
    repoRoot: { type: 'string' },
    engineRoot: { type: 'string' },
    buildBat: { type: 'string' },
    comspec: { type: 'string' },
    defaultPlatform: { type: 'string' },
    defaultConfiguration: { type: 'string' },
    defaultTarget: { type: 'string' },
    targets: {
      type: 'array',
      items: {
        type: 'object',
        additionalProperties: false,
        properties: {
          label: { type: 'string' },
          mode: { type: 'string' },
          target: { type: 'string' },
          projectFile: { type: 'string' },
          engineAssociation: { type: 'string' },
          exePath: { type: 'string' },
          commandLine: { type: 'string' },
          logDir: { type: 'string' },
          logFile: { type: 'string' },
          logExists: { type: 'boolean' },
        },
      },
    },
    warnings: { type: 'array', items: { type: 'string' } },
    error: { type: 'string' },
  },
};

/**
 * @template T
 * @param {T|undefined} value
 * @param {boolean} ok
 * @returns {T|undefined}
 */
function keep(value, ok) {
  return ok ? value : undefined;
}

/**
 * 渲染一次构建的文本结果。
 *
 * @param {any} value
 * @returns {string}
 */
function renderOutcome(value) {
  const WARNING_RENDER_LIMIT = 5;
  const ERROR_RENDER_LIMIT = 40;
  const lines = [];
  const target = value.target ?? {};
  const where = target.mode === 'editor' ? 'editor' : target.mode === 'script' ? 'script' : 'UBT';
  const headline = target.mode === 'editor' ? 'ue_editor' : 'ue_build';
  lines.push(
    `${value.ok ? 'OK' : 'FAILED'} — ${headline} (${where}, exit ${value.exitCode ?? 'n/a'}, ${value.durationMs} ms)`,
  );
  if (value.ubtResult) {
    lines.push(`UBT result: ${value.ubtResult}` + (value.ubtTotalTimeSeconds ? ` (UBT 自身计时 ${value.ubtTotalTimeSeconds}s)` : ''));
  }
  lines.push(`reason: ${value.reason}`);
  if (value.exe) lines.push(`exe: ${value.exe}`);
  if (target.target) {
    lines.push(`target: ${target.target} | ${target.platform ?? '?'} | ${target.configuration ?? '?'}`);
  }
  if (target.projectFile) lines.push(`project: ${target.projectFile}`);
  if (target.cwd) lines.push(`cwd: ${target.cwd}`);
  lines.push(`command: ${value.command}`);

  const errors = value.diagnostics ?? [];
  const warnings = value.warnings ?? [];
  if (errors.length > 0) {
    lines.push('', `errors (${errors.length}):`);
    for (const item of errors.slice(0, ERROR_RENDER_LIMIT)) {
      const loc = item.file
        ? `${item.file}${item.line ? `(${item.line}${item.column ? `,${item.column}` : ''})` : ''}`
        : '';
      lines.push(`  [${item.code ?? 'error'}] ${loc}${loc ? ' ' : ''}${item.message}`);
    }
    if (errors.length > ERROR_RENDER_LIMIT) lines.push(`  … ${errors.length - ERROR_RENDER_LIMIT} more error(s)`);
  }
  if (warnings.length > 0) {
    let warningsShown = 0;
    lines.push('', `warnings (${warnings.length}) — first ${WARNING_RENDER_LIMIT}:`);
    for (const item of warnings.slice(0, WARNING_RENDER_LIMIT)) {
      const loc = item.file ? `${item.file}${item.line ? `(${item.line})` : ''}` : '';
      lines.push(`  [${item.code ?? 'warning'}] ${loc}${loc ? ' ' : ''}${item.message}`);
      warningsShown += 1;
    }
    if (warnings.length > warningsShown) lines.push(`  … ${warnings.length - warningsShown} more warning(s)`);
  }

  const logInfo = value.target?.log;
  if (logInfo?.logFile) {
    lines.push('', `log: ${logInfo.logFile} (${logInfo.exists ? 'exists' : 'not created by this run'})`);
    lines.push(`  dir: ${logInfo.logDir}`);
  }

  const keyLines = value.output?.keyLines ?? [];
  if (keyLines.length > 0) {
    const total = value.output?.totalLines ?? 0;
    lines.push('', `key lines (${keyLines.length} of ${total} non-empty line(s)):`);
    for (const line of keyLines) lines.push(`  ${line}`);
  }

  const output = value.output ?? { head: '', tail: '', truncated: false, totalBytes: 0 };
  if (output.head) lines.push('', '--- output head ---', output.head);
  if (output.tail) lines.push('', '--- output tail ---', output.tail);
  if (output.truncated) {
    lines.push('', `(output truncated; ${output.totalBytes} bytes total${value.spillPath ? `; full log: ${value.spillPath}` : ''})`);
  }
  return lines.join('\n');
}

/**
 * @param {any} value
 * @returns {string}
 */

function renderTargets(value) {
  const lines = [];
  lines.push(`build script: ${value.buildBat ?? '(missing)'}`);
  lines.push(`cmd.exe: ${value.comspec ?? '(missing)'}`);
  lines.push(`defaults: ${value.defaultPlatform} | ${value.defaultConfiguration}`);
  if (value.defaultTarget) lines.push(`default target: ${value.defaultTarget}`);
  const targets = value.targets ?? [];
  if (targets.length === 0) {
    lines.push('', '没有可编译目标。');
  } else {
    lines.push('', `targets (${targets.length}):`);
    for (const item of targets) {
      lines.push(`  ${item.label}  [${item.mode}]  target=${item.target}`);
      if (item.projectFile) {
        lines.push(
          `      project: ${item.projectFile}` +
          (item.engineAssociation ? ` (EngineAssociation ${item.engineAssociation})` : ''),
        );
      }
      if (item.exePath) lines.push(`      exe: ${item.exePath}`);
      if (item.logFile) lines.push(`      log: ${item.logFile}`);
      lines.push(`      command: ${item.commandLine}`);
    }
  }
  for (const warning of value.warnings ?? []) lines.push('', `warning: ${warning}`);
  if (value.error) lines.push('', `error: ${value.error}`);
  return lines.join('\n');
}

/**
 * 插件入口。
 *
 * @param {any} ctx
 * @param {any} config
 */
export function apply(ctx, config) {
  const normalizeRoot = (value, fallback) => {
    const text = String(value ?? '').trim().replace(/[\\/]+$/, '');
    if (text.length === 0) return fallback;
    if (/^[\\/]dsh_work/i.test(text)) return fallback;
    return text;
  };
  const repoRoot = normalizeRoot(config.repoRoot, 'E:\\Doodle');
  const engineRoot = normalizeRoot(config.engineRoot, 'E:\\UnrealEngine');
  const buildBatRel = String(config.engineBuildBat ?? 'Engine/Build/BatchFiles/Build.bat').replace(/\//g, '\\');
  const buildBat = `${engineRoot}\\${buildBatRel}`;
  const projectFileRel = String(config.projectFile ?? '');
  const pluginBuildScriptRel = String(config.pluginBuildScript ?? '');
  const defaultTargetName = String(config.defaultTarget ?? '');
  const editorProjectFileRel = String(config.editorProjectFile ?? '').length > 0
    ? String(config.editorProjectFile)
    : projectFileRel;

  /** 动态 import 是插件硬约束：顶层不得出现 node: 内置模块。 */
  let fspPromise;
  const getFs = () => {
    if (!fspPromise) fspPromise = import('node:fs/promises');
    return fspPromise;
  };

  const targetContext = () => ({
    repoRoot,
    defaultProjectFileRel: projectFileRel,
    engineRoot,
    targetProjectFileRel: projectFileRel,
    pluginBuildScriptRel,
    defaultTarget: defaultTargetName,
  });

  /**
   * 把「同步存在性检查」做成对 async stat 的一次批量预取：
   * resolveTarget 需要同步的 exists()，所以先把候选路径全部探一遍。
   *
   * @param {string[]} candidates
   * @returns {Promise<(absolute:string)=>boolean>}
   */
  async function makeExists(candidates) {
    const fsp = await getFs();
    /** @type {Map<string, boolean>} */
    const known = new Map();
    await Promise.all(candidates.filter(Boolean).map(async (absolute) => {
      try {
        const stat = await fsp.stat(absolute);
        known.set(absolute.toLowerCase(), stat.isFile());
      } catch {
        known.set(absolute.toLowerCase(), false);
      }
    }));
    /**
     * @param {string} absolute
     * @returns {boolean}
     */
    return (absolute) => known.get(String(absolute).toLowerCase()) === true;
  }

  /**
   * @param {number|undefined} requested
   * @param {number} fallback
   * @returns {number}
   */
  function resolveTimeout(requested, fallback) {
    const value = typeof requested === 'number' && Number.isFinite(requested) ? Math.trunc(requested) : fallback;
    return Math.min(Math.max(value, 1), 2147483647);
  }

  /**
   * @param {any} exec
   * @param {number} timeoutMs
   * @returns {AbortSignal}
   */
  function timeoutSignal(exec, timeoutMs) {
    return AbortSignal.any([exec.signal, AbortSignal.timeout(timeoutMs)]);
  }

  /**
   * 计算编辑器日志位置：<工程目录>\Saved\Logs 与其中的 <项目名>.log。
   *
   * @param {string} projectFile
   * @returns {{logDir:string, logFile:string}}
   */
  function editorLogLocation(projectFile) {
    const normalized = String(projectFile ?? '').replace(/[\\/]+$/, '');
    const baseName = normalized.split(/[\\/]/).pop() ?? '';
    const stem = baseName.replace(/\.uproject$/i, '');
    const projectDir = dirOf(normalized);
    const logDir = `${projectDir}\\Saved\\Logs`;
    return { logDir, logFile: `${logDir}\\${stem}.log` };
  }

  /**
   * @param {string} logFile
   * @returns {Promise<boolean>}
   */
  async function logFileExists(logFile) {
    const fsp = await getFs();
    try {
      const stat = await fsp.stat(logFile);
      return stat.isFile();
    } catch {
      return false;
    }
  }

  /**
   * 按请求解析出一次确定的编辑器启动（含 argv 与 cwd）。
   *
   * @param {any} args
   * @returns {Promise<{ok:true, resolved:any, argv:string[], command:string} | {ok:false, problems:string[], command:string, target:Record<string, any>}>}
   */
  async function planEditor(args) {
    const exePath = editorExePath(editorExeName(args.exe), engineRoot);
    const projectFile = resolveProjectPath(editorProjectFileRel, repoRoot);
    const exists = await makeExists([exePath, projectFile]);

    const outcome = resolveEditor(
      { exe: args.exe, extraArgs: args.extraArgs, timeoutMs: args.timeoutMs },
      { repoRoot, engineRoot, editorProjectFileRel },
      exists,
    );
    if (!outcome.ok) {
      return {
        ok: false,
        problems: outcome.problems,
        command: renderCommandLine([exePath, projectFile]),
        target: { mode: 'editor', exePath, projectFile, cwd: dirOf(projectFile), engineRoot, repoRoot },
      };
    }

    const resolved = outcome.resolved;
    return {
      ok: true,
      resolved,
      argv: resolved.launchArgs,
      command: renderCommandLine(resolved.launchArgs),
      log: editorLogLocation(resolved.projectFile),
    };
  }

  /**
   * 按请求解析出一次确定的构建（含 argv 与 cwd）。
   *
   * @param {any} args
   * @returns {Promise<{ok:true, resolved:any, argv:string[], command:string} | {ok:false, problems:string[], command:string, target:Record<string, any>}>}
   */
  async function plan(args) {
    const context = targetContext();
    const repoPath = (rel) => (rel ? `${repoRoot}\\${String(rel).replace(/\//g, '\\')}` : '');
    const candidates = [
      buildBat,
      resolveProjectPath(projectFileRel, repoRoot),
      repoPath(pluginBuildScriptRel),
    ];
    if (typeof args.projectAbsolute === 'string') candidates.push(args.projectAbsolute);
    if (typeof args.projectFile === 'string') candidates.push(resolveProjectPath(args.projectFile, repoRoot));
    const exists = await makeExists(candidates);

    const outcome = resolveTarget(args, context, exists);
    if (!outcome.ok) {
      return {
        ok: false,
        problems: outcome.problems,
        command: 'Build.bat <target> <platform> <configuration>',
        target: { engineRoot, repoRoot, buildBat },
      };
    }

    const resolved = outcome.resolved;
    const argv = resolved.mode === 'ubt'
      ? buildLaunchArgv({ comspec: config.comspec, buildBat, args: resolved.ubtArgs })
      : buildLaunchArgv({ comspec: config.comspec, buildBat: resolved.target, args: [] });

    return {
      ok: true,
      resolved,
      argv,
      command: renderCommandLine(argv),
    };
  }

  // ------------------------------------------------------------------ ue_build

  ctx.effect(() => ctx.tools.register(defineTool({
    name: 'ue_build',
    description:
      '通过 UE 自带的 Engine\\Build\\BatchFiles\\Build.bat 编译一个 Unreal Engine 目标。' +
      '默认编译 Doodle 仓库的 UE 工程（target 由 config.defaultTarget 决定，Win64 / Development），支持增量编译。' +
      'goal: 编译游戏工程传 projectFile；编译编辑器 target 名通常是 `<Project>Editor`，但并非所有工程都有这个 target，' +
      'UBT 报 `Couldn\'t find target rules file` 说明名字错了，先用 ue_targets 或改用工程里实际定义的那个 target；编译引擎传 engine=true。' +
      'UBT 的真实失败写在日志里而不是退出码上，所以返回值里 ok/reason/code 是权威结论，' +
      'diagnostics 已解析出 MSVC 的 文件(行,列) 与 UBT 的 LogCompile/LogBuildException 错误。' +
      '首次编译或改了引擎头文件时可能耗时数十分钟，不要因为慢就重复调用（Build.bat 自带全局锁）。',
    parameters: {
      target: {
        type: 'string',
        description: 'UBT target 名，例如 MyGame / UnrealEditor / UnrealPak。省略时先取 config.defaultTarget，没有配置才回退到 `<工程名>Editor`。',
      },
      projectFile: {
        type: 'string',
        description: '相对 repoRoot 或绝对路径的 .uproject，例如 MyGame/MyGame.uproject 或 E:\\ue_main\\ue_main.uproject。省略时用插件 config 的 projectFile，再否则用 pluginBuildScript。',
      },
      engine: {
        type: 'boolean',
        description: '为 true 时编译引擎目标（默认 target=UnrealEditor），需要插件已配置一个 .uproject 供 UBT 解析引擎模块。',
      },
      scope: {
        type: 'string',
        enum: ['repo', 'absolute'],
        description: 'projectAbsolute 的解析口径：repo（默认，相对 repoRoot）或 absolute（工程根目录不在仓库内时必须用）。',
      },
      projectAbsolute: {
        type: 'string',
        description: '带 scope=absolute 显式指定口径时使用的 .uproject 绝对路径（项目根不在仓库内，例如 E:\\UnrealEngine\\MyGame\\MyGame.uproject）；只给绝对路径也可以直接写进 projectFile。',
      },
      platform: { type: 'string', description: '目标平台，默认 Win64。' },
      configuration: { type: 'string', description: 'Debug / DebugGame / Development（默认）/ Shipping / Test。' },
      extraArgs: {
        type: 'array',
        items: { type: 'string' },
        description: '透传给 UBT 的额外开关，例如 ["-WaitMutex"]；不接受 -project/-target/-platform/-configuration/-mode。',
      },
      timeoutMs: { type: 'integer', description: `本次调用的超时毫秒数，默认 ${config.buildTimeoutMs}。超时会终止进程树并返回失败。` },
    },
    output: {
      schema: OUTCOME_SCHEMA,
      render: (_args, value) => [{ type: 'text', text: renderOutcome(value) }],
    },
    timeoutMs: config.buildTimeoutMs + 60000,
    isConcurrencySafe: () => false,
    async execute(args, exec) {
      const planned = await plan(args);
      if (!planned.ok) {
        return invalid(planned.command, planned.problems, planned.target);
      }

      const resolved = planned.resolved;
      const timeoutMs = resolveTimeout(args.timeoutMs, config.buildTimeoutMs);

      return runUeBuild(ctx, {
        buildBat: resolved.mode === 'ubt' ? buildBat : resolved.target,
        cwd: resolved.cwd,
        argv: planned.argv,
        command: planned.command,
        engineRoot,
        repoRoot,
        target: resolved.target,
        platform: resolved.platform,
        configuration: resolved.configuration,
        projectFile: keep(resolved.projectFile || undefined, true),
        mode: resolved.mode,
        timeoutMs,
        maxOutputBytes: config.maxOutputBytes,
        outputHeadBytes: config.outputHeadBytes,
        outputTailBytes: config.outputTailBytes,
        signal: timeoutSignal(exec, timeoutMs),
      });
    },
  })));

  // ---------------------------------------------------------------- ue_editor

  ctx.effect(() => ctx.tools.register(defineTool({
    name: 'ue_editor',
    description:
      '运行 Unreal Engine 编辑器（UnrealEditor-Cmd.exe 或 UnrealEditor.exe），把插件配置里的 .uproject 固定成第一个参数，其余参数全部透传。' +
      '对应 UE 官方用法 `UnrealEditor-Cmd.exe <Project.uproject> <其它参数>`：把 `-run=...`、`-script=...`、`-ExecCmds=...`、`-game`、`-unattended`、' +
      '`-stdout`、`-NullRHI` 等原样放进 extraArgs。DSH 用 Windows Job 以隐藏窗口启动子进程，所以 GUI 版 UnrealEditor.exe 不会弹出窗口，只能读它的 stdout 日志；' +
      '默认 exe=cmd 无界面、输出最完整。编辑器是长驻进程：要么在参数里给它退出命令（-ExecCmds="QUIT_EDITOR"、-run=... 脚本里退出、-game -unattended 跑完退出），' +
      '要么接受它在 timeoutMs 之后被终止（返回 code=cancelled）。',
    parameters: {
      exe: {
        type: 'string',
        enum: EDITOR_EXE_KINDS,
        description: 'cmd（默认，UnrealEditor-Cmd.exe，无界面、stdout 最完整）或 editor（UnrealEditor.exe，GUI 进程但窗口被 DSH 隐藏）。',
      },
      extraArgs: {
        type: 'array',
        items: { type: 'string' },
        description:
          '原样追加在 .uproject 之后的编辑器参数，每项一个参数，如 ["-game","-unattended","-stdout","-NullRHI","-ExecCmds=QUIT_EDITOR"]。' +
          '不接受 -project / -absproject（工程由配置固定为第一个参数），也不接受 -abslog / -abslog=...。',
      },
      timeoutMs: { type: 'integer', description: `本次调用的超时毫秒数，默认 ${config.editorTimeoutMs}。超时会终止进程树并返回 cancelled。` },
    },
    output: {
      schema: OUTCOME_SCHEMA,
      render: (_args, value) => [{ type: 'text', text: renderOutcome(value) }],
    },
    timeoutMs: config.editorTimeoutMs + 60000,
    isConcurrencySafe: () => false,
    async execute(args, exec) {
      const planned = await planEditor(args);
      if (!planned.ok) {
        return invalid(planned.command, planned.problems, planned.target);
      }

      const resolved = planned.resolved;
      const timeoutMs = resolveTimeout(args.timeoutMs, config.editorTimeoutMs);

      return runUeBuild(ctx, {
        buildBat: resolved.exePath,
        exePath: resolved.exePath,
        cwd: resolved.cwd,
        argv: planned.argv,
        command: planned.command,
        engineRoot,
        repoRoot,
        target: resolved.exeName,
        mode: resolved.mode,
        projectFile: resolved.projectFile,
        timeoutMs,
        maxOutputBytes: config.maxOutputBytes,
        outputHeadBytes: config.outputHeadBytes,
        outputTailBytes: config.outputTailBytes,
        signal: timeoutSignal(exec, timeoutMs),
        log: { ...planned.log, exists: await logFileExists(planned.log.logFile) },
      });
    },
  })));

  // ---------------------------------------------------------------- ue_targets

  ctx.effect(() => ctx.tools.register(defineTool({
    name: 'ue_targets',
    description:
      '只读列出 dsh-ue 当前能编译的 UE 目标（默认工程、引擎目标、仓库里的插件工程脚本）与可运行的编辑器，' +
      '以及每一项实际会执行的命令行。不确定 ue_build / ue_editor 该传什么参数时先调它。',
    parameters: {},
    output: {
      schema: TARGETS_SCHEMA,
      render: (_args, value) => [{ type: 'text', text: renderTargets(value) }],
    },
    timeoutMs: 30000,
    isConcurrencySafe: () => true,
    async execute() {
      const fsp = await getFs();
      const warnings = [];
      const repoPath = (rel) => `${repoRoot}\\${String(rel).replace(/\//g, '\\')}`;

      /**
       * @param {string} absolute
       * @returns {Promise<string|undefined>}
       */
      const engineAssociationOf = async (absolute) => {
        if (!absolute) return undefined;
        try {
          return readEngineAssociation(await fsp.readFile(absolute, 'utf8'));
        } catch {
          return undefined;
        }
      };

      const batExists = await fsp.stat(buildBat).then((stat) => stat.isFile(), () => false);
      if (!batExists) warnings.push(`引擎构建脚本不存在：${buildBat}（检查 config 的 engineRoot）`);

      const lastLock = engineRoot.split(/[\\/]/).join('-');
      warnings.push(
        'Build.bat 用 %tmp% 下的锁文件串行化：同一时刻只能跑一个 UE 构建；' +
        `CC 编译器的任务数由 UE_USE_FASTBUILD / -MaxParallelActions 决定（本机 tmp 锁名前缀 ${lastLock}）。`,
      );

      /** @type {any[]} */
      const targets = [];
      const addUbt = (label, targetName, projectAbsolute, association) => {
        const ubtArgs = [targetName, config.defaultPlatform, config.defaultConfiguration];
        if (projectAbsolute) ubtArgs.push(`-Project=${projectAbsolute}`);
        const argv = buildLaunchArgv({ comspec: config.comspec, buildBat, args: ubtArgs });
        targets.push({
          label,
          mode: 'ubt',
          target: targetName,
          projectFile: projectAbsolute,
          engineAssociation: association,
          commandLine: renderCommandLine(argv),
        });
      };

      if (projectFileRel) {
        const projectAbsolute = resolveProjectPath(projectFileRel, repoRoot);
        const projectExists = await fsp.stat(projectAbsolute).then((stat) => stat.isFile(), () => false);
        if (!projectExists) warnings.push(`配置的 projectFile 不存在：${projectAbsolute}`);
        const association = await engineAssociationOf(projectAbsolute);
        const base = projectBaseName(projectAbsolute);
        addUbt(`默认工程（defaultTarget 或 ${base}）`, defaultTargetName || `${base}Editor`, projectAbsolute, association);
        addUbt('默认工程游戏', base, projectAbsolute, association);
        addUbt('引擎编辑器 UnrealEditor', 'UnrealEditor', projectAbsolute, association);
      } else {
        warnings.push('未配置 projectFile：只能通过 ue_build 的 projectFile / projectAbsolute 参数指定 .uproject。');
      }

      if (editorProjectFileRel) {
        const editorProject = resolveProjectPath(editorProjectFileRel, repoRoot);
        const editorAssociation = await engineAssociationOf(editorProject);
        const editorLog = editorLogLocation(editorProject);
        for (const exeKind of EDITOR_EXE_KINDS) {
          const exePath = editorExePath(exeKind, engineRoot);
          const exeExists = await fsp.stat(exePath).then((stat) => stat.isFile(), () => false);
          if (!exeExists) warnings.push(`编辑器不存在：${exePath}（检查 config 的 engineRoot，或先编译编辑器目标）`);
          targets.push({
            label: exeKind === 'cmd' ? '编辑器 UnrealEditor-Cmd.exe' : '编辑器 UnrealEditor.exe（窗口被 DSH 隐藏）',
            mode: 'editor',
            target: editorExeName(exeKind),
            projectFile: editorProject,
            engineAssociation: editorAssociation,
            exePath,
            commandLine: renderCommandLine([exePath, editorProject]),
            logDir: editorLog.logDir,
            logFile: editorLog.logFile,
          });
        }
      } else {
        warnings.push('未配置 projectFile / editorProjectFile：ue_editor 无法确定要启动的 .uproject。');
      }

      if (pluginBuildScriptRel) {
        const scriptAbsolute = repoPath(pluginBuildScriptRel);
        const scriptExists = await fsp.stat(scriptAbsolute).then((stat) => stat.isFile(), () => false);
        if (!scriptExists) {
          warnings.push(`配置的 pluginBuildScript 不存在：${scriptAbsolute}`);
        } else {
          targets.push({
            label: '插件工程脚本',
            mode: 'script',
            target: scriptAbsolute,
            commandLine: renderCommandLine(buildLaunchArgv({ comspec: config.comspec, buildBat: scriptAbsolute, args: [] })),
          });
        }
      }

      return sanitizeForJson({
        ok: batExists,
        repoRoot,
        engineRoot,
        buildBat,
        comspec: config.comspec,
        defaultPlatform: config.defaultPlatform,
        defaultConfiguration: config.defaultConfiguration,
        defaultTarget: defaultTargetName || undefined,
        targets,
        warnings,
      });
    },
  })));

  // ------------------------------------------------------------ 提示词指引段

  ctx.effect(() => ctx.systemPrompt.section({
    name: 'ue-build',
    order: config.guidanceOrder,
    text: ({ scope }) => {
      const hasBuild = ctx.tools.get('ue_build', scope) !== undefined;
      const hasEditor = ctx.tools.get('ue_editor', scope) !== undefined;
      const hasTargets = ctx.tools.get('ue_targets', scope) !== undefined;
      if (!hasBuild && !hasEditor && !hasTargets) return '';

      /** @type {string[]} */
      const lines = ['## Unreal Engine（dsh-ue）', ''];
      if (hasTargets) {
        lines.push('- `ue_targets` —— 只读列出当前可编译的目标、可运行的编辑器与它们实际会执行的命令行；不确定传参时先调它。');
      }
      if (hasBuild) {
        lines.push(
          `- \`ue_build\` —— 编译一个 UE 目标，执行 \`${buildBat}\`（cmd.exe 调用，cwd 在 .uproject 目录）；默认 Win64、Development、增量。`,
          `  不传 target 时用 config.defaultTarget，没配置才回退到 \`<工程名>Editor\`（当前：${defaultTargetName || '未配置'}）。`,
          '  编译指定工程用 `projectFile`（相对 repoRoot 或绝对路径）；编译引擎用 `engine: true`（默认 target=UnrealEditor）。',
        );
      }
      if (hasEditor) {
        lines.push(
          `- \`ue_editor\` —— 运行编辑器：插件配置的 \`.uproject\`（当前：${editorProjectFileRel || '未配置'}）固定为第一个参数，` +
          '其余参数全部透传给 `extraArgs`，即 `UnrealEditor-Cmd.exe <Project.uproject> <extraArgs...>`。',
          '  `exe` 默认 `cmd`（UnrealEditor-Cmd.exe，无界面、stdout 最完整）；`exe: "editor"` 启动 GUI 版，但 DSH 以隐藏窗口启动，看不到界面，只能读 stdout 日志。',
        );
      }
      lines.push('', '规则：');
      if (hasBuild) {
        lines.push(
          '- **不要手工拼** `Build.bat` 命令行，也不要直接调 `dotnet UnrealBuildTool.dll` —— 环境变量（DOTNET_ROOT、bundled dotnet）由 Build.bat 自己设置。',
          '- UBT 的失败经常仍以 exit code 0 结束：**以返回值的 `ok` / `code` 为权威结论**，`diagnostics` 已解析出 `文件(行,列) error CXXXX`。',
          '- `code=compile-failed` 才是 C++ 编译错误；`ubt-failed` 表示 UBT 自己拒绝了请求（目标不存在、模块依赖、工程类型不符）。',
          '- 首次构建和改引擎头文件后的构建可能耗时数十分钟：**不要因为慢就重复调用**，也别并发调用两个 `ue_build`（Build.bat 有全局锁，第二个会一直等）。',
          '- 不要在构建期间改动头文件或 `.uproject`，会造成半截状态。',
        );
      }
      if (hasEditor) {
        lines.push(
          '- 编辑器是长驻进程：`ue_editor` 会一直运行到编辑器自己退出，或在 `timeoutMs` 后被终止（此时返回 `code=cancelled`）。' +
          '要在参数里给它退出方式，例如 `-ExecCmds="QUIT_EDITOR"`、`-run=pythonscript -script=...` 的脚本里退出、`-game -unattended`（跑完自动退出）。',
          '- 需要读日志时加 `-stdout`（`-game` 要加 `-log`）；自动化场景加 `-unattended -stdout`，必要时 `-NullRHI` 跳过渲染。',
          '- 参数里的引号按宿主 `CommandLineToArgvW` 规则转义：带空格的路径直接把 `"..."` 写进同一个 `extraArgs` 项即可（如 `-script="D:\\a b\\x.py"`）。',
          '- 不要用 `ue_editor` 去启动另一个 `ue_editor`，也不要同时开两个编辑器操作同一个工程；启动前请确认编辑器目标已用 `ue_build` 编译通过。',
        );
      }
      return lines.join('\n');
    },
  }));
}

/**
 * MyGame.uproject → MyGame
 *
 * @param {string} projectAbsolute
 * @returns {string}
 */
function projectBaseName(projectAbsolute) {
  const base = String(projectAbsolute).split(/[\\/]/).pop() ?? '';
  const name = base.replace(/\.uproject$/i, '');
  return name.length > 0 ? name : 'Game';
}
