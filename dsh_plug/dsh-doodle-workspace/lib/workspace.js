// dsh-doodle-workspace —— Doodle 辅助插件共用的工作区/构建目录解析
//
// 这个包不是 DSH 插件：它没有 apply()、不注册工具、不读 ctx。
// 它只回答两个问题，并且只回答一次，供 dsh-cmake 与 dsh-kitsu-supplement 同时使用：
//   1) 发起会话的工作区根目录是哪一个；
//   2) 这个根目录对应的 git 分支与构建目录（<上一级>/build/<分支>/<preset>）怎么算。
//
// 约定：路径一律只做字符串处理，分隔符用字符码判断（92 / 47），不用反斜杠字面量与正则转义。
// 原因之一是 dsh-kitsu-supplement 的 test/pure.mjs 用 node:vm 按源码文本求值，那种求值方式下
// 字符串里的 `\d` 会退化成 `d`，反斜杠正则同样不可靠。
//
// 第二道防线是路径归一化：把分隔符全部折成 "/" 之后再拼接，出口处再折回 "\"。
// 这样即使某层字符串被求值方式削掉了反斜杠，也不会把路径拼成一坨。

export const WINDOWS_SEP = String.fromCharCode(92);

export const FORWARD_SEP = String.fromCharCode(47);

export const DEFAULT_TEST_EXE_NAME = "doodle_kitsu_supplement_test.exe";

export const DEFAULT_PRODUCTION_EXE_NAME = "doodle_kitsu_supplement.exe";

export const EXE_NAMES = [DEFAULT_TEST_EXE_NAME, DEFAULT_PRODUCTION_EXE_NAME];

export const KITSU_PRESET_DIR = "Ninja_debug";

export const BUILD_BIN_RELATIVE = "bin";

export const EXE_STOP_NOTICE =
  "解析不出 doodle_kitsu_supplement 的可执行文件路径，已停止本次操作：不要再尝试其他 exe 路径或其它构建目录，把所有细节交给用户检查。";

export const WORKSPACE_STOP_NOTICE =
  "解析不出当前会话的工作区根目录，已停止本次操作：不要改传别的路径重试，把所有细节交给用户检查。";

/**
 * @param {number} code
 * @returns {boolean}
 */
export function isSeparatorCode(code) {
  return code === 92 || code === 47;
}

/**
 * @param {unknown} value
 * @returns {string}
 */
export function trimText(value) {
  return typeof value === "string" ? value.trim() : "";
}

/**
 * 把任意分隔符形态折成 "/"。
 *
 * @param {unknown} value
 * @returns {string}
 */
export function toForwardPath(value) {
  const text = trimText(value);
  if (text === "") return "";
  const chars = [];
  for (let index = 0; index < text.length; index += 1) {
    const code = text.charCodeAt(index);
    chars.push(isSeparatorCode(code) ? FORWARD_SEP : text.charAt(index));
  }
  return chars.join("");
}

/**
 * 把 "/" 折回 "\"。
 *
 * @param {unknown} value
 * @returns {string}
 */
export function toWindowsPath(value) {
  const text = trimText(value);
  if (text === "") return "";
  const chars = [];
  for (let index = 0; index < text.length; index += 1) {
    const code = text.charCodeAt(index);
    chars.push(isSeparatorCode(code) ? WINDOWS_SEP : text.charAt(index));
  }
  return chars.join("");
}

/**
 * 统一成反斜杠、去掉末尾分隔符。
 *
 * @param {unknown} value
 * @returns {string}
 */
export function normalizeRoot(value) {
  const forward = toForwardPath(value);
  let end = forward.length;
  while (end > 1 && forward.charAt(end - 1) === FORWARD_SEP) end -= 1;
  if (end < forward.length && end > 2) return toWindowsPath(forward.slice(0, end));
  return toWindowsPath(forward);
}

/**
 * 按 "/" 切段，保留空段（UNC 前缀会产生前导空段）。
 *
 * @param {unknown} value
 * @returns {string[]}
 */
export function splitPathParts(value) {
  const forward = toForwardPath(value);
  if (forward === "") return [];
  return forward.split(FORWARD_SEP);
}

/**
 * 按 Windows 路径拼接；已有分隔符时不重复加。
 *
 * @param {Array<unknown>} segments
 * @returns {string}
 */
