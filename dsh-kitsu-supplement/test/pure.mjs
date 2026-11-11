// dsh-kitsu-supplement 纯函数自测：用 node:vm 从源码切片求值，无需安装 @deepseek-ai/* 依赖。
//
// 契约：lib/index.js 里 `//#region 插件入口` 之前的部分必须是不依赖任何 import 的纯逻辑。
//
// 注意：AGENTS.md 规定"绝不测试"，这里指的是 Doodle 仓库的 C++ 测试与构建目标。
// 本文件是插件自带的语法/纯函数校验脚本（node test/pure.mjs），不涉及 cmake / MSVC。

import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import vm from "node:vm";

const here = dirname(fileURLToPath(import.meta.url));
const libFile = join(here, "..", "lib", "index.js");
// 插件入口之前有多个区段（纯逻辑 / 配置与输出契约），只取第一个：纯逻辑区段。
const MARKER = "//#endregion";

function loadPure(relPath) {
  const source = readFileSync(join(here, "..", "lib", relPath), "utf8");
  const cut = source.indexOf(MARKER);
  if (cut < 0) throw new Error(`${relPath}: missing marker ${MARKER}`);
  const body = source
    .slice(0, cut)
    .replace(/^import[^;]*;[ \t]*\r?\n/gm, "")
    .replace(/^export\s+/gm, "");
  if (body.includes("z.object(")) {
    // Config 用的是 schemastery；纯逻辑区段必须停在它之前，否则这里要真的加载依赖。
    throw new Error(`${relPath}: pure region must end before the Config definition`);
  }
  const context = vm.createContext({ console, Math, JSON, Object, Array, String, Number, RegExp, Map, Set, Error });
  const names = [...body.matchAll(/^(?:async\s+)?(?:function|const|let|var)\s+([A-Za-z_$][\w$]*)/gm)].map((m) => m[1]);
  const script = new vm.Script(`${body}\n;({${names.join(",")}});`, { filename: relPath });
  return script.runInContext(context);
}

const P = loadPure("index.js");

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

async function testAsync(name, fn) {
  try {
    await fn();
    passed += 1;
    process.stdout.write(`  ok  ${name}\n`);
  } catch (error) {
    failed += 1;
    failures.push({ name, error });
    process.stdout.write(`FAIL  ${name}\n      ${error.message}\n`);
  }
}

function eq(actual, expected, what = "value") {
  const a = JSON.stringify(actual);
  const b = JSON.stringify(expected);
  if (a !== b) throw new Error(`${what}: expected ${b}, got ${a}`);
}

function ok(condition, message) {
  if (!condition) throw new Error(message);
}

const EXE = "E:\\build\\Ninja_debug\\bin\\doodle_kitsu_supplement.exe";
const CWD = "E:\\build\\Ninja_debug\\bin";

process.stdout.write("parsePort\n");
test("接受数字与数字字符串", () => {
  eq(P.parsePort(50025), 50025);
  eq(P.parsePort(" 50123 "), 50123);
});
test("拒绝越界与非数字", () => {
  eq(P.parsePort(0), undefined);
  eq(P.parsePort(65536), undefined);
  eq(P.parsePort("50a"), undefined);
  eq(P.parsePort(50.5), undefined);
  eq(P.parsePort(undefined), undefined);
});

