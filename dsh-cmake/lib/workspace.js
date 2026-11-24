//#region 工作区与构建目录解析（纯函数，可被 test/pure.mjs 通过源码切片单测）

/**
 * 会话工作区根目录 → 构建目录 → 分支的解析。
 *
 * 这里刻意只做纯计算与字符串处理：不 import 任何模块、不碰 ctx、不做 await，
 * 文件系统探测与 git 调用由调用方（lib/index.js）注入。
 *
 * 路径分隔符统一用 WINDOWS_SEP / FORWARD_SEP 两个常量表达，避免反斜杠字面量。
 */

const WINDOWS_SEP = String.fromCharCode(92);

const FORWARD_SEP = String.fromCharCode(47);

/**
 * 统一成反斜杠、去掉末尾分隔符。
 *
 * @param {unknown} value
 * @returns {string}
 */
function isSeparator(ch) {
  return ch.charCodeAt(0) === 92 || ch.charCodeAt(0) === 47;
}

function charAtOr(text, index) {
  return index < 0 || index >= text.length ? "" : text.charAt(index);
}

export function normalizeRoot(value) {
  if (typeof value !== 'string') return '';
  const text = value.trim();
  let out = "";
  for (let index = 0; index < text.length; index += 1) {
    const ch = charAtOr(text, index);
    if (isSeparator(ch)) {
      out = out + WINDOWS_SEP;
    } else {
      out = out + ch;
    }
  }
  let end = out.length;
  while (end > 1) {
    if (!isSeparator(charAtOr(out, end - 1))) break;
    end -= 1;
  }
  return out.slice(0, end);
}

/**
 * 取路径的父目录；已经是盘符根时返回空串。
 *
 * @param {string} value
 * @returns {string}
 */
export function parentOf(value) {
  const normalized = normalizeRoot(value);
  let cut = -1;
  for (let index = 0; index < normalized.length; index += 1) {
    if (isSeparator(charAtOr(normalized, index))) cut = index;
  }
  if (cut <= 2) return charAtOr(normalized, 0) + charAtOr(normalized, 1);
  let out = "";
  for (let index = 0; index < cut; index += 1) {
    const ch = charAtOr(normalized, index);
    if (isSeparator(ch)) {
      out = out + WINDOWS_SEP;
    } else {
      out = out + ch;
    }
  }
  return out + WINDOWS_SEP;
}

/**
 * 按优先级挑出可用的会话工作区根目录。
 *
 * 顺序是「会话当前目录 → 会话创建时的项目根 → 部署兜底目录」：当前目录会随
 * workingDirectory 工具切换而变化，必须先于创建时的项目根。
 *
 * @param {{workingDirectory?:string, sessionCwd?:string, defaultDirectory?:string}} spec
 * @returns {string}
 */
export function chooseWorkspaceRoot({ workingDirectory, sessionCwd, defaultDirectory }) {
  for (const candidate of [workingDirectory, sessionCwd, defaultDirectory]) {
    const normalized = normalizeRoot(candidate);
    if (normalized !== '') return normalized;
  }
  return '';
}

/**
 * 从会话对象上读出候选根目录，不依赖任何服务。
 *
 * 解析器把 session 放在 `session`、`agent.session` 或 `scope`（ToolRunContext 的
 * agent 与 prompt 组装上下文里 scope 就是 agent）三处之一，这里统一扫一遍。
 *
 * @param {any} subject
 * @returns {string} 会话创建时记录的项目根；读不到返回空串
 */
export function sessionCwdOf(subject) {
  if (subject === null || typeof subject !== 'object') return '';
  const session = subject.session ?? subject;
  const header = session !== null && typeof session === 'object' ? session.header : undefined;
  const cwd = header !== null && typeof header === 'object' ? header.cwd : undefined;
  return normalizeRoot(cwd);
}

/**
 * CMakePresets.json 里 `binaryDir` 表达式的等价实现：
 * `${sourceParentDir}/build/$env{DOODLE_GIT_BRANCH}/${presetName}`。
 *
 * 两处必须保持一致，否则 agent 会去一个不存在的目录里找产物。
 *
 * @param {{sourceDir:string, branch:string, presetName:string}} spec
 * @returns {string}
 */
export function buildDirOf({ sourceDir, branch, presetName }) {
  const parent = parentOf(sourceDir);
  const branchPart = branch === '' ? '' : `${branch}/`;
  return `${parent}/build/${branchPart}${presetName}`;
}

/**
 * 归一化 `git rev-parse --abbrev-ref HEAD` 的输出。
 *
 * 游离 HEAD 时输出 `HEAD`，与 set_env.ps1 一致地退化成短提交号（由调用方补上）。
 *
 * @param {unknown} text
 * @returns {{kind:'branch'|'detached'|'unknown', name:string}}
 */
export function parseGitBranch(text) {
  const value = typeof text === 'string' ? text.trim() : '';
  if (value === '') return { kind: 'unknown', name: '' };
  if (value === 'HEAD' || value.startsWith('HEAD ')) return { kind: 'detached', name: '' };
  return { kind: 'branch', name: value };
}

