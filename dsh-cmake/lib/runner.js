// runner.js —— 通过 set_env.ps1 驱动 cmake 的唯一执行通道。
//
// 设计要点：
//   * 一律经 `<repoRoot>/build_script/build/set_env.ps1` 包装调用 cmake；
//     直接调 cmake 会缺 vcvars 环境，工具链不可用。
//   * set_env.ps1 已改造为透传子进程退出码（见 README），因此退出码可信。
//   * 输出可能达数十 MB，用头尾截断后再交给模型。

import { parseDiagnostics, dedupeDiagnostics, decideOutcome, truncateOutput } from './diagnostics.js';

/**
 * 把内部诊断对象投影成 output schema 允许的形状。
 *
 * parseDiagnostics 保留了一个 `raw` 字段（原始行），供去重与人读使用；但
 * OUTCOME_SCHEMA 只声明 severity/file/line/code/message 且 additionalProperties:false，
 * 多出来的键会让结果失去无损 JSON 表示。这里做一次显式投影。
 *
 * @param {Record<string, any>} d
 * @returns {Record<string, any>}
 */
function toSchemaDiagnostic(d) {
  return pruneUndefined({
    severity: d.severity,
    file: typeof d.file === 'string' ? d.file : undefined,
    line: typeof d.line === 'number' && Number.isFinite(d.line) ? d.line : undefined,
    code: typeof d.code === 'string' ? d.code : undefined,
    message: typeof d.message === 'string' ? d.message : String(d.message ?? ''),
  });
}

/**
 * 删掉值为 undefined 的键。
 *
 * 工具的 output schema 声明了 `additionalProperties: false`，而 value 中一个
 * 值为 undefined 的键在 JSON 序列化后既不属于声明过的类型、也无法无损表示，
 * 会被 Host 判为 “value is not lossless JSON”。可选字段必须真正缺省。
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
 * 组装一次 cmake 调用的 argv。
 *
 * @param {{powershellPath:string, setEnvScript:string, cmakeExecutable:string, cmakeArgs:string[]}} spec
 * @returns {string[]}
 */
export function buildInvocation(spec) {
  return [
    spec.powershellPath,
    '-NoProfile',
    '-ExecutionPolicy',
    'Bypass',
    '-File',
    spec.setEnvScript,
    spec.cmakeExecutable,
    ...spec.cmakeArgs,
  ];
}

/**
 * 把 argv 渲染成人类可复制的一行命令。
 *
 * @param {string[]} argv
 * @returns {string}
 */