export function joinWindowsPath(segments) {
  const parts = [];
  for (const segment of segments ?? []) {
    let text = toForwardPath(segment);
    while (text.length > 0 && text.charAt(text.length - 1) === FORWARD_SEP) text = text.slice(0, -1);
    while (text.length > 0 && text.charAt(0) === FORWARD_SEP) text = text.slice(1);
    if (text !== "") parts.push(text);
  }
  if (parts.length === 0) return "";
  return toWindowsPath(parts.join(FORWARD_SEP));
}

/**
 * 取路径的上一级；盘符根返回空串。
 *
 * @param {unknown} value
 * @returns {string}
 */
export function parentOf(value) {
  const forward = toForwardPath(value);
  let end = forward.length;
  while (end > 0 && forward.charAt(end - 1) === FORWARD_SEP) end -= 1;
  const trimmed = forward.slice(0, end);
  let cut = -1;
  for (let index = 0; index < trimmed.length; index += 1) {
    if (trimmed.charAt(index) === FORWARD_SEP) cut = index;
  }
  if (cut < 0) return "";
  if (cut === 2 && trimmed.charAt(1) === ":") return trimmed.charAt(0) + ":";
  return toWindowsPath(trimmed.slice(0, cut));
}

/**
 * 路径是否是绝对路径。
 *
 * @param {unknown} value
 * @returns {boolean}
 */
export function isAbsolutePath(value) {
  const forward = toForwardPath(value);
  if (forward.length < 2) return false;
  const first = forward.charCodeAt(0);
  const alpha = (first >= 65 && first <= 90) || (first >= 97 && first <= 122);
  if (alpha && forward.charAt(1) === ":" && forward.charAt(2) === FORWARD_SEP) return true;
  return forward.charAt(0) === FORWARD_SEP && forward.charAt(1) === FORWARD_SEP;
}

/**
 * 选会话工作区根目录。
 *
 * 顺序是「会话当前目录 → 会话创建时的项目根 → 部署兜底目录」：当前目录会随
 * workingDirectory 工具切换而变化，必须先于创建时的项目根。
 *
 * @param {{workingDirectory?:unknown, sessionCwd?:unknown, defaultDirectory?:unknown}} spec
 * @returns {string}
 */
export function chooseWorkspaceRoot({ workingDirectory, sessionCwd, defaultDirectory }) {
  for (const candidate of [workingDirectory, sessionCwd, defaultDirectory]) {
    const normalized = normalizeRoot(candidate);
    if (normalized !== "") return normalized;
  }
  return "";
}

/**
 * 从会话（或 agent）对象上读出创建时记录的 cwd。
 *
 * @param {any} subject
 * @returns {string}
 */
export function sessionCwdOf(subject) {
  if (subject === null || typeof subject !== "object") return "";
  const session = subject.session ?? subject;
  const header = session !== null && typeof session === "object" ? session.header : undefined;
  const cwd = header !== null && typeof header === "object" ? header.cwd : undefined;
  return normalizeRoot(cwd);
}

/**
 * 归一化 `git rev-parse --abbrev-ref HEAD` 的输出。
 *
 * @param {unknown} text
 * @returns {{kind:'branch'|'detached'|'unknown', name:string}}
 */
export function parseGitBranch(text) {
  const value = trimText(text);
  if (value === "") return { kind: "unknown", name: "" };
  if (value === "HEAD" || value.startsWith("HEAD ")) return { kind: "detached", name: "" };
  return { kind: "branch", name: value };
}

/**
 * 按 CMakePresets.json 的表达方式拼构建目录：`<工作区上一级>/build/<分支>/<preset>`。
 *
 * @param {{sourceDir?:unknown, workspaceRoot?:unknown, branch?:unknown, presetName?:unknown}} spec
 * @returns {string}
 */
export function buildDirOf({ sourceDir, workspaceRoot, branch, presetName }) {
  const parent = parentOf(sourceDir ?? workspaceRoot);
  const name = trimText(branch);
  const preset = trimText(presetName);
  if (parent === "" || name === "" || preset === "") return "";
  return joinWindowsPath([parent, "build", name, preset]);
}

/**
 * 按构建目录拼 exe 全路径。
 *
 * @param {{buildDir?:unknown, exeName?:unknown}} spec
 * @returns {string}
 */
export function exePathOf({ buildDir, exeName }) {
  const dir = normalizeRoot(buildDir);
  const name = trimText(exeName);
  if (dir === "" || name === "") return "";
  return joinWindowsPath([dir, BUILD_BIN_RELATIVE, name]);
}

/**
 * 本次要跑哪个 exe：只认文件名，不接受路径。
 *
 * @param {{exeName?:unknown}} args
 * @returns {{name:string, problem?:string}}
 */
