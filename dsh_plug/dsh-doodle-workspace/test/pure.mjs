// dsh-doodle-workspace 纯函数自测：直接 import lib/workspace.js，无需安装任何依赖。
//
// 路径关键字只用字符码构造（BS / FW），避免源码里的反斜杠字面量在不同求值方式下退化。

import { strict as assert } from "node:assert";

import * as W from "../lib/workspace.js";

const BS = String.fromCharCode(92);
const FW = String.fromCharCode(47);

let passed = 0;
let failed = 0;
const failures = [];

function test(name, fn) {
  try {
    fn();
    passed += 1;
    process.stdout.write(`  ok  ${name}\n`);
  } catch (error) {
    failed += 1;
    failures.push({ name, error });
    process.stdout.write(`FAIL  ${name}\n      ${error.message}\n`);
  }
}

function eq(actual, expected, what = "value") {
  assert.deepEqual(actual, expected, what);
}

const ROOT = `E:${BS}Doodle`;
const TASK = `E:${BS}doodle_task_fix`;

process.stdout.write("separators\n");
test("isSeparatorCode 只认两个分隔符", () => {
  eq(W.isSeparatorCode(BS.charCodeAt(0)), true);
  eq(W.isSeparatorCode(FW.charCodeAt(0)), true);
  eq(W.isSeparatorCode(":".charCodeAt(0)), false);
  eq(W.isSeparatorCode(0), false);
});

test("toForwardPath / toWindowsPath 双向归一", () => {
  eq(W.toForwardPath(ROOT), `E:${FW}Doodle`);
  eq(W.toWindowsPath(`E:${FW}Doodle`), ROOT);
  eq(W.toForwardPath(""), "");
  eq(W.toForwardPath(undefined), "");
});

test("normalizeRoot 统一分隔符并去掉尾部分隔符", () => {
  eq(W.normalizeRoot(`E:${FW}Doodle${FW}`), ROOT);
  eq(W.normalizeRoot(`  ${ROOT}${BS}${BS}  `), ROOT);
  eq(W.normalizeRoot("E:" + BS), "E:" + BS);
  eq(W.normalizeRoot(42), "");
});

process.stdout.write("parentOf\n");
test("取上一级，盘符根返回空", () => {
  eq(W.parentOf(ROOT), "E:");
  eq(W.parentOf(`E:${BS}a${BS}b`), `E:${BS}a`);
  eq(W.parentOf(`E:${BS}a${BS}b${BS}`), `E:${BS}a`);
  eq(W.parentOf(`E:${BS}`), "");
});

process.stdout.write("joinWindowsPath\n");
test("拼接不重复分隔符", () => {
  eq(W.joinWindowsPath(["E:", "build", "master"]), `E:${BS}build${BS}master`);
  eq(W.joinWindowsPath([`E:${BS}`, "build"]), `E:${BS}build`);
  eq(W.joinWindowsPath(["", null, undefined]), "");
});

process.stdout.write("workspace root\n");
test("优先级：当前目录 > 会话 cwd > 部署默认", () => {
  eq(W.chooseWorkspaceRoot({ workingDirectory: TASK, sessionCwd: ROOT, defaultDirectory: `C:${BS}x` }), TASK);
  eq(W.chooseWorkspaceRoot({ sessionCwd: `E:${FW}Doodle${FW}` }), ROOT);
  eq(W.chooseWorkspaceRoot({ defaultDirectory: `C:${BS}x` }), `C:${BS}x`);
  eq(W.chooseWorkspaceRoot({}), "");
});

test("sessionCwdOf 同时认会话与 agent", () => {
  eq(W.sessionCwdOf({ header: { cwd: ROOT } }), ROOT);
  eq(W.sessionCwdOf({ session: { header: { cwd: `E:${FW}x` } } }), `E:${BS}x`);
  eq(W.sessionCwdOf(undefined), "");
});

process.stdout.write("branch\n");
test("parseGitBranch", () => {
  eq(W.parseGitBranch("master\n"), { kind: "branch", name: "master" });
  eq(W.parseGitBranch("HEAD"), { kind: "detached", name: "" });
  eq(W.parseGitBranch("  "), { kind: "unknown", name: "" });
});

process.stdout.write("build dir\n");
test("buildDirOf 跟随分支", () => {
  eq(W.buildDirOf({ sourceDir: ROOT, branch: "master", presetName: "Ninja_debug" }), `E:${BS}build${BS}master${BS}Ninja_debug`);
  eq(W.buildDirOf({ workspaceRoot: TASK, branch: "task_fix", presetName: "Ninja_debug" }), `E:${BS}build${BS}task_fix${BS}Ninja_debug`);
  eq(W.buildDirOf({ sourceDir: ROOT, branch: "", presetName: "Ninja_debug" }), "");
});

