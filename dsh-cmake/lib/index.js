// dsh-cmake —— 让 DSH 经 set_env.ps1 驱动 Doodle 项目的 CMake 配置、构建与清理。
//
// 工具清单：
//   cmake_presets   只读解析 CMakePresets.json / CMakeUserPresets.json
//   cmake_configure cmake --preset <preset>
//   cmake_build     cmake --build --preset <preset> | --build <dir> --target <...>
//   cmake_clean     cmake --build <dir> --target clean
//
// 硬约束（见 cordis-plugin-development 技能）：
//   * 顶层不 import node: 内置模块，需要时在 apply 内动态 import。
//   * 所有注册走 ctx.effect。
//   * 用最弱机制，本插件只需要 tools.register 与 systemPrompt.section。

import { defineTool } from '@deepseek-ai/dsh-tools';
import z from '@deepseek-ai/schemastery';

import { isSafePresetName, isSafeRelativeDir } from './diagnostics.js';
import { summarizePresets, binaryDirForConfigurePreset } from './presets.js';
import { runCmake, sanitizeForJson } from './runner.js';

export const name = 'cmake';

export const inject = ['tools', 'subprocess', 'systemPrompt'];

export const Config = z.object({
  repoRoot: z.string().default('E:\\Doodle'),
  cmakeExecutable: z.string().default('C:/PROGRA~1/CMake/bin/cmake.exe'),
  setEnvScript: z.string().default('build_script/build/set_env.ps1'),
  powershellPath: z.string().default('powershell.exe'),
  defaultBuildDir: z.string().default('build'),
  defaultConfigurePreset: z.string().default('Ninja_debug'),
  defaultBuildPreset: z.string().default('debug_doodle'),
  configureTimeoutMs: z.number().default(600000),
  buildTimeoutMs: z.number().default(3600000),
  cleanTimeoutMs: z.number().default(300000),
  maxParallel: z.number().default(8),
  maxOutputBytes: z.number().default(33554432),
  outputHeadBytes: z.number().default(4096),
  outputTailBytes: z.number().default(12288),
  guidanceOrder: z.number().default(1395),
});

const OUTCOME_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  properties: {
    ok: { type: 'boolean' },
    reason: { type: 'string' },
    exitCode: { oneOf: [{ type: 'integer' }, { type: 'null' }] },
    durationMs: { type: 'integer' },
    command: { type: 'string' },
    phase: { type: 'string' },
    target: {
      type: 'object',
      additionalProperties: false,
      properties: {
        repoRoot: { type: 'string' },
        preset: { type: 'string' },
        buildDir: { type: 'string' },
      },
    },
    diagnostics: {
      type: 'array',
      items: {
        type: 'object',
        additionalProperties: false,
        properties: {
          severity: { type: 'string' },
          file: { type: 'string' },
          line: { type: 'integer' },
          code: { type: 'string' },
          message: { type: 'string' },
        },
      },
    },
    output: {
      type: 'object',
      additionalProperties: false,
      properties: {
        head: { type: 'string' },
        tail: { type: 'string' },
        truncated: { type: 'boolean' },
        totalBytes: { type: 'integer' },
      },
    },
  },
};

const PRESETS_SCHEMA = {
  type: 'object',
  additionalProperties: false,
  properties: {
    ok: { type: 'boolean' },
    repoRoot: { type: 'string' },
    presetsDir: { type: 'string' },
    sources: { type: 'array', items: { type: 'string' } },
    configure: {
      type: 'array',
      items: {
        type: 'object',
        additionalProperties: false,
        properties: {
          name: { type: 'string' },
          hidden: { type: 'boolean' },
          description: { type: 'string' },
          inherits: { type: 'array', items: { type: 'string' } },
          generator: { type: 'string' },
          binaryDir: { type: 'string' },
        },
      },
    },
    build: {
      type: 'array',
      items: {
        type: 'object',
        additionalProperties: false,
        properties: {
          name: { type: 'string' },
          hidden: { type: 'boolean' },
          description: { type: 'string' },
          inherits: { type: 'array', items: { type: 'string' } },
          configurePreset: { type: 'string' },
          targets: { type: 'array', items: { type: 'string' } },
        },
      },
    },
    error: { type: 'string' },
  },
};

