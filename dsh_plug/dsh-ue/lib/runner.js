// runner.js —— 驱动 UE 的 Engine\Build\BatchFiles\Build.bat 的唯一执行通道。
//
// 设计要点：
//   * Build.bat 是批处理脚本，只能由 cmd.exe 解释；argv 交给 target.js 的
//     buildLaunchArgv 拆成独立项（路径含空格时由宿主逐项加引号）。
//   * cwd 必须是 .uproject 所在目录 —— UBT 用 cwd 找工程，找不到就报
//     "Unable to find project file"。
//   * Build.bat 自带 `%tmp%\<Build.bat 全路径>.lock` 串行锁：两个并发的 UE 构建会互相等待，
//     所以工具声明 isConcurrencySafe=false。
//   * UBT 输出可达数十 MB，用头尾截断后再交给模型。

import {
  decideOutcome,
  dedupeDiagnostics,
  editorLogDiagnostics,
  parseDiagnostics,
  pruneUndefined,
  readUbtSummary,
  summarizeEditorOutput,
  truncateOutput,
} from './parse.js';

/**
 * 读取 SubprocessOutputReader 的全部可得文本。
 *
 * readFrom 是偏移式、非消费式读取，循环直到不再前进。
 *
 * @param {any} reader
 * @returns {{text:string, lossy:boolean, spillPath?:string}}
 */
function drainReader(reader) {
  if (!reader) return { text: '', lossy: false };
  let text = '';
  let offset = 0;
  let lossy = false;
  /** @type {string|undefined} */
  let spillPath;
  for (let guard = 0; guard < 10000; guard += 1) {
    const chunk = reader.readFrom(offset);
    if (!chunk || chunk.nextOffset === offset) {
      if (chunk?.lossy) lossy = true;
      if (chunk?.spillPath) spillPath = chunk.spillPath;
      break;
    }
    text += chunk.text;
    offset = chunk.nextOffset;
    if (chunk.lossy) lossy = true;
    if (chunk.spillPath) spillPath = chunk.spillPath;
  }
  return pruneUndefined({ text, lossy, spillPath });
}

/**
 * 把一个值投影成「确定能无损序列化」的对象：丢掉 undefined 键，并把孤立代理项替换掉。
 *
 * Host 对工具返回值做无损 JSON 校验，一旦出现 undefined 键或孤立代理项，
 * 整次工具调用都会失败（"value is not lossless JSON"）。
 *
 * @param {any} value
 * @returns {any}
 */
export function sanitizeForJson(value) {
  if (typeof value === 'string') return stripLoneSurrogates(value);
  if (Array.isArray(value)) return value.map(sanitizeForJson);
  if (value && typeof value === 'object') {
    /** @type {Record<string, any>} */
    const out = {};
    for (const [key, item] of Object.entries(value)) {
      if (item === undefined) continue;
      out[key] = sanitizeForJson(item);
    }
    return out;
  }
  return value;
}

/**
 * @param {string} value
 * @returns {string}
 */