/**
 * 把一段 JSON 字符串解析成对象；不是对象时返回 undefined。
 *
 * @param {unknown} text
 * @returns {Record<string, any>|undefined}
 */
export function parseJsonObject(text) {
  if (typeof text !== 'string' || text.trim() === '') return undefined;
  let parsed;
  try {
    parsed = JSON.parse(text);
  } catch {
    return undefined;
  }
  if (parsed === null || typeof parsed !== 'object' || Array.isArray(parsed)) return undefined;
  return parsed;
}

/**
 * 读 CMakePresets.json 里某个 configure preset 的 binaryDir 表达式（含继承链）。
 *
 * 只做继承链上的字段覆盖：CMake 的 cacheVariables 是深度合并，binaryDir 是整字段覆盖。
 *
 * @param {Record<string, any>|undefined} doc
 * @param {string} presetName
 * @returns {string}
 */
export function presetBinaryDirExpression(doc, presetName) {
  if (doc === undefined) return '';
  const list = Array.isArray(doc.configurePresets) ? doc.configurePresets : [];
  const byName = new Map();
  for (const preset of list) {
    if (preset !== null && typeof preset === 'object' && typeof preset.name === 'string') byName.set(preset.name, preset);
  }
  const seen = new Set();
  /** @type {any[]} */
  const chain = [];
  let current = presetName;
  while (typeof current === 'string' && current !== '' && !seen.has(current)) {
    seen.add(current);
    const node = byName.get(current);
    if (node === undefined) break;
    chain.push(node);
    current = typeof node.inherits === 'string' ? node.inherits : '';
  }
  let expression = '';
  for (let index = chain.length - 1; index >= 0; index -= 1) {
    const value = chain[index].binaryDir;
    if (typeof value === 'string' && value !== '') expression = value;
  }
  return expression;
}

/**
 * 按预设的 binaryDir 表达式算出构建目录期望值（用于和 CMakeCache 比对）。
 *
 * @param {string} expression
 * @param {{sourceDir:string, presetName:string, branch:string}} vars
 * @returns {string}
 */
export function expandBinaryDirExpression(expression, { sourceDir, presetName, branch }) {
  if (typeof expression !== 'string' || expression === '') return '';
  const source = normalizeRoot(sourceDir).split(WINDOWS_SEP).join(FORWARD_SEP);
  const value = expression.replace(/\$\{(\w+)\}|\$env\{(\w+)\}/g, (whole, plain, env) => {
    if (plain === 'sourceDir') return source;
    if (plain === 'sourceParentDir') return parentOf(source);
    if (plain === 'presetName') return presetName;
    if (env === 'DOODLE_GIT_BRANCH') return branch;
    return whole;
  });
  let end = value.length;
  while (end > 0 && (value.charAt(end - 1) === FORWARD_SEP || value.charAt(end - 1) === WINDOWS_SEP)) end -= 1;
  return value.slice(0, end);
}

/**
 * 从 CMakeCache.txt 的内文里取 `CMAKE_HOME_DIRECTORY`。
 *
 * 只读我们自己关心的这一行，不引入完整 cache 解析器。
 *
 * @param {unknown} cacheText
 * @returns {string} 该构建目录已经绑定的源码根；读不到返回空串
 */
export function cacheHomeDirectory(cacheText) {
  if (typeof cacheText !== 'string') return '';
  const prefix = 'CMAKE_HOME_DIRECTORY:';
  for (const line of cacheText.split(/\r?\n/)) {
    if (!line.startsWith(prefix)) continue;
    const index = line.indexOf('=');
    if (index < 0) return '';
    return normalizeRoot(line.slice(index + 1));
  }
  return '';
}

/**
 * 判断一个已存在的构建目录是否仍然属于当前会话的工作区。
 *
 * @param {{sourceDir:string, buildDir:string, cacheHomeDirectory:string}} spec
 * @returns {'ok'|'empty'|'foreign-source'}
 */
export function buildDirOwnership({ sourceDir, buildDir, cacheHomeDirectory }) {
  if (normalizeRoot(cacheHomeDirectory) === '') return 'empty';
  return normalizeRoot(cacheHomeDirectory).toLowerCase() === normalizeRoot(sourceDir).toLowerCase()
    ? 'ok'
    : 'foreign-source';
}

/**
 * 把某个 configure preset 的 binaryDir 表达式展开成绝对构建目录。
 *
 * @param {Record<string, any>|undefined} doc CMakePresets.json 的解析结果
 * @param {{sourceDir:string, presetName:string, branch:string}} vars
 * @returns {string}
 */
export function resolveBuildDir(doc, { sourceDir, presetName, branch }) {
  const expression = presetBinaryDirExpression(doc, presetName);
  if (expression === '') return '';
  return expandBinaryDirExpression(expression, { sourceDir, presetName, branch });
}

//#endregion

//#region 插件入口
// 工作区解析全部是纯函数，没有需要宿主资源的实现部分；该标记供 test/pure.mjs 定位可切片范围。
//#endregion