/**
 * 渲染一次运行的文本结果。
 *
 * @param {any} value
 * @returns {string}
 */
function renderOutcome(value) {
  const lines = [];
  lines.push(`${value.ok ? 'OK' : 'FAILED'} — ${value.phase} (exit ${value.exitCode ?? 'n/a'}, ${value.durationMs} ms)`);
  lines.push(`reason: ${value.reason}`);
  lines.push(`command: ${value.command}`);

  const errors = (value.diagnostics ?? []).filter((d) => d.severity === 'error');
  const warnings = (value.diagnostics ?? []).filter((d) => d.severity === 'warning');

  if (errors.length > 0) {
    lines.push('', `errors (${errors.length}):`);
    for (const d of errors.slice(0, 40)) {
      const loc = d.file ? `${d.file}${d.line ? `:${d.line}` : ''}` : '';
      lines.push(`  [${d.code ?? 'error'}] ${loc}${loc ? ' ' : ''}${d.message}`);
    }
    if (errors.length > 40) lines.push(`  … ${errors.length - 40} more error(s)`);
  }
  if (warnings.length > 0) {
    lines.push('', `warnings (${warnings.length}) — first 10:`);
    for (const d of warnings.slice(0, 10)) {
      const loc = d.file ? `${d.file}${d.line ? `:${d.line}` : ''}` : '';
      lines.push(`  [${d.code ?? 'warning'}] ${loc}${loc ? ' ' : ''}${d.message}`);
    }
  }

  const out = value.output ?? { head: '', tail: '', truncated: false };
  if (out.head) lines.push('', '--- output head ---', out.head);
  if (out.tail) lines.push('', '--- output tail ---', out.tail);
  if (out.truncated) lines.push('', `(output truncated; ${out.totalBytes} bytes total)`);

  return lines.join('\n');
}

/**
 * @param {any} value
 * @returns {string}
 */
function renderPresets(value) {
  if (!value.ok) return `FAILED to read presets: ${value.error ?? 'unknown error'}`;
  const lines = [`presets from: ${(value.sources ?? []).join(', ') || value.presetsDir}`];
  const cfg = value.configure ?? [];
  const bld = value.build ?? [];

  lines.push('', `configure presets (${cfg.length}):`);
  for (const p of cfg) {
    const bits = [p.name];
    if (p.hidden) bits.push('[hidden]');
    if (p.generator) bits.push(`generator=${p.generator}`);
    if (p.binaryDir) bits.push(`binaryDir=${p.binaryDir}`);
    if (p.inherits?.length) bits.push(`inherits=${p.inherits.join('<')}`);
    if (p.description) bits.push(`— ${p.description}`);
    lines.push(`  ${bits.join('  ')}`);
  }
  lines.push('', `build presets (${bld.length}):`);
  for (const p of bld) {
    const bits = [p.name];
    if (p.hidden) bits.push('[hidden]');
    if (p.configurePreset) bits.push(`configurePreset=${p.configurePreset}`);
    if (p.targets?.length) bits.push(`targets=${p.targets.join(',')}`);
    if (p.description) bits.push(`— ${p.description}`);
    lines.push(`  ${bits.join('  ')}`);
  }
  return lines.join('\n');
}

/**
 * @param {unknown} value
 * @param {string} label
 * @returns {string|null} 出错时返回消息
 */
function validateRelativeDir(value, label) {
  if (value === undefined) return null;
  if (!isSafeRelativeDir(value)) {
    return `${label} 非法：必须是 repoRoot 内的相对路径，且不得包含 ".." 或盘符。收到：${JSON.stringify(value)}`;
  }
  return null;
}

/**
 * @param {unknown} value
 * @param {string} label
 * @returns {string|null}
 */
