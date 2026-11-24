/**
 * 纯函数单测：直接读 lib/index.js 源码，抽出纯函数在 vm 里求值。
 * 这样无需 import 插件（避免解析 @deepseek-ai/* 依赖），也能覆盖护栏逻辑。
 *
 * 运行：node test/pure.mjs
 */
import { readFileSync } from "node:fs";
import vm from "node:vm";

const source = readFileSync(new URL("../lib/index.js", import.meta.url), "utf8");
const block = source.slice(0, source.indexOf("//#region 插件入口"));
const sandbox = {
  // 最小 schemastery stub：只为让 Config 区求值，本测试不校验 schema 行为。
  z: {
    object: () => ({ default: () => ({}) }),
    string: () => ({ default: () => "" }),
    number: () => ({ default: () => 0 }),
    array: () => ({ default: () => [] }),
  },
};
vm.createContext(sandbox);
// 去掉 import/export 语句后求值；纯函数区不依赖任何导入。
vm.runInContext(block.replace(/^import .*$/gm, "").replace(/^export /gm, ""), sandbox);

const {
  trimTrailingSeparators,
  isFilesystemRoot,
  stripLongPathPrefix,
  tokenizePattern,
  patternHasScope,
  escapeLiteral,
  buildTerms,
  buildQuery,
  normalizeResults,
  joinPath,
  filetimeToIso,
  clampMaxResults,
  clampTimeout,
  resolveScope,
  esearchGuidanceText,
} = sandbox;

let failures = 0;
function check(label, actual, expected) {
  const ok = JSON.stringify(actual) === JSON.stringify(expected);
  if (!ok) failures += 1;
  console.log(`${ok ? "ok  " : "FAIL"} ${label}${ok ? "" : `\n     actual   = ${JSON.stringify(actual)}\n     expected = ${JSON.stringify(expected)}`}`);
}
function throws(label, fn, fragment) {
  try {
    fn();
    failures += 1;
    console.log(`FAIL ${label} (expected a throw)`);
  } catch (error) {
    const ok = String(error.message).includes(fragment);
    if (!ok) failures += 1;
    console.log(`${ok ? "ok  " : "FAIL"} ${label}${ok ? "" : ` -> ${error.message}`}`);
  }
}

// —— 范围护栏 ——
check("trimTrailingSeparators 去尾分隔符", trimTrailingSeparators("E:\\Doodle\\src\\"), "E:\\Doodle\\src");
check("isFilesystemRoot 盘根", isFilesystemRoot("E:\\"), true);
check("isFilesystemRoot 无斜杠盘根", isFilesystemRoot("E:"), true);
check("isFilesystemRoot UNC 根", isFilesystemRoot("\\\\srv\\share"), true);
check("isFilesystemRoot 普通目录为假", isFilesystemRoot("E:\\Doodle\\src"), false);
check("stripLongPathPrefix", stripLongPathPrefix("\\\\?\\E:\\a\\b"), "E:\\a\\b");

// —— 查询构造 ——
check("tokenizePattern 保留引号分组", tokenizePattern('foo "bar baz"'), ["foo", '"bar baz"']);
check("patternHasScope 识别 path:", patternHasScope("content:x path:E:\\Doodle"), true);
check("patternHasScope 不误判 content:", patternHasScope("content:alpha beta"), false);
check("escapeLiteral 空格加引号", escapeLiteral("my file.txt"), '"my file.txt"');
check("escapeLiteral 普通词不动", escapeLiteral("*.cpp"), "*.cpp");

check("buildTerms 文件名", buildTerms({ pattern: "*.cpp", content: false, regex: false, caseSensitive: false, ext: "", foldersOnly: false, raw: false }), ["*.cpp"]);
check("buildTerms 文件名+ext", buildTerms({ pattern: "*.cpp", content: false, regex: false, caseSensitive: false, ext: "cpp;h", foldersOnly: false, raw: false }), ["*.cpp", "ext:cpp;h"]);
check("buildTerms 内容", buildTerms({ pattern: "retarget_rotations", content: true, regex: false, caseSensitive: false, ext: "", foldersOnly: false, raw: false }), ["content:retarget_rotations"]);
check("buildTerms 内容+ext+regex+case+folders", buildTerms({ pattern: "x", content: true, regex: true, caseSensitive: true, ext: "cpp", foldersOnly: true, raw: false }), ["content:x", "ext:cpp", "regex:", "case:", "/ad"]);
check("buildTerms raw 原样透传", buildTerms({ pattern: "content:<a b> ext:cpp", content: false, regex: false, caseSensitive: false, ext: "", foldersOnly: false, raw: true }), ["content:<a b> ext:cpp"]);

check(
  "buildQuery 追加 path 范围",
  buildQuery({ pattern: "retarget_rotations", content: true, regex: false, caseSensitive: false, ext: "", foldersOnly: false, raw: false }, "E:\\Doodle\\src"),
  "content:retarget_rotations path:E:\\Doodle\\src",
);
check(
  "buildQuery 无范围时不加 path",
  buildQuery({ pattern: "*.cpp", content: false, regex: false, caseSensitive: false, ext: "", foldersOnly: false, raw: false }, undefined),
  "*.cpp",
);