export function resolveExeName(args) {
  const requested = trimText(args?.exeName);
  if (requested === "") return { name: DEFAULT_TEST_EXE_NAME };
  const lower = requested.toLowerCase();
  let hasSeparator = false;
  for (let index = 0; index < lower.length; index += 1) {
    if (isSeparatorCode(lower.charCodeAt(index))) hasSeparator = true;
  }
  if (hasSeparator || lower.indexOf(":") >= 0) {
    return {
      name: "",
      problem: `exeName 只接受文件名，不接受路径：${JSON.stringify(requested)}。要换构建请改会话工作区或重新 cmake_configure，不要传路径。`,
    };
  }
  if (EXE_NAMES.indexOf(lower) < 0) {
    return {
      name: "",
      problem: `exeName 只能是 ${EXE_NAMES.join(" 或 ")}，收到 ${JSON.stringify(requested)}。`,
    };
  }
  return { name: lower };
}

/**
 * 把一次 exe 解析的输入整理成可判定的结论。
 *
 * @param {{workspaceRoot?:unknown, branch?:unknown, exeName?:unknown, buildDir?:unknown, exePath?:unknown, exists?:boolean}} spec
 * @returns {{ok:boolean, problems:string[], facts:string[]}}
 */
export function judgeExeResolution({ workspaceRoot, branch, exeName, buildDir, exePath, exists }) {
  const facts = [];
  const problems = [];
  if (workspaceRoot !== "") facts.push(`会话工作区：${workspaceRoot}`);
  if (branch !== "") facts.push(`git 分支：${branch}`);
  if (buildDir !== "") facts.push(`构建目录：${buildDir}`);
  if (workspaceRoot === "" || workspaceRoot === undefined) {
    problems.push("无法确定会话的工作区根目录（workingDirectory 服务的会话目录、会话 header.cwd、部署默认目录都没有给出）。");
  }
  if (branch === "" || branch === undefined) {
    problems.push(
      workspaceRoot === "" || workspaceRoot === undefined
        ? "无法确定 git 分支。"
        : `无法确定 ${workspaceRoot} 的 git 分支（DOODLE_GIT_BRANCH 环境变量与 git rev-parse 都拿不到）。`,
    );
  }
  if (problems.length === 0 && (exePath === "" || exePath === undefined)) {
    problems.push(`无法由工作区、分支与 exeName=${exeName} 推出可执行文件路径。`);
  }
  if (problems.length === 0 && exists !== true) {
    problems.push(`可执行文件不存在：${exePath}。该分支还没有编译过，请先 cmake_configure + cmake_build 再重试。`);
  }
  return { ok: problems.length === 0, problems, facts };
}

/**
 * 在候选目录里找装着 @playwright/test 的那一个。
 *
 * 顺序是「会话工作区 → 兜底目录」：兜底目录只在会话工作区还没装依赖时使用。
 *
 * @param {{workspaceRoot?:unknown, fallbackDirs?:unknown[], hasDependency?:(dir:string)=>boolean}} spec
 * @returns {{cwd:string, lookedAt:string[]}}
 */
export function choosePlaywrightCwd({ workspaceRoot, fallbackDirs, hasDependency }) {
  const lookedAt = [];
  const candidates = [];
  for (const candidate of [workspaceRoot, ...(fallbackDirs ?? [])]) {
    const dir = normalizeRoot(candidate);
    if (dir === "" || candidates.indexOf(dir) >= 0) continue;
    candidates.push(dir);
  }
  for (const dir of candidates) {
    lookedAt.push(dir);
    if (typeof hasDependency === "function" && hasDependency(dir)) return { cwd: dir, lookedAt };
  }
  return { cwd: "", lookedAt };
}

/**
 * playwright 工作目录解析失败时的定型提示。
 *
 * @param {{workspaceRoot?:unknown, lookedAt?:unknown[]}} spec
 * @returns {string}
 */
export function playwrightCwdFailure({ workspaceRoot, lookedAt }) {
  const place = workspaceRoot === "" || workspaceRoot === undefined ? "（无法确定会话工作区根目录）" : workspaceRoot;
  const searched = (lookedAt ?? []).length === 0 ? "（没有可检查的目录）" : lookedAt.join("、");
  return (
    "找不到装着 @playwright/test 的 test_js 目录，已停止本次测试：" +
    `会话工作区 ${place} 与兜底目录里都没有 node_modules/@playwright/test（已查：${searched}）。` +
    "不要改传 cwd 到别处，请用户检查该工作区的 test_js 依赖是否安装。"
  );
}

