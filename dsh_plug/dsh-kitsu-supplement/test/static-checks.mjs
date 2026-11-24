// dsh-kitsu-supplement 静态缺陷回归：覆盖纯函数测试测不到的三类问题。
//
// 背景：调用 kitsu_playwright_test / kitsu_server_start 时连续暴露了四个缺陷，
// test/pure.mjs 一个都没拦住，因为 pure 测试只看纯函数，不检查：
//   1. 模块级作用域（调用了没导入的名字）        —— choosePlaywrightCwd / playwrightCwdFailure
//   2. 返回值与 output schema 的必填字段是否一致 —— startFailure 少 replaced
//   3. fs API 用错命名空间                        —— node:fs/promises 上调用 statSync
//
// 本文件只做静态检查与形状断言，不启动任何进程、不碰网络。

import { readFileSync, readdirSync, statSync } from "node:fs";
import { fileURLToPath, pathToFileURL } from "node:url";
import { dirname, join, resolve, relative } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const libDir = resolve(here, "..", "lib");
const workspaceLib = pathToFileURL(resolve(here, "..", "..", "dsh-doodle-workspace", "lib", "index.js")).href;

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

function ok(condition, message) {
  if (!condition) throw new Error(message);
}

function filesUnder(dir) {
  const out = [];
  for (const entry of readdirSync(dir)) {
    const full = join(dir, entry);
    if (statSync(full).isDirectory()) out.push(...filesUnder(full));
    else if (/\.(js|mjs)$/.test(entry)) out.push(full);
  }
  return out;
}

const LIB_FILES = filesUnder(libDir);

