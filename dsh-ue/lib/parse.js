//#region UE 输出解析（纯函数，可被 test/pure.mjs 通过源码切片单测）
//
// 这里只做纯文本解析：不 import 任何 Node 内置模块，也不接触 DSH 服务，
// 以便 test/pure.mjs 用 node:vm 切片求值。

// MSVC: d:\p\foo.cpp(123,45): error C2039: 'x': is not a member of 'y'
//       d:\p\foo.cpp(123,45): warning C4100: ...
//       d:\p\foo.cpp(123,45): note: see declaration of 'y'   （note 没有错误码）
//       LINK : fatal error LNK1181: ...
const RE_MSVC = /^\s*([^:][^()]*?)\((\d+)(?:,(\d+))?\)\s*:\s*(fatal error|error|warning|note)\s*:?\s*(?:([A-Z]{1,3}\d{3,5})\s*:\s*)?(.*)$/;
// clang / 部分工具链: foo.cpp:12:3: error: message
const RE_GCC = /^\s*(.*?):(\d+):(\d+):\s*(fatal error|error|warning|note)\s*:\s*(.*)$/;

// UBT 的致命错误块（无文件名、无行号）：
//   LogCompile     : error: <c++ 文件名>
//   LogCompile     : 一个或多个模块编译失败（X 个）。详见日志。
//   Log                : error: <UHT / 其它工具链错误>
//   LogBuildException  : error: 生成 <target> 时发生异常
const RE_UBT_ERROR = /^\s*(Log[A-Za-z]+)\s*:\s*error\s*:\s*(.*)$/;
// 只取 `LogXxx :` 头部，剩下的部分再判断：中文摘要行没有 `error:` 前缀
const RE_UBT_HEAD = /^\s*(Log[A-Za-z]+)\s*:\s*/;
// LogXxx: Error: message
const RE_UBT_SEVERITY = /^\s*(Log[A-Za-z]+)\s*:\s*(Error|Fatal|Warning)\s*:\s*(.*)$/i;
const RE_UBT_WARNING = /^\s*Log[A-Za-z]+\s*:\s*Warning\s*:\s*(.*)$/i;
// dotnet / MSBuild: ... : error MSB1008: ...  /  ... : warning MSB...
const RE_MSBUILD = /\b(error|warning)\s+([A-Z]{1,4}\d{3,5})\s*:\s*(.*)$/;