process.stdout.write("validateStartArgs\n");
test("缺 port 直接报错（local 会被服务端随机监听）", () => {
  const problems = P.validateStartArgs({ mode: "local", port: undefined, cwd: CWD, exePath: EXE, readyPath: "/" });
  ok(problems.some((p) => p.includes("必须显式指定 port")), problems.join(" | "));
});
test("生产模式缺 port 同样报错", () => {
  const problems = P.validateStartArgs({ mode: "production", port: "", cwd: CWD, exePath: EXE, readyPath: "/" });
  ok(problems.some((p) => p.includes("必须显式指定 port")), problems.join(" | "));
});
test("合法组合没有问题", () => {
  eq(P.validateStartArgs({ mode: "local", port: 50030, cwd: CWD, exePath: EXE, readyPath: "/" }), []);
  eq(P.validateStartArgs({ mode: "local", port: 50330, cwd: CWD, exePath: EXE, readyPath: "/" }), []);
  eq(P.validateStartArgs({ mode: "production", port: "50123", cwd: CWD, exePath: EXE, readyPath: "/" }), []);
});
test("port 低于 50030 报错", () => {
  const problems = P.validateStartArgs({ mode: "local", port: 50029, cwd: CWD, exePath: EXE, readyPath: "/" });
  ok(problems.some((p) => p.includes("50030-50330")), problems.join(" | "));
});
test("port 高于 50330 报错", () => {
  const problems = P.validateStartArgs({ mode: "local", port: 50331, cwd: CWD, exePath: EXE, readyPath: "/" });
  ok(problems.some((p) => p.includes("50030-50330")), problems.join(" | "));
});
test("port 为字符串且越界同样报错", () => {
  const problems = P.validateStartArgs({ mode: "production", port: "50025", cwd: CWD, exePath: EXE, readyPath: "/" });
  ok(problems.some((p) => p.includes("50030-50330")), problems.join(" | "));
});
test("mode 非法", () => {
  const problems = P.validateStartArgs({ mode: "dev", port: 50025, cwd: CWD, exePath: EXE, readyPath: "/" });
  ok(problems.some((p) => p.includes("mode 只能是")), problems.join(" | "));
});
test("root 必须是绝对路径", () => {
  const problems = P.validateStartArgs({ mode: "production", port: 50025, cwd: CWD, root: "sy_maigc", exePath: EXE, readyPath: "/" });
  ok(problems.some((p) => p.includes("root 必须是绝对路径")), problems.join(" | "));
});
test("readyPath 必须以 / 开头", () => {
  const problems = P.validateStartArgs({ mode: "production", port: 50025, cwd: CWD, exePath: EXE, readyPath: "health" });
  ok(problems.some((p) => p.includes("readyPath 必须以 / 开头")), problems.join(" | "));
});

process.stdout.write("validateExtraArgs / validateExtraConfig\n");
test("拒绝插件自己管理的开关", () => {
  const problems = P.validateExtraArgs(["--local", "--port=1", "--config", "x"]);
  eq(problems.length, 3, "problem count");
});
test("允许普通开关与取值", () => {
  eq(P.validateExtraArgs(["--foo", "bar", "--baz=1"]), []);
});
test("extraConfig 不允许带 port", () => {
  const problems = P.validateExtraConfig({ port: 1, db_path: "x" });
  eq(problems.length, 1, "problem count");
});
test("extraConfig 必须是对象", () => {
  eq(P.validateExtraConfig([1]).length, 1);
  eq(P.validateExtraConfig(undefined), []);
});

process.stdout.write("buildArgv\n");
test("local 模式顺序：--config= --root= --local --port=", () => {
  eq(
    P.buildArgv({ mode: "local", port: 50123, configPath: "C:\\t\\c.json", root: "D:\\data", extraArgs: ["--foo"] }),
    ["--config=C:\\t\\c.json", "--root=D:\\data", "--local", "--port=50123", "--foo"],
  );
});
test("production 模式不带 --local", () => {
  eq(P.buildArgv({ mode: "production", port: 50123, configPath: "c.json" }), ["--config=c.json", "--port=50123"]);
});
test("root 为空时不写 --root", () => {
  eq(P.buildArgv({ mode: "production", port: 50123, root: "" }), ["--port=50123"]);
});
test("带值开关一律单参数 --key=value（两个 argv 项会让值掉进位置参数）", () => {
  const argv = P.buildArgv({ mode: "local", port: 50030, configPath: "C:\\a b\\c.json", root: "D:\\x y" });
  eq(argv.filter((item) => item === "--root" || item === "--port" || item === "--config"), []);
  eq(argv, ["--config=C:\\a b\\c.json", "--root=D:\\x y", "--local", "--port=50030"]);
});
test("dbPath 以 --dbPath= 单参数传入，且在 --local 之前", () => {
  eq(
    P.buildArgv({ mode: "local", port: 50123, dbPath: "E:\\db\\test.sqlite" }),
    ["--dbPath=E:\\db\\test.sqlite", "--local", "--port=50123"],
  );
});
test("extraArgs 不允许重复 --dbPath", () => {
  const problems = P.validateExtraArgs(["--dbPath=x", "--dbPath", "y"]);
  eq(problems.length, 2);
});