// 去掉注释与字符串字面量，避免把注释里的名字当成真实引用。
function stripNoise(source) {
  return source
    .replace(/\/\*[\s\S]*?\*\//g, " ")
    .replace(/\/\/[^\n]*/g, " ")
    .replace(/`(?:\\.|[^`\\])*`/g, "``")
    .replace(/"(?:\\.|[^"\\])*"/g, '""')
    .replace(/'(?:\\.|[^'\\])*'/g, "''");
}

// ---------------------------------------------------------------------------
// 1. 从 dsh-doodle-workspace 导入的名字必须齐全
// ---------------------------------------------------------------------------

const WORKSPACE_EXPORTS = Object.keys(await import(workspaceLib)).sort();

test("dsh-doodle-workspace 可加载且导出非空", () => {
  ok(WORKSPACE_EXPORTS.length > 0, "导出列表为空");
});

test("所有用到 dsh-doodle-workspace 符号的文件都导入齐全", () => {
  const problems = [];
  for (const file of LIB_FILES) {
    const source = readFileSync(file, "utf8");
    const body = stripNoise(source);
    const imported = new Set();
    const importRe = /import\s*\{([^}]+)\}\s*from\s*["']dsh-doodle-workspace["']/g;
    let match;
    while ((match = importRe.exec(source)) !== null) {
      for (const token of match[1].split(",")) {
        const name = token.trim().split(/\s+as\s+/).pop();
        if (name) imported.add(name);
      }
    }
    const missing = WORKSPACE_EXPORTS.filter((name) => {
      if (imported.has(name)) return false;
      return new RegExp(`(?<![\\w.$])${name}(?![\\w$])`).test(body);
    });
    if (missing.length > 0) problems.push(`${relative(libDir, file)}: ${missing.join(", ")}`);
  }
  ok(problems.length === 0, `漏导入：\n      ${problems.join("\n      ")}`);
});

// ---------------------------------------------------------------------------
// 2. 不能对 node:fs/promises 调用同步 API
// ---------------------------------------------------------------------------

const SYNC_APIS = [
  "statSync", "lstatSync", "readFileSync", "writeFileSync", "appendFileSync",
  "readdirSync", "mkdirSync", "unlinkSync", "rmSync", "rmdirSync",
  "copyFileSync", "existsSync", "accessSync", "realpathSync", "openSync",
];

// 只去掉注释，保留字符串字面量：fs 检测需要靠 import("node:fs/promises") 的字面量路径。
function stripComments(source) {
  return source.replace(/\/\*[\s\S]*?\*\//g, " ").replace(/\/\/[^\n]*/g, " ");
}

test("没有对 node:fs/promises 调用同步 API", () => {
  const problems = [];
  for (const file of LIB_FILES) {
    const lines = stripComments(readFileSync(file, "utf8")).split("\n");
    lines.forEach((line, index) => {
      const declared = /(?:const|let|var)\s+(\w+)\s*=\s*await\s+import\(\s*["']node:fs\/promises["']\s*\)/.exec(line);
      if (declared === null) return;
      const binding = declared[1];
      for (let cursor = index; cursor < lines.length; cursor += 1) {
        for (const api of SYNC_APIS) {
          if (new RegExp(`\\b${binding}\\.${api}\\b`).test(lines[cursor])) {
            problems.push(`${relative(libDir, file)}:${cursor + 1} ${binding}.${api}`);
          }
        }
      }
    });
  }
  ok(problems.length === 0, `同步 API 误用：\n      ${problems.join("\n      ")}`);
});

test("node:fs/promises 确实没有 statSync（本检查的前提成立）", async () => {
  const fsp = await import("node:fs/promises");
  const fs = await import("node:fs");
  ok(typeof fs.statSync === "function", "node:fs.statSync 不可用");
  ok(fsp.statSync === undefined, "node:fs/promises 出现了 statSync，本规则的依据需重新确认");
});

// ---------------------------------------------------------------------------
// 3. 结果构造函数必须满足各自 output schema 的 required 字段
// ---------------------------------------------------------------------------

const schemas = await import(pathToFileURL(join(libDir, "schemas.js")).href);
const startModule = await import(pathToFileURL(join(libDir, "pure", "start.js")).href);
const playwrightRuntime = await import(pathToFileURL(join(libDir, "runtime", "playwright.js")).href);
const workspaceModule = await import(pathToFileURL(join(libDir, "..", "..", "dsh-doodle-workspace", "lib", "workspace.js")).href);

function requiredOf(schema) {
  return Object.entries(schema.properties ?? {})
    .filter(([, spec]) => spec && spec.required === true)
    .map(([key]) => key);
}

// 与 tools.js 里 resolved 的默认形状保持一致，只为让 startFailure 能算出一个完整结果。
const BASE_SPEC = {
  mode: "local",
  port: 50030,
  cwd: "E:\\build\\master\\Ninja_debug\\bin",
  exePath: "E:\\build\\master\\Ninja_debug\\bin\\doodle_kitsu_supplement_test.exe",
  exeKind: "test",
  readyPath: "/",
  configPath: "C:\\Temp\\kitsu.json",
  command: "doodle_kitsu_supplement_test.exe --local",
};

test("startFailure 默认结果满足 START_SCHEMA 必填项", () => {
  const result = startModule.startFailure(BASE_SPEC, "invalid-args", "exe 解析失败");
  const missing = requiredOf(schemas.START_SCHEMA).filter((key) => !(key in result));
  ok(missing.length === 0, `缺少必填字段：${missing.join(", ")}`);
});

test("startFailure 在 extra 覆盖时仍满足 START_SCHEMA 必填项", () => {
  const cases = [
    { replaced: [] },
    { replaced: [{ serverId: "kitsu-1", port: 50030, ok: false, reason: "kill 失败" }] },
    { serverId: "kitsu-2" },
  ];
  for (const extra of cases) {
    const result = startModule.startFailure(BASE_SPEC, "kill-failed", "boom", extra);
    const missing = requiredOf(schemas.START_SCHEMA).filter((key) => !(key in result));
    ok(missing.length === 0, `extra=${JSON.stringify(extra)} 缺少：${missing.join(", ")}`);
  }
});

test("startFailure 的 replaced 默认是空数组", () => {
  const result = startModule.startFailure(BASE_SPEC, "invalid-args", "boom");
  ok(Array.isArray(result.replaced), `replaced 不是数组：${typeof result.replaced}`);
  ok(result.replaced.length === 0, `replaced 默认应为空：${JSON.stringify(result.replaced)}`);
});

test("startFailure 的 extra 能覆盖 replaced 默认值", () => {
  const filled = [{ serverId: "kitsu-9", port: 50030, ok: true, reason: "" }];
  const result = startModule.startFailure(BASE_SPEC, "kill-failed", "boom", { replaced: filled });
  ok(result.replaced.length === 1, "extra.replaced 未生效");
  ok(result.replaced[0].serverId === "kitsu-9", "extra.replaced 内容丢失");
});

test("playwrightFailure 结果满足 PLAYWRIGHT_SCHEMA 必填项", () => {
  const base = {
    runId: "test-run",
    command: "node cli.js test",
    args: ["test"],
    cwd: "E:\\Doodle\\test_js",
    cliPath: "E:\\Doodle\\test_js\\node_modules\\@playwright\\test\\cli.js",
    baseUrlSource: "arg",
    durationMs: 0,
    summary: { passed: 0, failed: 0, skipped: 0, didNotRun: 0, flaky: 0 },
    failures: [],
    reportDir: "E:\\Doodle\\test_js\\playwright-report",
    outputDir: "E:\\Doodle\\test_js\\test-results",
    diagnostics: [],
    warnings: [],
  };
  const result = playwrightRuntime.playwrightFailure(base, "timeout-cli", "找不到 `@playwright/test`", []);
  const missing = requiredOf(schemas.PLAYWRIGHT_SCHEMA).filter((key) => !(key in result));
  ok(missing.length === 0, `缺少必填字段：${missing.join(", ")}`);
});

// ---------------------------------------------------------------------------
// 4. schema 里的 required 字段必须都有构造点兜底（结构性提示）
// ---------------------------------------------------------------------------

test("START_SCHEMA 与 PLAYWRIGHT_SCHEMA 的必填项都不为空", () => {
  ok(requiredOf(schemas.START_SCHEMA).length > 0, "START_SCHEMA 无必填项");
  ok(requiredOf(schemas.PLAYWRIGHT_SCHEMA).length > 0, "PLAYWRIGHT_SCHEMA 无必填项");
});

// ---------------------------------------------------------------------------
// 5. playwright cwd 候选目录必须包含会话工作区自己的 test_js
//
// 只查 workspaceRoot 本身会漏掉「依赖装在 <工作区>/test_js 下」的常见布局，
// 进而落到宿主仓库的兜底目录，跑出 No tests found。
// ---------------------------------------------------------------------------

test("playwright cwd 优先命中会话工作区的 test_js", () => {
  const workspace = "E:\\some_worktree";
  const fallbackDirs = [""];
  if (workspace !== "") fallbackDirs.unshift(`${workspace}\\test_js`);
  const parent = workspaceModule.parentOf(workspace);
  if (parent !== "") fallbackDirs.push(`${parent}\\Doodle\\test_js`);

  const onlyWorktreeTestJs = (dir) => dir === `${workspace}\\test_js`;
  const chosen = workspaceModule.choosePlaywrightCwd({
    workspaceRoot: workspace,
    fallbackDirs,
    hasDependency: onlyWorktreeTestJs,
  });
  ok(chosen.cwd === `${workspace}\\test_js`, `应命中工作区 test_js，实际：${chosen.cwd || "(空)"}`);
});

test("playwright cwd 在会话工作区自身有依赖时仍优先生效", () => {
  const workspace = "E:\\some_worktree";
  const fallbackDirs = [`${workspace}\\test_js`];
  const chosen = workspaceModule.choosePlaywrightCwd({
    workspaceRoot: workspace,
    fallbackDirs,
    hasDependency: (dir) => dir === workspace || dir === `${workspace}\\test_js`,
  });
  ok(chosen.cwd === workspace, `应命中工作区根，实际：${chosen.cwd || "(空)"}`);
});

test("index.js 的 fallbackDirs 含 workspaceRoot\\test_js 且有去重保障", () => {
  const source = readFileSync(join(libDir, "index.js"), "utf8");
  ok(
    /fallbackDirs\.unshift\(\s*`\$\{workspaceRoot\}\\\\test_js`\s*\)/.test(source),
    "index.js 未把 `${workspaceRoot}\\test_js` 放进候选目录",
  );
  const deduped = workspaceModule.choosePlaywrightCwd({
    workspaceRoot: "E:\\w",
    fallbackDirs: ["E:\\w\\test_js", "E:\\w\\test_js", ""],
    hasDependency: () => false,
  });
  ok(deduped.lookedAt.filter((d) => d === "E:\\w\\test_js").length === 1, "候选目录未去重");
});

process.stdout.write(`\n${passed} passed, ${failed} failed\n`);
if (failed > 0) {
  for (const failure of failures) process.stdout.write(`  - ${failure.name}: ${failure.error.stack}\n`);
  process.exitCode = 1;
}
