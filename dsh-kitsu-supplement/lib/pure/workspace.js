// dsh-kitsu-supplement —— 会话工作区、构建目录与可执行文件解析
//
// 本文件只放纯逻辑：不依赖任何 import，也不访问 ctx / await 宿主资源。
// 文件系统探测、git 调用与子进程都由调用方注入。
//
// 路径处理只用字符码判断分隔符：本文件会被 test/pure.mjs 用 node:vm 按源码文本求值，
// 反斜杠字面量在那种求值方式下不可靠。

export const WINDOWS_SEP = String.fromCharCode(92);

export const FORWARD_SEP = String.fromCharCode(47);

export const DEFAULT_TEST_EXE_NAME = "doodle_kitsu_supplement_test.exe";

export const DEFAULT_PRODUCTION_EXE_NAME = "doodle_kitsu_supplement.exe";

export const EXE_NAMES = [DEFAULT_TEST_EXE_NAME, DEFAULT_PRODUCTION_EXE_NAME];

export const BUILD_PRESET_DIR = "Ninja_debug";

export const BUILD_BIN_RELATIVE = "bin";

export const EXE_STOP_NOTICE =
  "解析不出 doodle_kitsu_supplement 的可执行文件路径，已停止本次操作：不要再尝试其他 exe 路径或其它构建目录，把所有细节交给用户检查。";

export function isSeparatorCode(code) {
  return code === 92 || code === 47;
}

export function splitPathParts(text) {
  const parts = [];
  let start = 0;
  for (let cursor = 0; cursor <= text.length; cursor += 1) {
    const atEnd = cursor === text.length;
    const code = atEnd ? 0 : text.charCodeAt(cursor);
    if (!atEnd && !isSeparatorCode(code)) continue;
    parts.push(text.slice(start, cursor));
    start = cursor + 1;
  }
  return parts;
}

export function toWindowsSeparators(text) {
  const chars = [];
  for (let cursor = 0; cursor < text.length; cursor += 1) {
    const code = text.charCodeAt(cursor);
    chars.push(isSeparatorCode(code) ? WINDOWS_SEP : text.charAt(cursor));
  }
  return chars.join("");
}

export function joinWindowsPath(segments) {
  const parts = [];
  for (const segment of segments ?? []) {
    const text = typeof segment === "string" ? segment : "";
    if (text === "") continue;
    parts.push(text);
  }
  if (parts.length === 0) return "";
  let out = parts[0];
  for (let index = 1; index < parts.length; index += 1) {
    let last = -1;
    if (out.length > 0) last = out.charCodeAt(out.length - 1);
    if (!isSeparatorCode(last)) out += WINDOWS_SEP;
    out += parts[index];
  }
  return out;
}

export function normalizeRoot(value) {
  if (typeof value !== "string") return "";
  const parts = splitPathParts(value.trim());
  while (parts.length > 1 && parts[parts.length - 1] === "") parts.pop();
  if (parts.length === 0) return "";
  const joined = parts.join(FORWARD_SEP);
  if (joined === "") return "";
  return toWindowsSeparators(joined);
}

export function parentOf(value) {
  const normalized = normalizeRoot(value);
  if (normalized === "") return "";
  const parts = splitPathParts(normalized);
  if (parts.length <= 1) return "";
  if (parts.length === 2) return normalized.charAt(0) + normalized.charAt(1) + WINDOWS_SEP;
  const parent = parts.slice(0, parts.length - 1);
  let head = parent[0];
  for (let index = 1; index < parent.length; index += 1) {
    head = head + WINDOWS_SEP + parent[index];
  }
  return head + WINDOWS_SEP;
}

export function isAbsolutePath(value) {
  const text = normalizeRoot(value);
  if (text.length < 2) return false;
  const first = text.charCodeAt(0);
  const alpha = (first >= 65 && first <= 90) || (first >= 97 && first <= 122);
  if (alpha && text.charAt(1) === ":" && isSeparatorCode(text.charCodeAt(2))) return true;
  return isSeparatorCode(first) && isSeparatorCode(text.charCodeAt(1));
}