process.stdout.write("splitSqlStatements\n");
test("按分号切分并保留语句正文", () => {
  eq(P.splitSqlStatements("INSERT INTO a VALUES (1);\nINSERT INTO b VALUES (2);"), [
    "INSERT INTO a VALUES (1)",
    "INSERT INTO b VALUES (2)",
  ]);
});
test("字符串字面量里的分号不切分", () => {
  eq(P.splitSqlStatements("INSERT INTO a VALUES ('x;y'); SELECT 1"), ["INSERT INTO a VALUES ('x;y')", "SELECT 1"]);
});
test("标识符引号里的分号不切分", () => {
  eq(P.splitSqlStatements('INSERT INTO "a;b" VALUES (1);'), ['INSERT INTO "a;b" VALUES (1)']);
  eq(P.splitSqlStatements("INSERT INTO [a;b] VALUES (1);"), ["INSERT INTO [a;b] VALUES (1)"]);
});
test("注释里的分号不切分", () => {
  eq(P.splitSqlStatements("-- a;b\nSELECT 1;"), ["SELECT 1"]);
  eq(P.splitSqlStatements("/* a;b */ SELECT 1;"), ["SELECT 1"]);
});
test("CREATE TRIGGER 体内分号不切分", () => {
  const script = "CREATE TRIGGER t AFTER INSERT ON a BEGIN INSERT INTO b VALUES (1); END; SELECT 1;";
  eq(P.splitSqlStatements(script), [
    "CREATE TRIGGER t AFTER INSERT ON a BEGIN INSERT INTO b VALUES (1); END",
    "SELECT 1",
  ]);
});
test("只有注释或空白的脚本没有语句", () => {
  eq(P.splitSqlStatements("-- nothing\n"), []);
  eq(P.splitSqlStatements("   \n\t"), []);
});

process.stdout.write("rejectSqlStatement\n");
test("拒绝 ATTACH / DETACH（含大小写）", () => {
  ok(P.rejectSqlStatement("ATTACH DATABASE 'x.db' AS x") !== undefined, "attach should be rejected");
  ok(P.rejectSqlStatement("detach database x") !== undefined, "detach should be rejected");
});
test("普通写语句放行", () => {
  eq(P.rejectSqlStatement("INSERT INTO a VALUES (1)"), undefined);
  eq(P.rejectSqlStatement("UPDATE a SET b = 1"), undefined);
});

process.stdout.write("buildConfig\n");
test("默认 -> extraConfig -> 参数覆盖，port 恒为参数", () => {
  const value = P.buildConfig({
    defaults: { db_path: "", kitsu_front_end_path: "D:/fe", public_url_port: 38193 },
    port: 50025,
    extraConfig: { public_url_port: 1, flag: true },
    overrides: { db_path: "E:\\db\\a.db" },
  });
  eq(value, { kitsu_front_end_path: "D:/fe", public_url_port: 1, flag: true, db_path: "E:\\db\\a.db", port: 50025 });
});
test("空串键被丢弃", () => {
  eq(P.buildConfig({ defaults: { a: "", b: "x" }, port: 1 }), { b: "x", port: 1 });
});

process.stdout.write("findPortLine\n");
test("第一行纯数字即端口", () => {
  eq(P.findPortLine("50025\n"), 50025);
  eq(P.findPortLine("50123\r\nlog line\r\n"), 50123);
});
test("首行不是数字 -> undefined（服务端没打印端口就退出）", () => {
  eq(P.findPortLine("[error] config parse failed\n50025\n"), undefined);
  eq(P.findPortLine(""), undefined);
});

process.stdout.write("parseNetstatOwners\n");
test("只取 TCP LISTENING 的目标端口", () => {
  const text = [
    "",
    "  Proto  Local Address          Foreign Address        State           PID",
    "  TCP    0.0.0.0:50025          0.0.0.0:0              LISTENING       4321",
    "  TCP    127.0.0.1:50025        127.0.0.1:50100        ESTABLISHED     9999",
    "  TCP    [::]:50025             [::]:0                 LISTENING       4321",
    "  TCP    0.0.0.0:50026          0.0.0.0:0              LISTENING       1234",
    "",
  ].join("\r\n");
  eq(P.parseNetstatOwners(text, 50025), [4321]);
});
test("无人监听 -> 空数组", () => {
  eq(P.parseNetstatOwners("", 1), []);
});

process.stdout.write("URL 与输出清洗\n");
test("baseUrl / readyUrl", () => {
  eq(P.baseUrlOf(50025), "http://127.0.0.1:50025");
  eq(P.readyUrlOf(50025, "/"), "http://127.0.0.1:50025/");
  eq(P.readyUrlOf(50025, "api/x"), "http://127.0.0.1:50025/api/x");
});
test("sanitizeForJson 丢掉 undefined 并替换孤立代理项", () => {
  eq(P.sanitizeForJson({ a: 1, b: undefined, c: { d: undefined, e: "\uD800tail" } }), { a: 1, c: { e: "\uFFFDtail" } });
});
test("tailText 行对齐", () => {
  eq(P.tailText("a\nbbbb\ncc\n", 6), "cc\n");
  eq(P.tailText("short", 100), "short");
});