test("preset 表达式按继承链展开", () => {
  const doc = W.parseJsonObject(
    JSON.stringify({
      configurePresets: [
        { name: "base", binaryDir: "${sourceParentDir}/build/$env{DOODLE_GIT_BRANCH}/${presetName}" },
        { name: "child", inherits: "base" },
      ],
    }),
  );
  eq(W.presetBinaryDirExpression(doc, "child"), "${sourceParentDir}/build/$env{DOODLE_GIT_BRANCH}/${presetName}");
  eq(W.resolveBuildDir(doc, { sourceDir: TASK, presetName: "child", branch: "task_fix" }), "E:/build/task_fix/child");
  eq(W.resolveBuildDir(doc, { sourceDir: ROOT, presetName: "missing", branch: "master" }), "");
});

test("exePathOf 拼 bin 目录", () => {
  eq(W.exePathOf({ buildDir: `E:${BS}build${BS}master${BS}Ninja_debug`, exeName: W.DEFAULT_TEST_EXE_NAME }), `E:${BS}build${BS}master${BS}Ninja_debug${BS}bin${BS}${W.DEFAULT_TEST_EXE_NAME}`);
  eq(W.exePathOf({ buildDir: "", exeName: W.DEFAULT_TEST_EXE_NAME }), "");
});

process.stdout.write("exe name\n");
test("只认两个文件名", () => {
  eq(W.resolveExeName({}), { name: W.DEFAULT_TEST_EXE_NAME });
  eq(W.resolveExeName({ exeName: "" }), { name: W.DEFAULT_TEST_EXE_NAME });
  eq(W.resolveExeName({ exeName: W.DEFAULT_PRODUCTION_EXE_NAME }), { name: W.DEFAULT_PRODUCTION_EXE_NAME });
  eq(W.resolveExeName({ exeName: `E:${BS}x${BS}${W.DEFAULT_TEST_EXE_NAME}` }).problem !== undefined, true);
  eq(W.resolveExeName({ exeName: `D:${FW}x${FW}${W.DEFAULT_TEST_EXE_NAME}` }).problem !== undefined, true);
  eq(W.resolveExeName({ exeName: "nope.exe" }).problem !== undefined, true);
});

test("judgeExeResolution", () => {
  const ok = W.judgeExeResolution({
    workspaceRoot: ROOT,
    branch: "master",
    exeName: W.DEFAULT_TEST_EXE_NAME,
    buildDir: `E:${BS}build${BS}master${BS}Ninja_debug`,
    exePath: "x.exe",
    exists: true,
  });
  eq(ok.ok, true);
  eq(ok.facts.length, 3);
  const missing = W.judgeExeResolution({ workspaceRoot: ROOT, branch: "master", exeName: W.DEFAULT_TEST_EXE_NAME, buildDir: "b", exePath: "x.exe", exists: false });
  eq(missing.ok, false);
  assert.match(missing.problems[0], /可执行文件不存在/);
  const none = W.judgeExeResolution({ workspaceRoot: "", branch: "", exeName: W.DEFAULT_TEST_EXE_NAME, buildDir: "", exePath: "", exists: false });
  eq(none.ok, false);
  assert.match(none.problems[0], /工作区根目录/);
});

process.stdout.write("playwright cwd\n");
test("先会话工作区，再兜底目录", () => {
  const picked = W.choosePlaywrightCwd({
    workspaceRoot: ROOT,
    fallbackDirs: [`C:${BS}other${BS}test_js`],
    hasDependency: (dir) => dir === `C:${BS}other${BS}test_js`,
  });
  eq(picked.cwd, `C:${BS}other${BS}test_js`);
  eq(picked.lookedAt.length, 2);
  const none = W.choosePlaywrightCwd({ workspaceRoot: "", fallbackDirs: [], hasDependency: () => false });
  eq(none.cwd, "");
  assert.match(W.playwrightCwdFailure(none), /停止/);
});

process.stdout.write("cache ownership\n");
test("CMakeCache 归属", () => {
  const text = `A=1\nCMAKE_HOME_DIRECTORY:INTERNAL=E:/Doodle\nB=2\n`;
  eq(W.cacheHomeDirectory(text), ROOT);
  eq(W.cacheHomeDirectory("nothing"), "");
  eq(W.buildDirOwnership({ sourceDir: ROOT, buildDir: "x", cacheHomeDirectory: "E:/Doodle" }), "ok");
  eq(W.buildDirOwnership({ sourceDir: TASK, buildDir: "x", cacheHomeDirectory: "E:/Doodle" }), "foreign-source");
  eq(W.buildDirOwnership({ sourceDir: ROOT, buildDir: "x", cacheHomeDirectory: "" }), "empty");
});

process.stdout.write(`\n${passed} passed, ${failed} failed\n`);
if (failed > 0) {
  for (const failure of failures) process.stdout.write(`FAILED: ${failure.name}\n${failure.error.stack}\n`);
  process.exit(1);
}