function stripLoneSurrogates(value) {
  return value.replace(
    /[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/g,
    '\uFFFD',
  );
}

/**
 * 把内部诊断投影成 output schema 允许的形状。
 *
 * @param {any} diagnostic
 * @returns {Record<string, any>}
 */
export function toSchemaDiagnostic(diagnostic) {
  return pruneUndefined({
    severity: diagnostic.severity,
    code: typeof diagnostic.code === 'string' ? diagnostic.code : 'UBT/Error',
    file: typeof diagnostic.file === 'string' ? diagnostic.file : undefined,
    line: Number.isFinite(diagnostic.line) ? diagnostic.line : undefined,
    column: Number.isFinite(diagnostic.column) ? diagnostic.column : undefined,
    message: typeof diagnostic.message === 'string' ? diagnostic.message : String(diagnostic.message ?? ''),
  });
}

/**
 * 执行一次 UE 构建。
 *
 * @param {object} deps
 * @param {{spawn(spec:any):any}} deps.subprocess  ctx.subprocess
 * @param {object} spec
 * @param {string} spec.buildBat             Build.bat 绝对路径
 * @param {string} spec.cwd                  子进程工作目录（.uproject 所在目录）
 * @param {string[]} spec.argv               完整 argv（cmd.exe 形态）
 * @param {string} spec.command              人类可读命令行
 * @param {string} spec.engineRoot
 * @param {string} spec.repoRoot
 * @param {string} spec.target
 * @param {string} spec.platform
 * @param {string} spec.configuration
 * @param {string|undefined} spec.projectFile
 * @param {string|undefined} spec.exePath    编辑器可执行文件（mode=editor 时给出）
 * @param {string} spec.mode                 ubt | script | editor
 * @param {number} spec.timeoutMs
 * @param {number} spec.maxOutputBytes
 * @param {number} spec.outputHeadBytes
 * @param {number} spec.outputTailBytes
 * @param {AbortSignal} spec.signal
 * @returns {Promise<any>}
 */
export async function runUeBuild(deps, spec) {
  const started = Date.now();

  /** @type {Record<string, any>} */
  const base = {
    ok: false,
    reason: '',
    code: 'unknown',
    durationMs: 0,
    exitCode: null,
    command: spec.command,
    ubtResult: undefined,
    ubtCompileFailed: false,
    ubtTotalTimeSeconds: undefined,
    target: pruneUndefined({
      mode: spec.mode,
      target: spec.target,
      platform: spec.platform,
      configuration: spec.configuration,
      projectFile: spec.projectFile,
      cwd: spec.cwd,
      engineRoot: spec.engineRoot,
      repoRoot: spec.repoRoot,
      buildBat: spec.buildBat,
      exePath: spec.exePath,
    }),
    diagnostics: [],
    warnings: [],
    output: { head: '', tail: '', truncated: false, totalBytes: 0 },
    spillPath: undefined,
  };

  for (const arg of [spec.buildBat, spec.cwd ?? '', ...(spec.argv ?? [])]) {
    if (/[\\/]dsh_work[\\/]/i.test(String(arg))) {
      return sanitizeForJson({
        ...base,
        reason: `拒绝执行：命令行里仍残留 dsh_work 路径（stale engineRoot / repoRoot 配置）：${spec.command}`,
        code: 'stale-config',
        durationMs: Date.now() - started,
        diagnostics: [{
          severity: 'error',
          code: 'config/stale',
          message: 'dsh-ue 的 engineRoot / repoRoot 配置仍是旧值（E:\\dsh_work\\…），请在插件配置里改成真实路径。',
        }],
      });
    }
  }

  /** @type {any} */
  let handle;
  try {
    handle = deps.subprocess.spawn({
      argv: spec.argv,
      cwd: spec.cwd,
      env: { DSH_UE_NO_AUTORUN: '1' },
      stdio: {
        stdin: 'ignore',
        stdout: { maxBytes: spec.maxOutputBytes, spill: { maxBytes: spec.maxOutputBytes } },
        stderr: { maxBytes: spec.maxOutputBytes, spill: { maxBytes: spec.maxOutputBytes } },
      },
      graceMs: 15000,
      signal: spec.signal,
    });
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    return sanitizeForJson({
      ...base,
      reason: `failed to spawn cmd.exe: ${message}`,
      code: 'spawn-failed',
      durationMs: Date.now() - started,
      diagnostics: [{ severity: 'error', code: 'spawn/failed', message: `无法启动 Build.bat：${message}` }],
    });
  }

  /** @type {any} */
  let outcome;
  let aborted = false;
  try {
    outcome = await handle.done;
  } catch (error) {
    aborted = spec.signal?.aborted === true;
    try {
      handle.terminate();
    } catch {
      // 进程已退出
    }
    outcome = { exitCode: null, signal: null };
    if (!aborted) {
      const message = error instanceof Error ? error.message : String(error);
      return sanitizeForJson({
        ...base,
        reason: `subprocess failed: ${message}`,
        code: 'subprocess-failed',
        durationMs: Date.now() - started,
        diagnostics: [{ severity: 'error', code: 'subprocess/failed', message }],
      });
    }
  }

  aborted = aborted || spec.signal?.aborted === true;

  const outText = drainReader(handle.collected?.stdout);
  const errText = drainReader(handle.collected?.stderr);
  const combined = [outText.text, errText.text].filter((part) => part.length > 0).join('\n');

  const parsed = dedupeDiagnostics([
    ...parseDiagnostics(combined),
    ...(spec.mode === 'editor' ? editorLogDiagnostics(combined) : []),
  ]);
  const summary = readUbtSummary(combined);
  const errors = parsed.filter((item) => item.severity === 'error');
  const warnings = parsed.filter((item) => item.severity === 'warning');
  const exitCode = typeof outcome?.exitCode === 'number' ? outcome.exitCode : null;
  const editor = spec.mode === 'editor' ? summarizeEditorOutput(combined) : undefined;

  const verdict = decideOutcome({
    mode: spec.mode,
    exitCode,
    aborted,
    result: summary.result,
    resultDetail: summary.detail,
    errors,
  });

  return sanitizeForJson({
    ...base,
    ok: verdict.ok,
    reason: verdict.reason,
    code: verdict.code,
    durationMs: Date.now() - started,
    exitCode,
    ubtResult: summary.result,
    ubtCompileFailed: errors.some((item) => item.code === 'UBT/CompileFailed'),
    ubtTotalTimeSeconds: summary.totalTimeSeconds,
    diagnostics: errors.map(toSchemaDiagnostic).slice(0, 200),
    warnings: warnings.map(toSchemaDiagnostic).slice(0, 100),
    output: {
      ...truncateOutput(combined, spec.outputHeadBytes, spec.outputTailBytes),
      keyLines: editor?.keyLines ?? [],
      keyLineCount: editor?.keyLineCount ?? 0,
      totalLines: editor?.totalLines ?? 0,
    },
    spillPath: outText.spillPath ?? errText.spillPath,
  });
}

/**
 * 校验失败时的统一返回值。
 *
 * @param {string} command
 * @param {string[]} problems
 * @param {Record<string, any>} target
 * @returns {any}
 */
export function invalid(command, problems, target) {
  const message = problems.join(' ');
  return sanitizeForJson({
    ok: false,
    reason: message,
    code: 'validation-failed',
    durationMs: 0,
    exitCode: null,
    command,
    ubtResult: undefined,
    ubtCompileFailed: false,
    ubtTotalTimeSeconds: undefined,
    target: pruneUndefined(target),
    diagnostics: [{ severity: 'error', code: 'validation/failed', message }],
    warnings: [],
    output: { head: '', tail: '', truncated: false, totalBytes: 0 },
    spillPath: undefined,
  });
}