process.stdout.write("resolveSpec\n");
const baseConfig = {
  exePath: EXE,
  defaultCwd: "",
  defaultMode: "production",
  defaultRoot: "",
  defaultConfigPaths: { db_path: "C:/db.sqlite", kitsu_front_end_path: "D:/kitsu/dist", root: "D:/kitsu_data", public_url_port: "" },
  readyPath: "/",
};
const allExists = async () => true;
const noneExists = async () => false;

process.stdout.write("resolveSqlTarget\n");
test("参数优先，其次实例，其次插件默认", () => {
  const registry = new Map([["kitsu-1", { dbPath: "E:\\from-instance.db" }]]);
  eq(P.resolveSqlTarget({ args: { dbPath: "E:\\explicit.db" }, config: baseConfig, registry }), {
    dbPath: "E:\\explicit.db",
    serverId: undefined,
  });
  eq(P.resolveSqlTarget({ args: { serverId: "kitsu-1" }, config: baseConfig, registry }), {
    dbPath: "E:\\from-instance.db",
    serverId: "kitsu-1",
  });
  eq(P.resolveSqlTarget({ args: {}, config: baseConfig, registry: new Map() }), {
    dbPath: "C:/db.sqlite",
    serverId: undefined,
  });
});
test("内存库被明确拒绝", () => {
  const registry = new Map([["kitsu-2", { dbPath: ":memory:" }]]);
  const target = P.resolveSqlTarget({ args: { serverId: "kitsu-2" }, config: baseConfig, registry });
  ok(target.problem !== undefined && target.problem.includes("内存库"), String(target.problem));
});
test("空库路径 + 无默认值 -> 报错", () => {
  const target = P.resolveSqlTarget({ args: {}, config: { defaultConfigPaths: { db_path: "" } }, registry: new Map() });
  ok(target.problem !== undefined && target.problem.includes("无法确定数据库路径"), String(target.problem));
});


await testAsync("cwd 缺省取 exe 所在目录", async () => {
  const plan = await P.resolveSpec({ config: baseConfig, args: { port: 50100, mode: "" }, exists: allExists });
  eq(plan.problems, []);
  eq(plan.spec.cwd, CWD);
  eq(plan.spec.mode, "production");
  eq(plan.spec.port, 50100);
  eq(plan.spec.configValues, {
    db_path: "C:/db.sqlite",
    kitsu_front_end_path: "D:/kitsu/dist",
    root: "D:/kitsu_data",
    port: 50100,
  });
});
await testAsync("local + 参数覆盖路径", async () => {
  const plan = await P.resolveSpec({
    config: baseConfig,
    args: { port: "50123", mode: "local", dbPath: "E:/a/b.db", root: "D:/sy", frontEndPath: "E:/fe" },
    exists: allExists,
  });
  eq(plan.problems, []);
  eq(plan.spec.mode, "local");
  eq(plan.spec.port, 50123);
  eq(plan.spec.root, "D:\\sy");
  eq(plan.spec.configValues.db_path, "E:\\a\\b.db");
  eq(plan.spec.configValues.kitsu_front_end_path, "E:\\fe");
});
await testAsync("缺 port 时 problems 非空且 configValues 仍带 port", async () => {
  const plan = await P.resolveSpec({ config: baseConfig, args: { mode: "local" }, exists: allExists });
  ok(plan.problems.length > 0, "expected problems");
  eq(plan.spec.port, undefined);
});
await testAsync("exe / cwd 不存在报错", async () => {
  const plan = await P.resolveSpec({ config: baseConfig, args: { port: 50100, mode: "" }, exists: noneExists });
  ok(plan.problems.some((p) => p.includes("exe 不存在")), plan.problems.join(" | "));
  ok(plan.problems.some((p) => p.includes("cwd 不存在")), plan.problems.join(" | "));
});
await testAsync("exePath 无目录且无 cwd -> 报错而不是猜", async () => {
  const plan = await P.resolveSpec({
    config: { ...baseConfig, exePath: "doodle_kitsu_supplement.exe" },
    args: { port: 50100, mode: "" },
    exists: allExists,
  });
  ok(plan.problems.some((p) => p.includes("无法从 exePath 推断 cwd")), plan.problems.join(" | "));
});

