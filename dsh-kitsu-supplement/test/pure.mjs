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

process.stdout.write(`\n${passed} passed, ${failed} failed\n`);
if (failed > 0) {
  for (const failure of failures) process.stdout.write(`  - ${failure.name}: ${failure.error.stack}\n`);
  process.exitCode = 1;
}
