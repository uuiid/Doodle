// 加载整个插件模块图以发现「导入不存在的符号 / 未定义标识符」。
// 做法：把 ESM 的静态 import 改写成对 __load() 的 await 调用，在 vm 里以 async 函数执行。
// @deepseek-ai/* 用最小 stub 顶替，其余模块按真实相对路径递归加载。
import { readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import vm from "node:vm";

const here = dirname(fileURLToPath(import.meta.url));
const libRoot = join(here, "..", "lib");

const cache = new Map();
const problems = [];

function makeSchemaStub() {
  const node = () => ({
    default: (v) => v,
    optional: () => node(),
    description: () => node(),
    value: () => node(),
  });
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
    if (trimmed.startsWith("{")) {
      header.push(`const { ${parseClauseNames(trimmed).join(", ")} } = ${id};`);
    } else if (trimmed.startsWith("*")) {
      header.push(`const ${trimmed.split(/\s+as\s+/)[1].trim()} = ${id};`);
    } else {
      header.push(`const ${trimmed} = ${id}.default !== undefined ? ${id}.default : ${id};`);
    }
    return "";
  });

  code = code.replace(/^export\s+(async\s+)?(function|const|class|let|var)\s+([A-Za-z_$][\w$]*)/gm, (_a, asyncKw, kind, name) => {
    exported.push({ publicName: name, expr: name });
    return `${asyncKw ?? ""}${kind} ${name}`;
  });

  const collects = exported.map(({ publicName, expr }) => `__collect(${JSON.stringify(publicName)}, () => ${expr});`);
  return `${header.join("\n")}\n${code}\n${collects.join("\n")}`;
}
async function loadModule(absPath) {
  if (cache.has(absPath)) return cache.get(absPath);
  const exported = {};
  cache.set(absPath, exported);

  const raw = readFileSync(absPath, "utf8");
  const code = transform(raw);

  const __load = async (spec) => {
    if (!spec.startsWith(".")) {
      const stub = stubs[spec];
      if (stub === undefined) throw new Error(`缺少 stub: ${spec}`);
      return stub;
    }
    return loadModule(resolve(dirname(absPath), spec));
  };
  const __collect = (name, getter) => {
    Object.defineProperty(exported, name, { enumerable: true, configurable: true, get: getter });
  };

  const runner = new vm.Script(`(async function(__load, __collect) {\n${code}\n})`, { filename: absPath });
  try {
    const fn = runner.runInThisContext();
    await fn(__load, __collect);
  } catch (error) {
    problems.push(`${absPath.slice(libRoot.length + 1)}: ${error.message}`);
  }
  return exported;
}

const mod = await loadModule(join(libRoot, "index.js"));

if (problems.length > 0) {
  for (const p of problems) process.stdout.write(`PROBLEM ${p}\n`);
  process.exitCode = 1;
} else {
  const keys = Object.keys(mod).sort();
  process.stdout.write(`OK 加载成功: ${keys.join(", ")}\n`);
  if (typeof mod.apply !== "function") process.stdout.write("PROBLEM apply 不是函数\n");
  if (mod.Config === undefined) process.stdout.write("PROBLEM Config 缺失\n");
  if (!Array.isArray(mod.inject)) process.stdout.write("PROBLEM inject 缺失\n");
}