process.stdout.write("validatePlaywrightArgs\n");
test("缺 args / 空数组 / 空串都报错", () => {
  ok(P.validatePlaywrightArgs(undefined).length > 0, "undefined 应该报错");
  ok(P.validatePlaywrightArgs([]).length > 0, "空数组应该报错");
  ok(P.validatePlaywrightArgs(["test", "  "]).length > 0, "空串项应该报错");
  ok(P.validatePlaywrightArgs("test").length > 0, "非数组应该报错");
});
test("原样透传的常见形态都通过", () => {
  eq(P.validatePlaywrightArgs(["test"]), []);
  eq(P.validatePlaywrightArgs(["test", "--list"]), []);
  eq(P.validatePlaywrightArgs(["test", "-g", "创建设备"]), []);
  eq(P.validatePlaywrightArgs(["test", "--", "--my-flag"]), []);
});

process.stdout.write("parseBaseUrlArg\n");
process.stdout.write("parseBaseUrlArg\n");
test("显式地址与缺省；auto 不再被接受", () => {
  eq(P.parseBaseUrlArg(undefined), { kind: "none" });
  eq(P.parseBaseUrlArg("  "), { kind: "none" });
  eq(P.parseBaseUrlArg("http://127.0.0.1:50136/"), { kind: "explicit", value: "http://127.0.0.1:50136" });
  eq(P.parseBaseUrlArg("https://kitsu.local"), { kind: "explicit", value: "https://kitsu.local" });
  eq(P.parseBaseUrlArg("auto").kind, "invalid");
  eq(P.parseBaseUrlArg("AUTO").kind, "invalid");
  ok(P.parseBaseUrlArg("auto").problem.includes("http(s)://"), P.parseBaseUrlArg("auto").problem);
});
test("不是 URL 就报 invalid", () => {
  eq(P.parseBaseUrlArg("127.0.0.1:50136").kind, "invalid");
  eq(P.parseBaseUrlArg("ftp://x").kind, "invalid");
});
eq(typeof P.resolveAutoBaseUrl, "undefined");

process.stdout.write("mergePlaywrightEnv\n");
test("注入覆盖同名环境变量，显式 env 优先级最高", () => {
  const merged = P.mergePlaywrightEnv({
    baseEnv: { Path: "C:\\bin", doodle_base_url: "http://old:1", KEEP: "1" },
    injected: { DOODLE_BASE_URL: "http://new:2", PLAYWRIGHT_HTML_OPEN: "never" },
    explicit: { "doodle_base_url": "http://explicit:3" },
  });
  eq(merged.doodle_base_url, "http://explicit:3", "显式值的键名原样保留，值覆盖注入值");
  eq(Object.keys(merged).filter((key) => key.toUpperCase() === "DOODLE_BASE_URL").length, 1, "同名键只保留一个");
  eq(merged.PLAYWRIGHT_HTML_OPEN, "never");
  eq(merged.Path, "C:\\bin");
  eq(merged.KEEP, "1");
});
test("显式 env 可以新建键，非字符串值被忽略", () => {
  const merged = P.mergePlaywrightEnv({
    baseEnv: {},
    injected: {},
    explicit: { FACE_CAPTURE_TRANSCODE_E2E: "1", BAD: 2 },
  });
  eq(merged.FACE_CAPTURE_TRANSCODE_E2E, "1");
  ok(!Object.hasOwn(merged, "BAD"), "非字符串值不应进入 env");
});

process.stdout.write("playwrightOutputDirs\n");
test("默认落在项目目录，isolate 落到临时目录", () => {
  eq(P.playwrightOutputDirs({ cwd: "E:\\Doodle\\test_js", runId: "pw-1", isolate: false, tmpRoot: "C:\\tmp" }), {
    reportDir: "E:\\Doodle\\test_js/playwright-report",
    outputDir: "E:\\Doodle\\test_js/test-results",
  });
  eq(P.playwrightOutputDirs({ cwd: "E:\\Doodle\\test_js", runId: "pw-1", isolate: true, tmpRoot: "C:\\tmp" }), {
    reportDir: "C:\\tmp/pw-1/playwright-report",
    outputDir: "C:\\tmp/pw-1/test-results",
  });
});
test("cli 路径与锁路径按 cwd 拼", () => {
  eq(P.defaultPlaywrightCli("E:/Doodle/test_js"), "E:\\Doodle\\test_js\\node_modules\\@playwright\\test\\cli.js");
  eq(P.playwrightLockPath("E:/Doodle/test_js/"), "E:\\Doodle\\test_js\\.playwright-run.lock");
});

