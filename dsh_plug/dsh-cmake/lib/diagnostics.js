//#region 诊断解析（纯函数，可被 test/pure.mjs 通过源码切片单测）

/**
 * 从 CMake / MSVC / Ninja 输出中提取结构化诊断。
 *
 * 设计约束：这里只做**纯文本解析**，不接触任何 Node 内置模块或 DSH 服务，
 * 以便 test/pure.mjs 用 node:vm 切片求值。
 */

// CMake Error at CMakeLists.txt:12 (target_link_libraries):
const RE_CMAKE_ERROR_AT = /^\s*CMake Error at (.+?):(\d+)(?:\s*\(([^)]*)\))?:/;
// CMake Error: ... （无位置）
const RE_CMAKE_ERROR_BARE = /^\s*CMake Error:\s*(.*)$/;
// CMake Warning at foo/CMakeLists.txt:9 (add_library):
const RE_CMAKE_WARNING_AT = /^\s*CMake Warning at (.+?):(\d+)(?:\s*\(([^)]*)\))?:/;
// CMake Warning: ... / CMake Warning (dev) at ...
const RE_CMAKE_WARNING_BARE = /^\s*CMake Warning(?:\s*\(dev\))?(?:\s+at .*?)?:\s*(.*)$/;
// MSVC: file.cpp(123,45): error C2039: ... / warning C4100
const RE_MSVC = /^\s*(.*?)\((\d+)(?:,\d+)?\)\s*:\s*(fatal error|error|warning)\s+([A-Z]{1,3}\d{3,5})\s*:\s*(.*)$/;
// MSVC alternate: file.cpp(123): error C2039
const RE_MSVC_SHORT = /^\s*(.*?)\((\d+)\)\s*:\s*(fatal error|error|warning)\s+([A-Z]{1,3}\d{3,5})\s*:\s*(.*)$/;
// MSVC link/other: LINK : fatal error LNK1181: cannot open input file
const RE_MSVC_TOOLLESS = /^\s*(?:[A-Z]+)?\s*:\s*(fatal error|error|warning)\s+([A-Z]{1,3}\d{3,5})\s*:\s*(.*)$/;
// ninja
const RE_NINJA_STOPPED = /^\s*ninja:\s*build stopped:\s*subcommand failed/;
const RE_NINJA_FAILED = /^\s*FAILED:\s*(.*)$/;
// clang / gcc style
const RE_GCC = /^\s*(.*?):(\d+):(\d+):\s*(fatal error|error|warning)\s*:\s*(.*)$/;

const SEVERITY_ORDER = { error: 0, warning: 1, info: 2 };

/**
 * @param {string} severity
 * @returns {'error'|'warning'}
 */
function normalizeSeverity(severity) {
  return severity === 'warning' ? 'warning' : 'error';
}

/**
 * 解析一段文本，返回诊断数组。
 *
 * @param {string} text
 * @returns {{severity:'error'|'warning', file?:string, line?:number, code?:string, message:string, raw:string}[]}
 */