export function renderCommandLine(argv) {
  return argv
    .map((part) => (/[\s"]/.test(part) ? `"${part.replace(/"/g, '\\"')}"` : part))
    .join(' ');
}

/**
 * 读取 SubprocessOutputReader 的全部可得文本。
 *
 * @param {{readFrom(fromByte:number):{text:string,nextOffset:number,lossy:boolean,spillPath?:string}}|undefined} reader
 * @returns {{text:string, lossy:boolean, spillPath?:string}}
 */
function drainReader(reader) {
  if (!reader) return { text: '', lossy: false };
  let text = '';
  let offset = 0;
  let lossy = false;
  let spillPath;
  // readFrom 是偏移式、非消费式读取，循环直到不再前进。
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
  return { text, lossy, spillPath };
}

/**
 * 执行一次 cmake 调用。
 *
 * @param {object} deps
 * @param {{spawn(spec:any):any}} deps.subprocess  ctx.subprocess
 * @param {object} spec
 * @param {string} spec.phase            configure | build | clean | presets
 * @param {string[]} spec.cmakeArgs      传给 cmake 的参数（不含可执行文件本身）
 * @param {string} spec.repoRoot
 * @param {string} spec.setEnvScript     绝对路径
 * @param {string} spec.cmakeExecutable
 * @param {string} spec.powershellPath
 * @param {number} spec.timeoutMs
 * @param {number} spec.maxOutputBytes
 * @param {number} spec.outputHeadBytes
 * @param {number} spec.outputTailBytes
 * @param {AbortSignal} spec.signal
 * @param {string|undefined} spec.preset
 * @param {string|undefined} spec.buildDir
 * @returns {Promise<object>}
 */
export async function runCmake(deps, spec) {
  const started = Date.now();
  const argv = buildInvocation(spec);
  const command = renderCommandLine(argv);

  /** @type {any} */
  const base = {
    ok: false,
    exitCode: null,
    durationMs: 0,
    command,
    phase: spec.phase,
    // 只保留有值的字段：schema 里这些键声明为 string 且 additionalProperties:false，
    // 留下 undefined 会让结果无法序列化为 lossless JSON。
    target: pruneUndefined({
      repoRoot: spec.repoRoot,
      preset: spec.preset,
      buildDir: spec.buildDir,
    }),
    diagnostics: [],
    output: { head: '', tail: '', truncated: false, totalBytes: 0 },
    reason: '',
  };

  /** @type {any} */
  let handle;
  try {
    handle = deps.subprocess.spawn({
      argv,
      cwd: spec.repoRoot,
      stdio: {
        stdin: 'ignore',
        stdout: { maxBytes: spec.maxOutputBytes, spill: { maxBytes: spec.maxOutputBytes } },
        stderr: { maxBytes: spec.maxOutputBytes, spill: { maxBytes: spec.maxOutputBytes } },
      },
      graceMs: 5000,
      signal: spec.signal,
    });
  } catch (error) {
    return {
      ...base,
      durationMs: Date.now() - started,
      reason: `failed to spawn: ${error instanceof Error ? error.message : String(error)}`,
      diagnostics: [{
        severity: 'error',
        code: 'spawn/failed',
        message: `无法启动 set_env.ps1：${error instanceof Error ? error.message : String(error)}`,
      }],
    };
  }

  /** @type {any} */
  let outcome;
  let aborted = false;
  try {
    outcome = await handle.done;
  } catch (error) {
    aborted = spec.signal?.aborted === true;
    try { handle.terminate(); } catch { /* 已退出 */ }
    outcome = { exitCode: null, signal: null };
    if (!aborted) {
      return {
        ...base,
        durationMs: Date.now() - started,
        reason: `subprocess failed: ${error instanceof Error ? error.message : String(error)}`,
        diagnostics: [{
          severity: 'error',
          code: 'subprocess/failed',
          message: error instanceof Error ? error.message : String(error),
        }],
      };
    }
  }

  const outText = drainReader(handle.collected?.stdout);
  const errText = drainReader(handle.collected?.stderr);
  const combined = [outText.text, errText.text].filter((t) => t.length > 0).join('\n');

  const diagnostics = dedupeDiagnostics(parseDiagnostics(combined)).map(toSchemaDiagnostic);
  const exitCode = typeof outcome?.exitCode === 'number' ? outcome.exitCode : null;
  const verdict = decideOutcome(exitCode, diagnostics, { aborted });

  return {
    ...base,
    ok: verdict.ok,
    exitCode,
    reason: verdict.reason,
    durationMs: Date.now() - started,
    diagnostics,
    output: truncateOutput(combined, spec.outputHeadBytes, spec.outputTailBytes),
  };
}

/**
 * 把一个值投影成「确定能无损序列化」的对象：丢掉 undefined 键，并把任何
 * 非法 Unicode（孤立代理项）替换掉。
 *
 * 为什么需要：Host 对工具返回值做无损 JSON 校验，一旦值里含有 undefined 键或
 * 孤立代理项，整次工具调用都会失败（"value is not lossless JSON"），
 * 而且**即使工具本身已经成功执行**也会丢结果。这是最后一道防线。
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
    for (const [k, v] of Object.entries(value)) {
      if (v === undefined) continue;
      out[k] = sanitizeForJson(v);
    }
    return out;
  }
  return value;
}

/**
 * 把孤立代理项替换为 U+FFFD，使字符串成为合法 Unicode 序列。
 *
 * @param {string} s
 * @returns {string}
 */
function stripLoneSurrogates(s) {
  return s.replace(
    /[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/g,
    '\uFFFD',
  );
}