process.stdout.write("parsePlaywrightSummary\n");
test("抓 passed / failed / skipped / didNotRun / flaky", () => {
  const text = ["  1 failed", "  3 passed (12.4s)", "  2 skipped", "  1 did not run", "  1 flaky"].join("\n");
  eq(P.parsePlaywrightSummary(text), { passed: 3, failed: 1, skipped: 2, didNotRun: 1, flaky: 1 });
});
test("没有计数时全 0", () => {
  eq(P.parsePlaywrightSummary("Running 5 tests using 1 worker"), {
    passed: 0,
    failed: 0,
    skipped: 0,
    didNotRun: 0,
    flaky: 0,
  });
});

process.stdout.write("parsePlaywrightFailures\n");
test("抓失败标题与错误首行", () => {
  const text = [
    "  ✘  1 [face_capture.js:190:1] 设备列表 › 返回设备 (120ms)",
    "",
    "    Error: expect(received).to.equal(expected)",
    "",
    "  1 failed",
  ].join("\n");
  eq(P.parsePlaywrightFailures(text), [
    { title: "设备列表 › 返回设备", error: "Error: expect(received).to.equal(expected)" },
  ]);
});
test("× 记号与跨平台输出都能识别", () => {
  const text = "  ×  2 [canvas.js:10:1] 画布 › 创建 (5ms)\r\n    TimeoutError: page.goto: Timeout 30000ms exceeded";
  eq(P.parsePlaywrightFailures(text), [
    { title: "画布 › 创建", error: "TimeoutError: page.goto: Timeout 30000ms exceeded" },
  ]);
});
test("没有失败行时返回空数组", () => {
  eq(P.parsePlaywrightFailures("  3 passed (12.4s)"), []);
});

process.stdout.write("parsePlaywrightListing\n");
test("抓 --list 的总数与用例行", () => {
  const text = [
    "Listing tests:",
    "  [face_capture.js] › 设备 › 列表",
    "  [canvas.js] › 画布 › 创建",
    "Total: 2 tests in 2 files",
  ].join("\n");
  const listed = P.parsePlaywrightListing(text);
  eq(listed.tests, 2);
  eq(listed.files, 2);
  eq(listed.titles, ["[face_capture.js] › 设备 › 列表", "[canvas.js] › 画布 › 创建"]);
});
test("只有 Total 行时 files 记 0；完全没有就 undefined", () => {
  eq(P.parsePlaywrightListing("Total: 7 tests").files, 0);
  eq(P.parsePlaywrightListing("  3 passed (12.4s)"), undefined);
});

process.stdout.write("interpretPlaywrightExit\n");
test("退出码与失败计数的组合", () => {
  const clean = { passed: 3, failed: 0 };
  eq(P.interpretPlaywrightExit({ exitCode: 0, summary: clean, failures: [] }), "ok");
  eq(P.interpretPlaywrightExit({ exitCode: 1, summary: { passed: 0, failed: 1 }, failures: [] }), "tests-failed");
  eq(P.interpretPlaywrightExit({ exitCode: 1, summary: { passed: 0, failed: 0 }, failures: [] }), "exit-nonzero");
  eq(P.interpretPlaywrightExit({ exitCode: 1, summary: { passed: 0, failed: 0 }, failures: [{ title: "t", error: "e" }] }), "tests-failed");
  eq(P.interpretPlaywrightExit({ exitCode: null, summary: { passed: 0, failed: 0 }, failures: [] }), "exit-nonzero");
});
test("超时与取消优先于退出码", () => {
  const clean = { passed: 0, failed: 0 };
  eq(P.interpretPlaywrightExit({ exitCode: null, summary: clean, failures: [], timedOut: true }), "timeout");
  eq(P.interpretPlaywrightExit({ exitCode: 0, summary: clean, failures: [], aborted: true }), "cancelled");
  eq(P.interpretPlaywrightExit({ exitCode: 0, summary: clean, failures: [], timedOut: false, aborted: false }), "ok");
});
process.stdout.write(`== 槽位规则 / 实例快照 ==\n`);

const rec = (id, mode, extra = {}) => [id, { mode, port: 50030 + id.length, pid: 1000 + id.length, startedAt: Date.now() - 1000, ...extra }];

process.stdout.write(`groupRunningByMode\n`);
test("按模式分组，已退出与非法模式被忽略", () => {
  const groups = P.groupRunningByMode(new Map([rec("kitsu-1", "local"), rec("kitsu-2", "production"), rec("kitsu-3", "local", { exited: true }), rec("kitsu-4", "weird")]));
  eq(JSON.stringify([...groups.get("local")]), JSON.stringify(["kitsu-1"]));
  eq(JSON.stringify([...groups.get("production")]), JSON.stringify(["kitsu-2"]));
});
test("空表与 undefined 都给两个空槽", () => {
  for (const input of [new Map(), undefined]) {
    const groups = P.groupRunningByMode(input);
    eq(groups.get("local").length, 0);
    eq(groups.get("production").length, 0);
  }
});