export function parseDiagnostics(text) {
  const out = [];
  if (typeof text !== 'string' || text.length === 0) return out;
  const lines = text.split(/\r?\n/);

  for (const raw of lines) {
    const line = raw.trimEnd();
    if (line.length === 0) continue;

    let m;

    if ((m = RE_NINJA_STOPPED.exec(line))) {
      out.push({ severity: 'error', code: 'ninja/build-stopped', message: 'ninja: build stopped: subcommand failed', raw: line });
      continue;
    }

    if ((m = RE_CMAKE_ERROR_AT.exec(line))) {
      out.push({
        severity: 'error',
        file: m[1],
        line: Number(m[2]),
        code: m[3] ? `cmake/${m[3]}` : 'cmake/error',
        message: line.trim(),
        raw: line,
      });
      continue;
    }

    if ((m = RE_CMAKE_WARNING_AT.exec(line))) {
      out.push({
        severity: 'warning',
        file: m[1],
        line: Number(m[2]),
        code: m[3] ? `cmake/${m[3]}` : 'cmake/warning',
        message: line.trim(),
        raw: line,
      });
      continue;
    }

    if ((m = RE_MSVC.exec(line))) {
      const sev = normalizeSeverity(m[3]);
      out.push({ severity: sev, file: m[1], line: Number(m[2]), code: m[4], message: m[5].trim(), raw: line });
      continue;
    }

    if ((m = RE_MSVC_SHORT.exec(line))) {
      const sev = normalizeSeverity(m[3]);
      out.push({ severity: sev, file: m[1], line: Number(m[2]), code: m[4], message: m[5].trim(), raw: line });
      continue;
    }

    if ((m = RE_GCC.exec(line))) {
      const sev = normalizeSeverity(m[4]);
      out.push({ severity: sev, file: m[1], line: Number(m[2]), code: sev === 'error' ? 'cc/error' : 'cc/warning', message: m[5].trim(), raw: line });
      continue;
    }

    if ((m = RE_CMAKE_ERROR_BARE.exec(line))) {
      out.push({ severity: 'error', code: 'cmake/error', message: m[1].trim(), raw: line });
      continue;
    }

    if ((m = RE_CMAKE_WARNING_BARE.exec(line))) {
      out.push({ severity: 'warning', code: 'cmake/warning', message: m[1].trim(), raw: line });
      continue;
    }

    if ((m = RE_NINJA_FAILED.exec(line))) {
      out.push({ severity: 'error', code: 'ninja/failed', message: m[1].trim(), raw: line });
      continue;
    }

    if ((m = RE_MSVC_TOOLLESS.exec(line))) {
      const sev = normalizeSeverity(m[1]);
      out.push({ severity: sev, code: m[2], message: m[3].trim(), raw: line });
      continue;
    }
  }

  return out;
}

/**
 * 去掉重复诊断（同一 file:line:code:message 重复出现的情况在并行编译里很常见）。
 *
 * @param {ReturnType<typeof parseDiagnostics>} diagnostics
 * @returns {ReturnType<typeof parseDiagnostics>}
 */
export function dedupeDiagnostics(diagnostics) {
  const seen = new Set();
  const out = [];
  for (const d of diagnostics) {
    const key = `${d.severity}\u0000${d.file ?? ''}\u0000${d.line ?? ''}\u0000${d.code ?? ''}\u0000${d.message}`;
    if (seen.has(key)) continue;
    seen.add(key);
    out.push(d);
  }
  return out;
}

/**
 * 决定某次运行是否成功。
 *
 * @param {number|null} exitCode
 * @param {ReturnType<typeof parseDiagnostics>} diagnostics
 * @param {{aborted?: boolean}} [extra]
 * @returns {{ok:boolean, reason:string}}
 */
export function decideOutcome(exitCode, diagnostics, extra = {}) {
  if (extra.aborted) {
    return { ok: false, reason: 'aborted' };
  }

  const errors = diagnostics.filter((d) => d.severity === 'error');
  const nonzero = exitCode !== 0 && exitCode !== null;

  if (errors.length > 0 && nonzero) {
    return { ok: false, reason: `exit code ${exitCode} with ${errors.length} error diagnostic(s)` };
  }
  if (nonzero) {
    return { ok: false, reason: `exit code ${exitCode} (no error diagnostic parsed from output)` };
  }
  if (errors.length > 0) {
    // 退出码 0 但输出里有 error 行：CMake 有时在 warning-as-error 或子脚本里这么干，保守判失败。
    return { ok: false, reason: `${errors.length} error diagnostic(s) in output despite exit code 0` };
  }
  if (exitCode === null) {
    return { ok: false, reason: 'exit code unavailable' };
  }
  return { ok: true, reason: 'exit code 0 and no error diagnostics' };
}

/**
 * 头尾截断。保留开头 headBytes 与结尾 tailBytes，中间丢弃。
 *
 * @param {string} text
 * @param {number} headBytes
 * @param {number} tailBytes
 * @returns {{head:string, tail:string, truncated:boolean, totalBytes:number}}
 */
export function truncateOutput(text, headBytes = 4096, tailBytes = 12288) {
  const s = typeof text === 'string' ? text : '';
  const totalBytes = utf8ByteLength(s);
  if (totalBytes <= headBytes + tailBytes) {
    return { head: s, tail: '', truncated: false, totalBytes };
  }
  return {
    head: sliceByBytes(s, headBytes, 'head'),
    tail: sliceByBytes(s, tailBytes, 'tail'),
    truncated: true,
    totalBytes,
  };
}