const RE_UBT_RESULT = /^\s*Result:\s*(Succeeded|Failed)\b(.*)$/;
const RE_UBT_TOTAL_TIME = /^\s*Total execution time:\s*([\d.]+)\s*seconds\.?/;
const RE_UBT_FATAL = /一个或多个模块编译失败|one or more modules failed to compile/i;
const RE_STACK_FRAME = /^\s*at\s+[\w.<>+`]+\s*\(/;

export const UBT_COMPILE_FAILED = 'UBT/CompileFailed';
export const UBT_TOOL_ERROR = 'UBT/Error';
export const UBT_EXCEPTION = 'UBT/Exception';

const LINK_HINT = /link\.exe|LNK\d{3,5}|unresolved external/i;

const NOISE_HINTS = [
  /\bwarning\b/i,
  /\bninja\b/i,
  /\bbuilding\b/i,
  /\bcompiling\b/i,
  /\blinking\b/i,
  /^\[/,
  /^\s*\d+\s*>/,
];

/**
 * @param {string} text
 * @returns {string[]}
 */
function splitLines(text) {
  if (typeof text !== 'string' || text.length === 0) return [];
  return text.split(/\r?\n/);
}

/**
 * 删掉值为 undefined 的键（工具 output schema 声明了 additionalProperties:false）。
 *
 * @param {Record<string, any>} value
 * @returns {Record<string, any>}
 */
export function pruneUndefined(value) {
  /** @type {Record<string, any>} */
  const out = {};
  for (const [key, item] of Object.entries(value)) {
    if (item !== undefined) out[key] = item;
  }
  return out;
}

/**
 * @param {string} severity
 * @returns {'error'|'warning'|'note'}
 */
function normalizeSeverity(severity) {
  const value = String(severity).toLowerCase();
  if (value === 'warning') return 'warning';
  if (value === 'note') return 'note';
  return 'error';
}

/**
 * @param {Record<string, any>} value
 * @returns {Record<string, any>}
 */
function pruneDiagnostic(value) {
  return pruneUndefined({
    severity: value.severity,
    code: String(value.code ?? UBT_TOOL_ERROR),
    file: typeof value.file === 'string' && value.file.length > 0 ? value.file : undefined,
    line: Number.isFinite(value.line) && value.line > 0 ? value.line : undefined,
    column: Number.isFinite(value.column) && value.column > 0 ? value.column : undefined,
    message: typeof value.message === 'string' ? value.message : '',
    raw: typeof value.raw === 'string' ? value.raw : undefined,
  });
}

/**
 * 解析单行诊断。
 *
 * @param {string} line 已 trimEnd 的一行
 * @param {string} nextLine 下一行原文：UBT 的 `LogCompile : error:` 后通常跟一行真正的错误说明
 * @returns {any|undefined}
 */
function parseOneLine(line, nextLine) {
  if (line.trim().length === 0) return undefined;

  let match = RE_MSVC.exec(line);
  if (match) {
    const severity = normalizeSeverity(match[4]);
    return pruneDiagnostic({
      severity,
      file: match[1].trim(),
      line: Number(match[2]),
      column: match[3] === undefined ? undefined : Number(match[3]),
      code: severity === 'note' ? 'MSVC/note' : (match[5] ?? 'MSVC/Diagnostic'),
      message: match[6].trim(),
      raw: line,
    });
  }

  match = RE_GCC.exec(line);
  if (match) {
    const severity = normalizeSeverity(match[4]);
    return pruneDiagnostic({
      severity,
      file: match[1],
      line: Number(match[2]),
      column: Number(match[3]),
      code: severity === 'note' ? 'Clang/note' : 'Clang/Diagnostic',
      message: match[5].trim(),
      raw: line,
    });
  }

  match = RE_UBT_ERROR.exec(line);
  if (match) {
    const detail = match[2].trim();
    const next = typeof nextLine === 'string' ? nextLine.trim() : '';
    const nextIsUseful = next.length > 0
      && !RE_STACK_FRAME.test(next)
      && !NOISE_HINTS.some((re) => re.test(next))
      && !RE_UBT_ERROR.test(next);
    const message = nextIsUseful && !detail.includes(next) ? `${detail} — ${next}` : detail;
    const isCompile = RE_UBT_FATAL.test(message);
    const isException = /Exception/i.test(match[1]);
    return pruneDiagnostic({
      severity: 'error',
      code: isCompile ? UBT_COMPILE_FAILED : (isException ? UBT_EXCEPTION : UBT_TOOL_ERROR),
      message: `${match[1]}: ${message}`,
      raw: line,
    });
  }

  // UBT 的中文摘要行没有 `error:` 前缀（`LogCompile : 一个或多个模块编译失败（N 个）`），
  // 只能在剥离 `LogXxx :` 头部之后判断。
  match = RE_UBT_HEAD.exec(line);
  if (match) {
    const tail = line.slice(match[0].length).trim();
    const isException = /Exception/i.test(match[1]) || /Exception/i.test(tail);
    if (tail.length > 0 && RE_UBT_FATAL.test(tail)) {
      return pruneDiagnostic({
        severity: 'error',
        code: UBT_COMPILE_FAILED,
        message: `${match[1]}: ${tail}`,
        raw: line,
      });
    }
    if (tail.length > 0 && (isException || /^(error|fatal)\b/i.test(tail))) {
      return pruneDiagnostic({
        severity: 'error',
        code: isException ? UBT_EXCEPTION : UBT_TOOL_ERROR,
        message: `${match[1]}: ${tail}`,
        raw: line,
      });
    }
  }

  match = RE_UBT_SEVERITY.exec(line);
  if (match) {
    const level = match[2].toLowerCase();
    if (level === 'warning') {
      return { severity: 'warning', code: 'UBT/Warning', message: match[3].trim(), raw: line };
    }
    const isException = /Exception/i.test(match[1]) || /Exception/i.test(match[3]);
    return pruneDiagnostic({
      severity: 'error',
      code: isException ? UBT_EXCEPTION : 'UBT/Error',
      message: match[3].trim(),
      raw: line,
    });
  }

  match = RE_MSBUILD.exec(line);
  if (match) {
    return {
      severity: match[1].toLowerCase() === 'warning' ? 'warning' : 'error',
      code: match[2],
      message: match[3].trim(),
      raw: line,
    };
  }

  match = RE_UBT_WARNING.exec(line);
  if (match) {
    return { severity: 'warning', code: 'UBT/Warning', message: match[1].trim(), raw: line };
  }

  return undefined;
}

/**
 * 解析一段 UBT / MSVC / dotnet 输出，返回结构化诊断。
 *
 * @param {string} text
 * @returns {any[]}
 */
export function parseDiagnostics(text) {
  /** @type {any[]} */
  const out = [];
  const lines = splitLines(text);
  for (let index = 0; index < lines.length; index += 1) {
    const parsed = parseOneLine(lines[index].trimEnd(), lines[index + 1]);
    if (parsed !== undefined) out.push(parsed);
  }
  return out;
}

/**
 * 去掉重复诊断：并行编译时同一条诊断会从不同通道（stdout / stderr、UBT 重放）重复出现。
 *
 * @param {any[]} diagnostics
 * @returns {any[]}
 */
export function dedupeDiagnostics(diagnostics) {
  const seen = new Set();
  /** @type {any[]} */
  const out = [];
  for (const item of diagnostics) {
    const key = JSON.stringify([
      item.severity,
      item.code,
      item.file ?? '',
      item.line ?? '',
      item.column ?? '',
      item.message,
    ]);
    if (seen.has(key)) continue;
    seen.add(key);
    out.push(item);
  }
  return out;
}

/**
 * 从 UBT 输出里读出 `Result:` 行与总耗时。
 *
 * @param {string} text
 * @returns {{result?:'Succeeded'|'Failed', detail?:string, totalTimeSeconds?:number}}
 */
export function readUbtSummary(text) {
  let result;
  let detail;
  let totalTimeSeconds;
  for (const line of splitLines(text)) {
    const handled = RE_UBT_RESULT.exec(line);
    if (handled) {
      result = handled[1] === 'Succeeded' ? 'Succeeded' : 'Failed';
      const rest = handled[2].trim().replace(/^\(/, '').replace(/\)$/, '');
      detail = rest.length > 0 ? rest : undefined;
    }
    const elapsed = RE_UBT_TOTAL_TIME.exec(line);
    if (elapsed) totalTimeSeconds = Number(elapsed[1]);
  }
  return pruneUndefined({ result, detail, totalTimeSeconds });
}

/**
 * 把 error 级诊断分成编译错误 / 链接错误 / 其它。
 *
 * 注意：带 `C2039` 这类 MSVC 错误码的诊断是编译错误，即使同一行里出现了 `LINK` 字样。
 *
 * @param {any[]} errors
 * @returns {{compileErrors:any[], linkErrors:any[], otherErrors:any[]}}
 */
export function classifyErrors(errors) {
  /** @type {any[]} */
  const compileErrors = [];
  /** @type {any[]} */
  const linkErrors = [];
  /** @type {any[]} */
  const otherErrors = [];
  for (const item of errors) {
    const code = typeof item.code === 'string' ? item.code : '';
    if (code === UBT_COMPILE_FAILED || /^C\d{3,5}$/.test(code)) {
      compileErrors.push(item);
      continue;
    }
    const haystack = `${item.code} ${item.message} ${item.raw ?? ''}`;
    if (LINK_HINT.test(haystack)) {
      linkErrors.push(item);
      continue;
    }
    otherErrors.push(item);
  }
  return { compileErrors, linkErrors, otherErrors };
}

/**
 * 把一次 UE 构建的「事实」收敛成结论。
 *
 * 判定顺序（任何一条命中都算失败）：
 *   1. 被取消（abort / 超时）—— 失败，且不猜测 UBT 自身状态。
 *   2. 存在 error 级诊断。
 *   3. UBT 明确输出 `Result: Failed`（UBT 自己判失败，例如目标缺失、模块依赖错误）。
 *   4. 进程退出码非 0（UE 的 Build.bat 用 999 表示脚本自身失败）。
 *   5. 退出码 0 但 UBT 输出 `Result: Succeeded` 才算成功。
 *
 * @param {{exitCode:number|null, aborted:boolean, result?:string, resultDetail?:string, errors:any[]}} facts
 * @returns {{ok:boolean, reason:string, code:string}}
 */
export function decideOutcome(facts) {
  const exitCode = typeof facts.exitCode === 'number' ? facts.exitCode : null;
  const errors = Array.isArray(facts.errors) ? facts.errors : [];
  const result = typeof facts.result === 'string' ? facts.result : undefined;

  if (facts.aborted) {
    return { ok: false, reason: 'cancelled (aborted or timed out)', code: 'cancelled' };
  }

  if (errors.length > 0) {
    const { compileErrors, linkErrors, otherErrors } = classifyErrors(errors);
    const parts = [];
    if (compileErrors.length > 0) parts.push(`${compileErrors.length} compile error(s)`);
    if (linkErrors.length > 0) parts.push(`${linkErrors.length} link error(s)`);
    if (otherErrors.length > 0) parts.push(`${otherErrors.length} other error(s)`);
    const suffix = exitCode === null ? ' (exit code unavailable)' : ` — exit code ${exitCode}`;
    return {
      ok: false,
      reason: parts.join(', ') + suffix,
      code: compileErrors.length > 0 ? 'compile-failed' : linkErrors.length > 0 ? 'link-failed' : 'error-diagnostics',
    };
  }

  if (result === 'Failed') {
    const detail = facts.resultDetail ? ` (${facts.resultDetail})` : '';
    const suffix = exitCode === null ? '' : ` — exit code ${exitCode}`;
    return { ok: false, reason: `UnrealBuildTool reported Result: Failed${detail}${suffix}`, code: 'ubt-failed' };
  }

  if (result === 'Succeeded' && exitCode !== null && exitCode !== 0) {
    return {
      ok: false,
      reason: `UnrealBuildTool reported Result: Succeeded but the process exited with ${exitCode}`,
      code: 'inconsistent',
    };
  }

  if (exitCode === null) {
    return { ok: false, reason: 'exit code unavailable and no UBT result line found', code: 'unknown' };
  }

  if (exitCode !== 0) {
    const hint = exitCode === 999
      ? '（999 是 Build.bat 脚本自身的失败：缺少 dotnet SDK / UBT 编译失败 / batch 位置不对，看 output 头部的 ERROR 行）'
      : '';
    return {
      ok: false,
      reason: `exit code ${exitCode}${hint} and no error diagnostic parsed from output`,
      code: 'exit-code',
    };
  }

  if (result === 'Succeeded') {
    return { ok: true, reason: 'exit code 0, UBT Result: Succeeded, no error diagnostics', code: 'ok' };
  }

  return {
    ok: false,
    reason: 'exit code 0 but no `Result:` line in UBT output — UBT 可能没跑起来或输出被过滤，看 output 确认',
    code: 'no-result',
  };
}

/**
 * UTF-8 字节长度（不依赖 Buffer，便于在 node:vm 里求值）。
 *
 * @param {string} value
 * @returns {number}
 */
export function utf8ByteLength(value) {
  let size = 0;
  for (const ch of String(value)) {
    const cp = ch.codePointAt(0) ?? 0;
    size += cp <= 0x7f ? 1 : cp <= 0x7ff ? 2 : cp <= 0xffff ? 3 : 4;
  }
  return size;
}

/**
 * 按 UTF-8 字节预算截取，且绝不切开代理对。
 *
 * @param {string} value
 * @param {number} budgetBytes
 * @param {'head'|'tail'} side
 * @returns {string}
 */
function sliceByBytes(value, budgetBytes, side) {
  const budget = Number.isFinite(budgetBytes) && budgetBytes > 0 ? Math.trunc(budgetBytes) : 0;
  if (budget === 0) return '';
  const codePoints = Array.from(String(value));
  /** @type {string[]} */
  const picked = [];
  let used = 0;
  if (side === 'head') {
    for (const cp of codePoints) {
      const size = utf8ByteLength(cp);
      if (used + size > budget) break;
      picked.push(cp);
      used += size;
    }
    return picked.join('');
  }
  for (let index = codePoints.length - 1; index >= 0; index -= 1) {
    const cp = codePoints[index];
    const size = utf8ByteLength(cp);
    if (used + size > budget) break;
    picked.push(cp);
    used += size;
  }
  picked.reverse();
  return picked.join('');
}

/**
 * 头尾截断。
 *
 * @param {string} text
 * @param {number} headBytes
 * @param {number} tailBytes
 * @returns {{head:string, tail:string, truncated:boolean, totalBytes:number}}
 */
export function truncateOutput(text, headBytes = 4096, tailBytes = 12288) {
  const value = typeof text === 'string' ? text : '';
  const totalBytes = utf8ByteLength(value);
  if (totalBytes <= headBytes + tailBytes) {
    return { head: value, tail: '', truncated: false, totalBytes };
  }
  return {
    head: sliceByBytes(value, headBytes, 'head'),
    tail: sliceByBytes(value, tailBytes, 'tail'),
    truncated: true,
    totalBytes,
  };
}

//#endregion