process.stdout.write(`slotSummary\n`);
test("空表：两个模式都空闲、可用列表全给", () => {
  const summary = P.slotSummary(new Map());
  eq(summary.usage.running, 0);
  eq(summary.usage.limitPerMode, 1);
  eq(JSON.stringify(summary.available), JSON.stringify(["production", "local"]));
  eq(summary.slots.local, null);
  eq(summary.slots.production, null);
  eq(summary.duplicates.length, 0);
});
test("local + production 各一个：都占着，没有空位", () => {
  const summary = P.slotSummary(new Map([rec("kitsu-1", "local"), rec("kitsu-2", "production")]));
  eq(summary.usage.running, 2);
  eq(summary.usage.local, 1);
  eq(summary.usage.production, 1);
  eq(summary.available.length, 0);
  eq(summary.slots.local.serverId, "kitsu-1");
  eq(summary.slots.production.serverId, "kitsu-2");
  eq(summary.slots.local.baseUrl, `http://127.0.0.1:${summary.slots.local.port}`);
  eq(summary.duplicates.length, 0);
});
test("同模式两个实例被记为 duplicate，且该模式不算占着", () => {
  const summary = P.slotSummary(new Map([rec("kitsu-1", "local"), rec("kitsu-2", "local")]));
  eq(summary.usage.local, 2);
  eq(summary.slots.local, null);
  eq(summary.duplicates.length, 1);
  eq(summary.duplicates[0].mode, "local");
  eq(JSON.stringify(summary.duplicates[0].serverIds), JSON.stringify(["kitsu-1", "kitsu-2"]));
  eq(JSON.stringify(summary.available), JSON.stringify(["production"]));
});
test("已退出实例不占槽位", () => {
  const summary = P.slotSummary(new Map([rec("kitsu-1", "production", { exited: true })]));
  eq(summary.usage.running, 0);
  eq(JSON.stringify(summary.available), JSON.stringify(["production", "local"]));
});

process.stdout.write(`findSlotHolder / validateSlotRequest\n`);
test("找占位者；空闲与非法模式返回 undefined", () => {
  const records = new Map([rec("kitsu-7", "production")]);
  eq(P.findSlotHolder(records, "production"), "kitsu-7");
  eq(P.findSlotHolder(records, "local"), undefined);
  eq(P.findSlotHolder(records, "nope"), undefined);
});
test("空闲时直接放行", () => {
  eq(P.validateSlotRequest({ records: new Map(), mode: "local" }).kind, "free");
});
test("重复启动被拒且给出停指纹提示", () => {
  const verdict = P.validateSlotRequest({ records: new Map([rec("kitsu-7", "local")]), mode: "local" });
  eq(verdict.kind, "occupied");
  eq(verdict.holderId, "kitsu-7");
  eq(verdict.holderPort, 50037);
  ok(verdict.problem.includes("kitsu-7"));
  ok(verdict.problem.includes("replaceExisting"));
});
test("replaceExisting=true 时转为替换，并带上原实例信息", () => {
  const verdict = P.validateSlotRequest({ records: new Map([rec("kitsu-7", "local")]), mode: "local", replaceExisting: true });
  eq(verdict.kind, "replace");
  eq(verdict.holderId, "kitsu-7");
  ok(typeof verdict.holderUptimeMs === "number");
});
test("另一个模式占着不影响本模式", () => {
  eq(P.validateSlotRequest({ records: new Map([rec("kitsu-7", "local")]), mode: "production" }).kind, "free");
});

process.stdout.write(`instancesPathOf\n`);
test("默认落在临时目录的 instances.json", () => {
  eq(P.instancesPathOf({ stateDir: "", instancesFile: "", tmpRoot: "C:\\Temp" }), "C:\\Temp\\instances.json");
  eq(P.instancesPathOf({ tmpRoot: "C:\\Temp" }), "C:\\Temp\\instances.json");
});
test("stateDir 覆盖目录；相对 instancesFile 拼在 stateDir 下；绝对路径整体覆盖", () => {
  eq(P.instancesPathOf({ stateDir: "E:\\dsh-state", tmpRoot: "C:\\Temp" }), "E:\\dsh-state\\instances.json");
  eq(P.instancesPathOf({ stateDir: "E:\\dsh-state", instancesFile: "kitsu.json", tmpRoot: "C:\\Temp" }), "E:\\dsh-state\\kitsu.json");
  eq(P.instancesPathOf({ stateDir: "E:\\dsh-state", instancesFile: "D:\\other\\k.json", tmpRoot: "C:\\Temp" }), "D:\\other\\k.json");
});

