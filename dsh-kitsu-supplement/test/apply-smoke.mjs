// 用最小 ctx 真正调用 apply()，验证拆分后的模块在运行期能装配 5 个工具、读状态文件、生成 guidance。
// 不启动真实服务端；start/stop 的执行体由 load-graph 与人工 e2e 覆盖。
import { readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import vm from "node:vm";

const here = dirname(fileURLToPath(import.meta.url));
const libRoot = join(here, "..", "lib");

const cache = new Map();
const problems = [];

function makeSchemaStub() {
  const node = () => ({ default: (v) => v, optional: () => node(), description: () => node(), value: () => node() });
  return {
    default: {
      object: (s) => ({ ...node(), shape: s }),
      string: () => node(),
      number: () => node(),
      boolean: () => node(),
      union: () => node(),
      array: () => node(),
      any: () => node(),
      const: () => node(),
      dict: () => node(),
    },
  };
}

const stubs = {
  "@deepseek-ai/dsh-tools": { defineTool: (spec) => ({ __tool: spec }) },
  "@deepseek-ai/schemastery": makeSchemaStub(),
};

function parseClauseNames(clause) {
  const inner = clause.slice(1, clause.lastIndexOf("}"));
  return inner
    .split(",")
    .map((s) => s.trim())
    .filter(Boolean)
    .map((s) => {
      const [orig, alias] = s.split(/\s+as\s+/);
      return alias ? `${orig.trim()}: ${alias.trim()}` : orig.trim();
    });
}

function transform(raw) {
  const header = [];
  const exported = [];
  let counter = 0;
  let code = raw;

  code = code.replace(/^export\s*\{([^}]*)\}\s*from\s*"([^"]+)";[ \t]*$/gm, (_a, names, spec) => {
    const id = `__ns${counter++}`;
    header.push(`const ${id} = await __load(${JSON.stringify(spec)});`);
    for (const name of parseClauseNames(`{${names}}`)) {
      const [orig, alias] = name.split(": ");
      exported.push({ publicName: alias ?? orig, expr: `${id}.${orig}` });
    }
    return "";
  });

  code = code.replace(/^import\s+([\s\S]*?)\s+from\s+"([^"]+)";[ \t]*$/gm, (_a, clause, spec) => {
    const id = `__ns${counter++}`;
    header.push(`const ${id} = await __load(${JSON.stringify(spec)});`);
    const trimmed = clause.trim();
    if (trimmed.startsWith("{")) header.push(`const { ${parseClauseNames(trimmed).join(", ")} } = ${id};`);
    else if (trimmed.startsWith("*")) header.push(`const ${trimmed.split(/\s+as\s+/)[1].trim()} = ${id};`);
    else header.push(`const ${trimmed} = ${id}.default !== undefined ? ${id}.default : ${id};`);
    return "";
  });

  code = code.replace(/^export\s+(async\s+)?(function|const|class|let|var)\s+([A-Za-z_$][\w$]*)/gm, (_a, asyncKw, kind, name) => {
    exported.push({ publicName: name, expr: name });
    return `${asyncKw ?? ""}${kind} ${name}`;
  });

  const collects = exported.map(({ publicName, expr }) => `__collect(${JSON.stringify(publicName)}, () => ${expr});`);
  const withDynamic = code.replace(/(?<![.\w$])import\s*\(/g, "__import(");
  return `${header.join("\n")}\n${withDynamic}\n${collects.join("\n")}`;
}

async function loadModule(absPath) {
  if (cache.has(absPath)) return cache.get(absPath);
  const exported = {};
  cache.set(absPath, exported);
  const code = transform(readFileSync(absPath, "utf8"));
  const __load = async (spec) => {
    if (!spec.startsWith(".")) {
      if (stubs[spec] === undefined) throw new Error(`缺少 stub: ${spec}`);
      return stubs[spec];
    }
    return loadModule(resolve(dirname(absPath), spec));
  };
  const __collect = (name, getter) => Object.defineProperty(exported, name, { enumerable: true, get: getter });
  globalThis.____dynImport = (spec) => import(spec);
  try {
    const prelude = "const __import = globalThis.____dynImport;\n";
    const fn = new vm.Script(`(async function(__load, __collect) {\n${prelude}${code}\n})`, { filename: absPath }).runInThisContext();
    await fn(__load, __collect);
  } catch (error) {
    problems.push(`${absPath.slice(libRoot.length + 1)}: ${error.message}`);
  }
  return exported;
}

const mod = await loadModule(join(libRoot, "index.js"));

const registered = [];
const sections = [];
const effects = [];
const fakeCtx = {
  tools: {
    get() {
      return undefined;
    },
    register(tool) {
      registered.push(tool.__tool.name);
    },
  },
  systemPrompt: {
    section(spec) {
      sections.push(spec);
    },
  },
  effect(fn) {
    effects.push(fn);
  },
  subprocess: {
    spawn() {
      throw new Error("本检查不 spawn 进程");
    },
  },
};

if (problems.length > 0) { for (const q of problems) process.stdout.write(`LOAD-PROBLEM ${q}\n`); process.exit(1); }
await mod.apply(fakeCtx, {
  exePath: "E:\\build\\Ninja_debug\\bin\\doodle_kitsu_supplement.exe",
  defaultConfigPaths: { db_path: "", kitsu_front_end_path: "", root: "", public_url_port: "" },
  stateDir: process.env.KITSU_STATE_DIR || join(process.env.TEMP || ".", "kitsu-split-check"),
  playwrightCwd: "E:\\Doodle\\test_js",
});

const expected = ["kitsu_server_start", "kitsu_server_status", "kitsu_server_stop", "kitsu_server_sql", "kitsu_playwright_test"];
const missing = expected.filter((n) => !registered.includes(n));
if (missing.length > 0) problems.push(`未注册的工具: ${missing.join(", ")}`);
if (sections.length !== 1) problems.push(`guidance section 数量 ${sections.length}，应为 1`);
if (typeof sections[0]?.text !== "function") problems.push("guidance text 不是函数");
if (effects.length !== 1) problems.push(`ctx.effect 数量 ${effects.length}，应为 1`);

// 导出的 getServer 应可调用
if (typeof mod.getServer !== "function") problems.push("getServer 未导出");

if (problems.length > 0) {
  for (const p of problems) process.stdout.write(`PROBLEM ${p}\n`);
  process.exitCode = 1;
} else {
  process.stdout.write(`OK apply() 注册: ${registered.join(", ")}\n`);
  process.stdout.write(`OK guidance section: ${sections[0].name} (order=${sections[0].order})\n`);
  process.stdout.write("OK ctx.effect 已挂载\n");
}