// —— 结果归一化 ——
const normalizingPayload = {
  totalResults: 3,
  results: [
    { type: "file", name: "kimodo.cpp", path: "E:\\Doodle\\src\\ai", size: 24678, date_modified: 134340803243422688 },
    { type: "folder", name: "motion_rep", path: "E:\\Doodle\\src\\ai" },
    { type: "file", name: "", path: "E:\\Doodle\\src\\ai\\direct.cpp" },
  ],
};
const normalized = normalizeResults(normalizingPayload, 10);
check("归一化 total", normalized.total, 3);
check("归一化 文件绝对路径", normalized.items[0].path, "E:\\Doodle\\src\\ai\\kimodo.cpp");
check("归一化 kind=file", normalized.items[0].kind, "file");
check("归一化 sizeBytes", normalized.items[0].sizeBytes, 24678);
check("归一化 dateModified 为 ISO", typeof normalized.items[0].dateModified, "string");
check("归一化 文件夹 kind", normalized.items[1].kind, "folder");
check("归一化 无 name 时用 path 本身", normalized.items[2].path, "E:\\Doodle\\src\\ai\\direct.cpp");
check("归一化 遵守 limit", normalizeResults(normalizingPayload, 2).items.length, 2);
check("归一化 空结果", normalizeResults({ totalResults: 0, results: [] }, 10), { total: 0, items: [] });

// —— 其它 ——
check("joinPath 反斜杠", joinPath("E:\\a\\", "b.cpp"), "E:\\a\\b.cpp");
check("joinPath 空目录", joinPath("", "b.cpp"), "b.cpp");
check("filetimeToIso 非法值返回 undefined", filetimeToIso(0), undefined);
check("clampMaxResults 上限", clampMaxResults(99999), 500);
check("clampMaxResults 下限", clampMaxResults(0), 1);
check("clampMaxResults 默认", clampMaxResults(undefined), 50);
check("clampTimeout 默认", clampTimeout(undefined), 30000);

// —— 范围护栏（resolveScope 与纯函数同区，可直接验证）——
throws("resolveScope 缺范围必须报错", () => resolveScope(undefined, [], {}), "no path scope");
throws("resolveScope 拒绝盘根", () => resolveScope("E:\\", [], {}), "filesystem root");
check("resolveScope 用显式 path", resolveScope("E:\\Doodle\\src\\", [], {}), "E:\\Doodle\\src");
check("resolveScope 回退配置 scopes", resolveScope(undefined, ["E:\\Doodle\\src"], {}), "E:\\Doodle\\src");
check("resolveScope 回退会话 workspace", resolveScope(undefined, [], { cwd: "E:\\Doodle" }), "E:\\Doodle");

// —— 系统提示词指引文案 ——
check("指引点名 glob/grep", esearchGuidanceText(true, true).includes("instead of glob or grep"), true);
check("指引要求 path 范围", esearchGuidanceText(true, true).includes("`path` scope"), true);
check("只有 grep 时只点名 grep", esearchGuidanceText(false, true).includes("instead of grep or any shell search"), true);
check("两个都没有时不提替代工具", esearchGuidanceText(false, false).includes("instead of"), false);

// —— 插件装配：esearch 工具 + `tool:esearch` 指引段 ——
// 整份源码在独立 context 里求值（剥掉 import/export），用 stub 顶替 defineTool 与 schemastery。
const assemblyContext = {
  z: {
    object: () => ({ default: () => ({}) }),
    string: () => ({ default: () => "" }),
    number: () => ({ default: () => 0 }),
    array: () => ({ default: () => [] }),
  },
  defineTool: (tool) => tool,
  console,
};
vm.createContext(assemblyContext);
vm.runInContext(source.replace(/^import .*$/gm, "").replace(/^export /gm, ""), assemblyContext);
const { apply } = assemblyContext;

/** 最小 ctx：记录注册结果，`visible` 决定哪些工具"在可见工具集里"。 */
function stubCtx(visible) {
  const sections = [];
  const registered = [];
  return {
    sections,
    registered,
    ctx: {
      tools: {
        register: (tool) => registered.push(tool),
        get: (name) => (visible.includes(name) ? { name } : undefined),
      },
      systemPrompt: { section: (section) => sections.push(section) },
    },
  };
}

const full = stubCtx(["esearch", "glob", "grep"]);
apply(full.ctx, {});
check("装配注册 esearch 工具", full.registered.map((tool) => tool.name), ["esearch"]);
check("装配注册指引段", full.sections.map((section) => section.name), ["tool:esearch"]);
check("默认段序 1390（先于 TOOL_GLOB=1400）", full.sections[0].order, 1390);
check("esearch 可见时指引非空", full.sections[0].text({ scope: {} }).includes("Use the esearch tool"), true);

const bare = stubCtx(["esearch"]);
apply(bare.ctx, {});
check("没有 glob/grep 时不点名它们", bare.sections[0].text({ scope: {} }).includes("instead of"), false);

const hidden = stubCtx(["glob", "grep"]);
apply(hidden.ctx, {});
check("esearch 不可见时指引为空", hidden.sections[0].text({ scope: {} }), "");

const raised = stubCtx(["esearch"]);
apply(raised.ctx, { guidanceOrder: 999 });
check("guidanceOrder 可覆盖到 999", raised.sections[0].order, 999);

throws("guidanceOrder 非数字必须报错", () => apply(stubCtx(["esearch"]).ctx, { guidanceOrder: "top" }), "guidanceOrder");

console.log(failures === 0 ? "\nALL PASS" : `\n${failures} FAILURE(S)`);
process.exitCode = failures === 0 ? 0 : 1;