/**
 * 按 UTF-8 字节预算截取字符串，且绝不切开代理对。
 *
 * 两点必须同时成立：
 *   1. 原始实现用 `s.slice(0, headBytes)` 把「字节数」当作「UTF-16 码元数」用，
 *      预算与实际占用不是一回事；这里按真实字节累积。
 *   2. 更关键的是**不能切在代理对中间**。cmake/vcpkg 输出里有大量非 BMP 字符，
 *      切出的孤立代理项（如 `"\ud83d"`）不是合法的 Unicode 标量值，
 *      JSON 序列化后无法无损还原，Host 会以
 *      "value is not lossless JSON" 拒绝整次工具调用。
 *
 * @param {string} s
 * @param {number} budgetBytes
 * @param {'head'|'tail'} side
 * @returns {string}
 */
function sliceByBytes(s, budgetBytes, side) {
  const budget = Number.isFinite(budgetBytes) && budgetBytes > 0 ? Math.trunc(budgetBytes) : 0;
  if (budget === 0) return '';

  // 用 Array.from 按码点切分，保证每个元素都是一个完整码点（不会拿到半个代理对）。
  const codePoints = Array.from(s);
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

  for (let i = codePoints.length - 1; i >= 0; i -= 1) {
    const cp = codePoints[i];
    const size = utf8ByteLength(cp);
    if (used + size > budget) break;
    picked.push(cp);
    used += size;
  }
  picked.reverse();
  return picked.join('');
}

/**
 * UTF-8 字节长度（不依赖 Buffer，便于在 node:vm 里求值）。
 *
 * @param {string} s
 * @returns {number}
 */
export function utf8ByteLength(s) {
  let n = 0;
  for (const ch of s) {
    const cp = ch.codePointAt(0);
    n += cp <= 0x7f ? 1 : cp <= 0x7ff ? 2 : cp <= 0xffff ? 3 : 4;
  }
  return n;
}

/**
 * 校验 preset / target 名是否安全（只允许 CMake 合法标识符字符）。
 *
 * 额外拒绝以 `-` 开头的名字：这类名字会被 cmake 当成选项解析，可能绕过
 * `--preset <name>` 的位置语义。
 *
 * @param {unknown} name
 * @returns {boolean}
 */
export function isSafePresetName(name) {
  if (typeof name !== 'string' || name.length === 0 || name.length > 200) return false;
  if (name.startsWith('-')) return false;
  return /^[A-Za-z0-9_.\-]+$/.test(name);
}

/**
 * 校验 buildDir：必须是相对路径、不许逃逸、不许绝对路径。
 *
 * @param {unknown} dir
 * @returns {boolean}
 */
export function isSafeRelativeDir(dir) {
  if (typeof dir !== 'string' || dir.length === 0 || dir.length > 400) return false;
  if (dir.includes('\u0000')) return false;
  // 绝对路径（含盘符、UNC、前导斜杠）
  if (/^[A-Za-z]:/.test(dir)) return false;
  if (dir.startsWith('\\\\') || dir.startsWith('//')) return false;
  if (dir.startsWith('/') || dir.startsWith('\\')) return false;
  const parts = dir.split(/[\\/]+/);
  for (const p of parts) {
    if (p === '' ) continue;
    if (p === '.' ) continue;
    if (p === '..') return false;
  }
  return true;
}

/**
 * 展开 preset 里的变量：${sourceDir} / ${presetName} / ${fileDir} 等。
 *
 * @param {string} value
 * @param {{sourceDir:string, presetName:string, fileDir?:string}} vars
 * @returns {string}
 */
export function expandPresetVars(value, vars) {
  if (typeof value !== 'string') return value;
  return value.replace(/\$\{(\w+)\}/g, (whole, key) => {
    if (key === 'sourceDir') return vars.sourceDir ?? whole;
    if (key === 'presetName') return vars.presetName ?? whole;
    if (key === 'fileDir') return vars.fileDir ?? vars.sourceDir ?? whole;
    return whole;
  });
}

//#endregion

//#region 插件入口
// 下面这行以下的内容依赖 @deepseek-ai/* 与 DSH 服务，不被纯函数单测加载。
//#endregion