function validatePresetName(value, label) {
  if (value === undefined) return null;
  if (!isSafePresetName(value)) {
    return `${label} 非法：只允许字母、数字、下划线、点和短横线，且不得以 "-" 开头。收到：${JSON.stringify(value)}`;
  }
  return null;
}

/**
 * @param {unknown} value
 * @param {string} label
 * @returns {string|null}
 */
function validateTargets(value, label) {
  if (value === undefined) return null;
  if (!Array.isArray(value)) return `${label} 必须是字符串数组。`;
  for (const t of value) {
    if (!isSafePresetName(t)) {
      return `${label} 含非法目标名：${JSON.stringify(t)}`;
    }
  }
  return null;
}

/**
 * 构造一个「校验失败」的返回值。
 *
 * @param {string} phase
 * @param {string} command
 * @param {string} message
 * @param {{preset?:string, buildDir?:string, repoRoot:string}} target
 * @returns {any}
 */
function invalid(phase, command, message, target) {
  return sanitizeForJson({
    ok: false,
    reason: message,
    exitCode: null,
    durationMs: 0,
    command,
    phase,
    // 丢弃 undefined：schema 用 additionalProperties:false，残留的 undefined 键
    // 会让结果失去无损 JSON 表示，Host 会拒绝整次工具调用。
    target: pruneUndefined(target),
    diagnostics: [{ severity: 'error', code: 'validation/failed', message }],
    output: { head: '', tail: '', truncated: false, totalBytes: 0 },
  });
}

/**
 * 删掉值为 undefined 的键（保持其余键原样）。
 *
 * @param {Record<string, any>} obj
 * @returns {Record<string, any>}
 */
function pruneUndefined(obj) {
  /** @type {Record<string, any>} */
  const out = {};
  for (const [key, value] of Object.entries(obj)) {
    if (value !== undefined) out[key] = value;
  }
  return out;
}

/**
 * 读并合并 CMakePresets.json 与 CMakeUserPresets.json。
 *
 * @param {string} repoRoot
 * @param {typeof import('node:fs/promises')} fsp
 * @returns {Promise<{doc:any, sources:string[], error?:string}>}
 */
async function loadPresetDocs(repoRoot, fsp) {
  const pathMod = await import('node:path');
  const fileNames = ['CMakePresets.json', 'CMakeUserPresets.json'];
  /** @type {any} */
  const merged = { version: undefined, configurePresets: [], buildPresets: [] };
  const sources = [];
  const errors = [];

  for (const fileName of fileNames) {
    const full = pathMod.join(repoRoot, fileName);
    let raw;
    try {
      raw = await fsp.readFile(full, 'utf8');
    } catch (error) {
      if (error?.code === 'ENOENT') continue;
      errors.push(`${fileName}: ${error instanceof Error ? error.message : String(error)}`);
      continue;
    }
    sources.push(full);
    let parsed;
    try {
      parsed = JSON.parse(raw);
    } catch (error) {
      errors.push(`${fileName}: JSON 解析失败 — ${error instanceof Error ? error.message : String(error)}`);
      continue;
    }
    if (typeof merged.version !== 'number' && typeof parsed.version === 'number') {
      merged.version = parsed.version;
    }
    for (const field of ['configurePresets', 'buildPresets']) {
      if (Array.isArray(parsed[field])) merged[field].push(...parsed[field]);
    }
  }

  if (sources.length === 0 && errors.length === 0) {
    return { doc: merged, sources, error: `未找到 CMakePresets.json（repoRoot=${repoRoot}）` };
  }
  return { doc: merged, sources, error: errors.length > 0 ? errors.join('; ') : undefined };
}

/**
 * 插件入口。
 *
 * @param {any} ctx
 * @param {any} config
 */