export function chooseWorkspaceRoot({ workingDirectory, sessionCwd, defaultDirectory }) {
  for (const candidate of [workingDirectory, sessionCwd, defaultDirectory]) {
    const normalized = normalizeRoot(candidate);
    if (normalized !== "") return normalized;
  }
  return "";
}

export function sessionCwdOf(subject) {
  if (subject === null || typeof subject !== "object") return "";
  const session = subject.session ?? subject;
  const header = session !== null && typeof session === "object" ? session.header : undefined;
  const cwd = header !== null && typeof header === "object" ? header.cwd : undefined;
  return normalizeRoot(cwd);
}

export function parseGitBranch(text) {
  const value = typeof text === "string" ? text.trim() : "";
  if (value === "") return { kind: "unknown", name: "" };
  if (value === "HEAD" || value.startsWith("HEAD ")) return { kind: "detached", name: "" };
  return { kind: "branch", name: value };
}

export function buildDirOf({ workspaceRoot, branch, presetName }) {
  const parent = parentOf(workspaceRoot);
  if (parent === "" || branch === "" || presetName === "") return "";
  return joinWindowsPath([parent, "build", branch, presetName]);
}

export function exePathOf({ buildDir, exeName }) {
  const dir = normalizeRoot(buildDir);
  if (dir === "" || exeName === "") return "";
  return joinWindowsPath([dir, BUILD_BIN_RELATIVE, exeName]);
}

export function resolveExeName(args) {
  const requested = typeof args?.exeName === "string" ? args.exeName.trim() : "";
  if (requested === "") return { name: DEFAULT_TEST_EXE_NAME };
  const lower = requested.toLowerCase();
  let hasSeparator = false;
  for (let cursor = 0; cursor < lower.length; cursor += 1) {
    if (isSeparatorCode(lower.charCodeAt(cursor))) hasSeparator = true;
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

export function judgeExeResolution({ workspaceRoot, branch, exeName, buildDir, exePath, exists }) {
  const facts = [];
  const problems = [];
  if (workspaceRoot !== "") facts.push(`会话工作区：${workspaceRoot}`);
  if (branch !== "") facts.push(`git 分支：${branch}`);
  if (buildDir !== "") facts.push(`构建目录：${buildDir}`);
  if (workspaceRoot === "") {
    problems.push("无法确定会话的工作区根目录（workingDirectory 服务的会话目录、会话 header.cwd、部署默认目录都没有给出）。");
  }
  if (branch === "") {
    problems.push(
      workspaceRoot === ""
        ? "无法确定 git 分支。"
        : `无法确定 ${workspaceRoot} 的 git 分支（DOODLE_GIT_BRANCH 环境变量与 git rev-parse 都拿不到）。`,
    );
  }
  if (problems.length === 0 && exePath === "") {
    problems.push(`无法由工作区、分支与 exeName=${exeName} 推出可执行文件路径。`);
  }
  if (problems.length === 0 && exists !== true) {
    problems.push(`可执行文件不存在：${exePath}。该分支还没有编译过，请先 cmake_configure + cmake_build 再重试。`);
  }
  return { ok: problems.length === 0, problems, facts };
}

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
    if (hasDependency(dir)) return { cwd: dir, lookedAt };
  }
  return { cwd: "", lookedAt };
}

export function playwrightCwdFailure({ workspaceRoot, lookedAt }) {
  const place = workspaceRoot === "" ? "（无法确定会话工作区根目录）" : workspaceRoot;
  const searched = lookedAt.length === 0 ? "（没有可检查的目录）" : lookedAt.join("、");
  return (
    "找不到装着 @playwright/test 的 test_js 目录，已停止本次测试：" +
    `会话工作区 ${place} 与兜底目录里都没有 node_modules/@playwright/test（已查：${searched}）。` +
    "不要改传 cwd 到别处，请用户检查该工作区的 test_js 依赖是否安装。"
  );
}