/**
 * 把一段 JSON 字符串解析成对象；不是对象时返回 undefined。
 *
 * @param {unknown} text
 * @returns {Record<string, any>|undefined}
 */
export function parseJsonObject(text) {
  const value = trimText(text);
  if (value === "") return undefined;
  let parsed;
  try {
    parsed = JSON.parse(value);
  } catch {
    return undefined;
  }
  if (parsed === null || typeof parsed !== "object" || Array.isArray(parsed)) return undefined;
  return parsed;
}

/**
 * 读 CMakePresets.json 里某个 configure preset 的 binaryDir 表达式（含继承链）。
 *
 * @param {Record<string, any>|undefined} doc
 * @param {unknown} presetName
 * @returns {string}
 */
export function presetBinaryDirExpression(doc, presetName) {
  if (doc === undefined || doc === null) return "";
  const list = Array.isArray(doc.configurePresets) ? doc.configurePresets : [];
  const byName = new Map();
  for (const preset of list) {
    if (preset !== null && typeof preset === "object" && typeof preset.name === "string") byName.set(preset.name, preset);
  }
  const seen = new Set();
  const chain = [];
  let current = trimText(presetName);
  while (current !== "" && !seen.has(current)) {
    seen.add(current);
    const node = byName.get(current);
    if (node === undefined) break;
    chain.push(node);
    current = typeof node.inherits === "string" ? node.inherits : "";
  }
  let expression = "";
  for (let index = chain.length - 1; index >= 0; index -= 1) {
    const value = chain[index].binaryDir;
    if (typeof value === "string" && value !== "") expression = value;
  }
  return expression;
}

/**
 * 展开 preset 的 binaryDir 表达式。
 *
 * 支持 ${sourceDir} / ${sourceParentDir} / ${presetName} 与 $env{DOODLE_GIT_BRANCH}。
 *
 * @param {string} expression
 * @param {{sourceDir?:unknown, presetName?:unknown, branch?:unknown}} vars
 * @returns {string}
 */
export function expandBinaryDirExpression(expression, { sourceDir, presetName, branch }) {
  const text = trimText(expression);
  if (text === "") return "";
  const source = toForwardPath(sourceDir).replace(/\/+$/, "");
  const value = text.replace(/\$\{(\w+)\}|\$env\{(\w+)\}/g, (whole, plain, env) => {
    if (plain === "sourceDir") return source;
    if (plain === "sourceParentDir") return parentOf(source);
    if (plain === "presetName") return trimText(presetName);
    if (env === "DOODLE_GIT_BRANCH") return trimText(branch);
    return whole;
  });
  return toForwardPath(value).replace(/\/+$/, "");
}

/**
 * 把某个 configure preset 的 binaryDir 展开成绝对构建目录。
 *
 * @param {Record<string, any>|undefined} doc
 * @param {{sourceDir?:unknown, presetName?:unknown, branch?:unknown}} vars
 * @returns {string}
 */
export function resolveBuildDir(doc, vars) {
  const expression = presetBinaryDirExpression(doc, vars?.presetName);
  if (expression === "") return "";
  return expandBinaryDirExpression(expression, vars);
}

/**
 * 从 CMakeCache.txt 的内文里取 `CMAKE_HOME_DIRECTORY`。
 *
 * @param {unknown} cacheText
 * @returns {string}
 */
export function cacheHomeDirectory(cacheText) {
  if (typeof cacheText !== "string") return "";
  const prefix = "CMAKE_HOME_DIRECTORY:";
  for (const line of cacheText.split("\n")) {
    if (line.indexOf(prefix) !== 0) continue;
    const index = line.indexOf("=");
    if (index < 0) return "";
    return normalizeRoot(line.slice(index + 1));
  }
  return "";
}

/**
 * 判断一个已存在的构建目录是否仍属于当前会话的工作区。
 *
 * @param {{sourceDir?:unknown, buildDir?:unknown, cacheHomeDirectory?:unknown}} spec
 * @returns {'ok'|'empty'|'foreign-source'}
 */
export function buildDirOwnership({ sourceDir, buildDir, cacheHomeDirectory }) {
  const home = normalizeRoot(cacheHomeDirectory);
  if (home === "") return "empty";
  return home.toLowerCase() === normalizeRoot(sourceDir).toLowerCase() ? "ok" : "foreign-source";
}