export function apply(ctx, config) {
  const repoRoot = config.repoRoot;
  const setEnvScriptAbs = `${repoRoot.replace(/[\\/]+$/, '')}\\${config.setEnvScript.replace(/\//g, '\\')}`;

  /** 动态 import 是插件硬约束：顶层不得出现 node: 内置模块。 */
  let fspPromise;
  const getFs = () => {
    if (!fspPromise) fspPromise = import('node:fs/promises');
    return fspPromise;
  };
  const getPath = () => import('node:path');

  /**
   * @param {number} requested
   * @param {number} fallback
   * @returns {number}
   */
  function resolveTimeout(requested, fallback) {
    const value = typeof requested === 'number' && Number.isFinite(requested) ? Math.trunc(requested) : fallback;
    return Math.min(Math.max(value, 1), 2147483647);
  }

  /** @param {any} exec @param {number} timeoutMs */
  function timeoutSignal(exec, timeoutMs) {
    return AbortSignal.any([exec.signal, AbortSignal.timeout(timeoutMs)]);
  }

  const commonTarget = () => ({ repoRoot, preset: undefined, buildDir: undefined });

  // ------------------------------------------------------------ cmake_presets

  ctx.effect(() => ctx.tools.register(defineTool({
    name: 'cmake_presets',
    description:
      '读取并汇总项目 CMakePresets.json / CMakeUserPresets.json 中的 configure 与 build preset，' +
      '包含继承链、generator、展开后的 binaryDir 与 targets。只读，不执行任何构建。' +
      '在不确定该用哪个 preset 时先调用它。',
    parameters: {},
    output: {
      schema: PRESETS_SCHEMA,
      render: (_args, value) => [{ type: 'text', text: renderPresets(value) }],
    },
    timeoutMs: 30000,
    isConcurrencySafe: () => true,
    async execute(_args, exec) {
      const fsp = await getFs();
      const { doc, sources, error } = await loadPresetDocs(repoRoot, fsp);
      if (error && sources.length === 0) {
        return sanitizeForJson({ ok: false, repoRoot, presetsDir: repoRoot, sources: [], configure: [], build: [], error });
      }
      const summary = summarizePresets(doc, { sourceDir: repoRoot.replace(/\\/g, '/') }, ['configure', 'build']);
      return sanitizeForJson({
        ok: true,
        repoRoot,
        presetsDir: repoRoot,
        sources,
        configure: (summary.presets.configure ?? []).map((p) => ({
          name: p.name,
          hidden: p.hidden === true,
          description: p.description,
          inherits: p.inherits ?? [],
          generator: p.generator,
          binaryDir: p.binaryDir,
        })),
        build: (summary.presets.build ?? []).map((p) => ({
          name: p.name,
          hidden: p.hidden === true,
          description: p.description,
          inherits: p.inherits ?? [],
          configurePreset: p.configurePreset,
          targets: p.targets,
        })),
        ...(error ? { error } : {}),
      });
    },
  })));

  // ---------------------------------------------------------- cmake_configure

  ctx.effect(() => ctx.tools.register(defineTool({
    name: 'cmake_configure',
    description:
      '经 build_script/build/set_env.ps1 运行 `cmake --preset <preset>`，为项目配置构建目录。' +
      '常用 preset：Ninja_debug（Debug，默认，无 Maya）、Ninja_RelWithDebInfo（Maya 2024）、vs2022。' +
      '不要传 --fresh（会重写 build.ninja 使增量状态归零）。',
    parameters: {
      preset: { type: 'string', required: true, description: 'configure preset 名，例如 Ninja_debug。先用 cmake_presets 查询可用值。' },
      extraArgs: { type: 'array', items: { type: 'string' }, description: '追加到 cmake 命令末尾的额外参数（谨慎使用）。' },
      timeoutMs: { type: 'integer', description: '本次调用的超时毫秒数，默认 600000。' },
    },
    output: {
      schema: OUTCOME_SCHEMA,
      render: (_args, value) => [{ type: 'text', text: renderOutcome(value) }],
    },
    timeoutMs: 660000,
    isConcurrencySafe: () => false,
    async execute(args, exec) {
      const bad = validatePresetName(args.preset, 'preset') ?? validateTargets(args.extraArgs, 'extraArgs');
      const cmakeArgs = ['--preset', args.preset, ...(args.extraArgs ?? [])];
      const command = `cmake ${cmakeArgs.join(' ')}`;
      if (bad) return invalid('configure', command, bad, { repoRoot, preset: args.preset, buildDir: undefined });

      const timeoutMs = resolveTimeout(args.timeoutMs, config.configureTimeoutMs);
      return sanitizeForJson(await runCmake(ctx, {
        phase: 'configure',
        cmakeArgs,
        repoRoot,
        setEnvScript: setEnvScriptAbs,
        cmakeExecutable: config.cmakeExecutable,
        powershellPath: config.powershellPath,
        timeoutMs,
        maxOutputBytes: config.maxOutputBytes,
        outputHeadBytes: config.outputHeadBytes,
        outputTailBytes: config.outputTailBytes,
        signal: timeoutSignal(exec, timeoutMs),
        preset: args.preset,
        buildDir: undefined,
      }));
    },
  })));

  // -------------------------------------------------------------- cmake_build

  ctx.effect(() => ctx.tools.register(defineTool({
    name: 'cmake_build',
    description:
      '经 build_script/build/set_env.ps1 构建项目。两种形态：' +
      '(1) 传 preset —— `cmake --build --preset <preset>`，使用 preset 自带的 targets，' +
      '常用 debug_doodle（doodle_kitsu_supplement + test_main）与 debug_maya（doodle_maya）；' +
      '(2) 传 buildDir —— `cmake --build <dir> --target <targets...>`。' +
      'preset 与 buildDir 二选一，都省略时用默认 build preset。' +
      '默认 --parallel 16：更大并行度会让 cl 抢写 obj 文件而报 Permission denied。',
    parameters: {
      preset: { type: 'string', description: 'build preset 名，例如 debug_doodle。与 buildDir 二选一。' },
      buildDir: { type: 'string', description: '相对 repoRoot 的构建目录，例如 build/Ninja_debug。与 preset 二选一。' },
      targets: { type: 'array', items: { type: 'string' }, description: '要构建的目标；仅在与 buildDir 同用时有意义。' },
      parallel: { type: 'integer', description: '并行度，默认 16。不要显著调大。' },
      cleanFirst: { type: 'boolean', description: '为 true 时先执行 --target clean 再构建，默认 false。' },
      extraArgs: { type: 'array', items: { type: 'string' }, description: '追加到 cmake 命令末尾的额外参数（谨慎使用）。' },
      timeoutMs: { type: 'integer', description: '本次调用的超时毫秒数，默认 3600000。' },
    },
    output: {
      schema: OUTCOME_SCHEMA,
      render: (_args, value) => [{ type: 'text', text: renderOutcome(value) }],
    },
    timeoutMs: 3660000,
    isConcurrencySafe: () => false,
    async execute(args, exec) {
      const problems = [
        validatePresetName(args.preset, 'preset'),
        validateRelativeDir(args.buildDir, 'buildDir'),
        validateTargets(args.targets, 'targets'),
        validateTargets(args.extraArgs, 'extraArgs'),
      ].filter(Boolean);

      if (args.preset !== undefined && args.buildDir !== undefined) {
        problems.push('preset 与 buildDir 不能同时给出，请二选一。');
      }

      const maxParallel = Number.isFinite(config.maxParallel) && config.maxParallel > 0
        ? Math.trunc(config.maxParallel)
        : 16;
      const parallel = typeof args.parallel === 'number' && Number.isFinite(args.parallel) && args.parallel > 0
        ? Math.min(Math.trunc(args.parallel), 256)
        : maxParallel;

      /** @type {string[]} */
      const cmakeArgs = [];
      if (args.preset !== undefined || args.buildDir === undefined) {
        const preset = args.preset ?? config.defaultBuildPreset;
        cmakeArgs.push('--build', '--preset', preset);
      } else {
        // buildDir 已是相对 repoRoot 的完整相对路径（如 build/Ninja_debug），
        // 不能再补 defaultBuildDir 前缀，否则会拼成 build/build/Ninja_debug。
        cmakeArgs.push('--build', args.buildDir.replace(/\\/g, '/').replace(/\/+/g, '/'));
        if (Array.isArray(args.targets) && args.targets.length > 0) {
          cmakeArgs.push('--target', ...args.targets);
        }
      }
      cmakeArgs.push('--parallel', String(parallel));
      if (args.cleanFirst === true) cmakeArgs.push('--clean-first');
      if (Array.isArray(args.extraArgs)) cmakeArgs.push(...args.extraArgs);

      const command = `cmake ${cmakeArgs.join(' ')}`;
      const target = {
        repoRoot,
        preset: args.preset ?? (args.buildDir === undefined ? config.defaultBuildPreset : undefined),
        buildDir: args.buildDir,
      };
      if (problems.length > 0) return invalid('build', command, problems.join(' '), target);

      const timeoutMs = resolveTimeout(args.timeoutMs, config.buildTimeoutMs);
      return sanitizeForJson(await runCmake(ctx, {
        phase: 'build',
        cmakeArgs,
        repoRoot,
        setEnvScript: setEnvScriptAbs,
        cmakeExecutable: config.cmakeExecutable,
        powershellPath: config.powershellPath,
        timeoutMs,
        maxOutputBytes: config.maxOutputBytes,
        outputHeadBytes: config.outputHeadBytes,
        outputTailBytes: config.outputTailBytes,
        signal: timeoutSignal(exec, timeoutMs),
        preset: target.preset,
        buildDir: target.buildDir,
      }));
    },
  })));

  // -------------------------------------------------------------- cmake_clean

  ctx.effect(() => ctx.tools.register(defineTool({
    name: 'cmake_clean',
    description:
      '经 build_script/build/set_env.ps1 运行 `cmake --build <dir> --target clean`，删除指定构建目录里的产物。' +
      '可传 buildDir（相对 repoRoot，例如 build/Ninja_debug），或传 preset 由该 preset 的 binaryDir 推导。' +
      '都不传时清理默认 configure preset 的构建目录。',
    parameters: {
      buildDir: { type: 'string', description: '相对 repoRoot 的构建目录，例如 build/Ninja_debug。' },
      preset: { type: 'string', description: 'configure preset 名，用它的 binaryDir 作为清理目标。与 buildDir 二选一。' },
      timeoutMs: { type: 'integer', description: '本次调用的超时毫秒数，默认 300000。' },
    },
    output: {
      schema: OUTCOME_SCHEMA,
      render: (_args, value) => [{ type: 'text', text: renderOutcome(value) }],
    },
    timeoutMs: 360000,
    isConcurrencySafe: () => false,
    async execute(args, exec) {
      const problems = [
        validateRelativeDir(args.buildDir, 'buildDir'),
        validatePresetName(args.preset, 'preset'),
      ].filter(Boolean);
      if (args.buildDir !== undefined && args.preset !== undefined) {
        problems.push('buildDir 与 preset 不能同时给出，请二选一。');
      }

      let dirArg;
      let resolvedFromPreset;
      if (args.buildDir !== undefined) {
        // 同 cmake_build：buildDir 已含 build/ 前缀，勿再拼 defaultBuildDir。
        dirArg = args.buildDir.replace(/\\/g, '/').replace(/\/+/g, '/');
      } else {
        const configurePreset = args.preset ?? config.defaultConfigurePreset;
        resolvedFromPreset = configurePreset;
        const fsp = await getFs();
        const pathMod = await getPath();
        const { doc, error } = await loadPresetDocs(repoRoot, fsp);
        if (error && !doc?.configurePresets?.length) {
          return invalid('clean', 'cmake --build <dir> --target clean', `无法推导构建目录：${error}`, {
            repoRoot, preset: configurePreset, buildDir: undefined,
          });
        }
        const abs = binaryDirForConfigurePreset(doc, configurePreset, repoRoot.replace(/\\/g, '/'));
        if (!abs) {
          return invalid('clean', 'cmake --build <dir> --target clean',
            `无法从 preset "${configurePreset}" 推导 binaryDir，请显式传 buildDir。`, {
              repoRoot, preset: configurePreset, buildDir: undefined,
            });
        }
        dirArg = pathMod.relative(repoRoot, abs).replace(/\\/g, '/');
      }

      const cmakeArgs = ['--build', dirArg, '--target', 'clean'];
      const command = `cmake ${cmakeArgs.join(' ')}`;
      const target = { repoRoot, preset: resolvedFromPreset, buildDir: dirArg };
      if (problems.length > 0) return invalid('clean', command, problems.join(' '), target);

      const timeoutMs = resolveTimeout(args.timeoutMs, config.cleanTimeoutMs);
      return sanitizeForJson(await runCmake(ctx, {
        phase: 'clean',
        cmakeArgs,
        repoRoot,
        setEnvScript: setEnvScriptAbs,
        cmakeExecutable: config.cmakeExecutable,
        powershellPath: config.powershellPath,
        timeoutMs,
        maxOutputBytes: config.maxOutputBytes,
        outputHeadBytes: config.outputHeadBytes,
        outputTailBytes: config.outputTailBytes,
        signal: timeoutSignal(exec, timeoutMs),
        preset: target.preset,
        buildDir: target.buildDir,
      }));
    },
  })));

  // ------------------------------------------------------------ 提示词指引段

  ctx.effect(() => ctx.systemPrompt.section({
    name: 'cmake',
    order: config.guidanceOrder,
    text: ({ scope }) => {
      const available = ['cmake_presets', 'cmake_configure', 'cmake_build', 'cmake_clean']
        .filter((toolName) => ctx.tools.get(toolName, scope) !== undefined);
      if (available.length === 0) return '';

      return [
        '## CMake 构建（dsh-cmake）',
        '',
        `本仓库位于 ${repoRoot}。所有 CMake 操作都必须经由下列工具，它们通过 ` +
        '`build_script/build/set_env.ps1` 激活 MSVC 环境后调用 cmake：',
        '',
        `- \`cmake_presets\` —— 只读列出可用 preset 与它们的构建目录；不确定用哪个先调它。`,
        `- \`cmake_configure\` —— \`cmake --preset <name>\`。`,
        `- \`cmake_build\` —— \`cmake --build --preset <name>\` 或指定 buildDir + targets。`,
        `- \`cmake_clean\` —— 删除构建产物。`,
        '',
        '规则：',
        '- **不要**手工拼 `-G Ninja -DCMAKE_C_COMPILER=cl`，也**不要**自己激活 VS 环境 —— 会让工具集落回 v143，导致 vcpkg ABI 失效并全量重编。',
        '- **禁止 `--fresh`**：它会重写 build.ninja 使增量状态归零；仅在 CMakeCache 确实损坏时使用。',
        '- **不要使用 release preset**（`Ninja_release` / `Release_*` / `package_release`），它们耗时过长。',
        `- configure preset 常用 \`${config.defaultConfigurePreset}\`（Debug）与 \`Ninja_RelWithDebInfo\`（Maya 2024）；` +
        `build preset 常用 \`${config.defaultBuildPreset}\`（doodle_kitsu_supplement + test_main）与 \`debug_maya\`。`,
        '- 保持 `parallel` 默认值：更高并行度会让 `cl` 抢写 obj 报 `Permission denied: cannot open compiler generated file`。',
        '- 工具的返回值里 `ok` 是权威结论，`diagnostics` 会给出错误对应的文件与行号；失败时优先看 `diagnostics` 而不是翻长输出。',
      ].join('\n');
    },
  }));
}

//#region 插件入口
// 上面是插件主体；本行以下不再有内容（此标记供 test/pure.mjs 定位可切片范围）。
//#endregion