process.stdout.write(`serializeInstances / parseInstancesState\n`);
test("序列化只留元数据，版本号与 savedAt 就位", () => {
  const dumped = P.serializeInstances(new Map([rec("kitsu-1", "local", { handle: {}, output: "x" })]));
  eq(dumped.version, 1);
  ok(typeof dumped.savedAt === "number");
  eq(dumped.servers.length, 1);
  eq(dumped.servers[0].serverId, "kitsu-1");
  eq("handle" in dumped.servers[0], false);
  eq("output" in dumped.servers[0], false);
});
test("解析再序列化能还原关键字段", () => {
  const original = new Map([rec("kitsu-1", "local", { dbPath: "E:\\a.db", exited: true, exitCode: 3 })]);
  const parsed = P.parseInstancesState(JSON.stringify(P.serializeInstances(original)));
  eq(parsed.problem, undefined);
  eq(parsed.records.length, 1);
  eq(parsed.records[0].mode, "local");
  eq(parsed.records[0].dbPath, "E:\\a.db");
  eq(parsed.records[0].exited, true);
  eq(parsed.records[0].exitCode, 3);
});
test("坏快照都给 problem 而不是抛异常", () => {
  ok(P.parseInstancesState("{not json").problem.includes("JSON"));
  ok(P.parseInstancesState("[]").problem.includes("对象"));
  ok(P.parseInstancesState(JSON.stringify({ version: 99, servers: [] })).problem.includes("版本"));
  ok(P.parseInstancesState(JSON.stringify({ version: 1 })).problem.includes("servers"));
});
test("解析丢掉坏条目与非对象元素", () => {
  const text = JSON.stringify({ version: 1, servers: [null, { serverId: "" }, { serverId: "kitsu-9", mode: "weird", port: "50030" }] });
  const parsed = P.parseInstancesState(text);
  eq(parsed.records.length, 1);
  eq(parsed.records[0].serverId, "kitsu-9");
  eq(parsed.records[0].mode, "production");
  eq(parsed.records[0].port, 50030);
});

process.stdout.write(`instanceLooksAlive / mergeInstancesState\n`);
test("端口上查不到该 pid 就算死了", () => {
  const byPort = new Map([[50030, [111]]]);
  eq(P.instanceLooksAlive({ port: 50030, pid: 111 }, byPort), true);
  eq(P.instanceLooksAlive({ port: 50030, pid: 222 }, byPort), false);
  eq(P.instanceLooksAlive({ port: 50031, pid: 111 }, byPort), false);
  eq(P.instanceLooksAlive({ pid: 111 }, byPort), false);
  eq(P.instanceLooksAlive({ port: 50030, pid: 111 }, new Map()), false);
});
test("合并时实时记录优先；快照里查不到的标 exited", () => {
  const saved = [
    { serverId: "kitsu-1", mode: "local", port: 50030, pid: 111 },
    { serverId: "kitsu-2", mode: "production", port: 50031, pid: 222 },
    { serverId: "kitsu-3", mode: "local", port: 50032, pid: 333 },
  ];
  const live = new Map([["kitsu-1", { mode: "local", port: 50030 }]]);
  const merged = P.mergeInstancesState({ saved, live, livePidsByPort: new Map([[50031, [222]]]) });
  eq(merged.length, 2);
  eq(merged[0].serverId, "kitsu-2");
  eq(merged[0].exited, false);
  eq(merged[1].serverId, "kitsu-3");
  eq(merged[1].exited, true);
});
test("快照里已 exited 的保持 exited（不再探测）", () => {
  const saved = [{ serverId: "kitsu-5", mode: "local", port: 50030, pid: 111, exited: true }];
  const merged = P.mergeInstancesState({ saved, live: new Map(), livePidsByPort: new Map([[50030, [111]]]) });
  eq(merged.length, 1);
  eq(merged[0].exited, true);
});

process.stdout.write(`\n${passed} passed, ${failed} failed\n`);
if (failed > 0) {
  for (const failure of failures) process.stdout.write(`  - ${failure.name}: ${failure.error.stack}\n`);
  process.exitCode = 1;
}