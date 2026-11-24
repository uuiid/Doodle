// 加载整个插件模块图以发现「导入不存在的符号 / 未定义标识符」。
// 与 dsh-kitsu-supplement 的同名脚本同构：把 ESM 静态 import 改写成 __load()，
// @deepseek-ai/* 用最小 stub，dsh-doodle-workspace 与其余相对路径按真实文件递归加载。

import { readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import vm from "node:vm";

const here = dirname(fileURLToPath(import.meta.url));
const libRoot = join(here, "..", "lib");
const WORKSPACE_ENTRY = fileURLToPath(new URL("../../dsh-doodle-workspace/lib/workspace.js", import.meta.url));

const cache = new Map();
const problems = [];

function makeSchemaStub() {
  const node = () => ({ default: (value) => value, optional: () => node(), description: () => node(), value: () => node() });
  return {
    default: {
      object: (shape) => ({ ...node(), shape }),
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

function loadSharedWorkspace() {
  const source = readFileSync(WORKSPACE_ENTRY, "utf8");
  const names = [...source.matchAll(/^export\s+(?:async\s+)?(?:function|const|class|let|var)\s+([A-Za-z_$][\w$]*)/gm)].map((m) => m[1]);
  const body = source.replace(/^export\s+/gm, "");
  const factory = new vm.Script(`(function() {\n${body}\n; return {${names.join(",")}};\n})`, { filename: WORKSPACE_ENTRY });
  return factory.runInThisContext()();
}

const stubs = {
  "@deepseek-ai/dsh-tools": { defineTool: (spec) => spec },
  "@deepseek-ai/schemastery": makeSchemaStub(),
  "dsh-doodle-workspace": loadSharedWorkspace(),
};

function parseClauseNames(clause) {
  const inner = clause.slice(1, clause.lastIndexOf("}"));
  return inner
    .split(",")
    .map((item) => item.trim())
    .filter(Boolean)
    .map((item) => {
      const [orig, alias] = item.split(/\s+as\s+/);
      return alias ? `${orig.trim()}: ${alias.trim()}` : orig.trim();
    });
}

function transform(raw) {
  const header = [];
  const exported = [];
  let counter = 0;
  let code = raw;

  code = code.replace(/^import\s+([\s\S]*?)\s+from\s+['"]([^'"]+)['"];[ \t]*$/gm, (_all, clause, spec) => {
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

  code = code.replace(/^export\s+(async\s+)?(function|const|class|let|var)\s+([A-Za-z_$][\w$]*)/gm, (_all, asyncKw, kind, name) => {
    exported.push({ publicName: name, expr: name });
    return `${asyncKw ?? ""}${kind} ${name}`;
  });

  code = code.replace(/^export\s*\{([^}]*)\}\s*(?:from\s*['"]([^'"]+)['"])?;?[ \t]*$/gm, (_all, names, from) => {
    let id = "";
    if (from !== undefined) {
      id = `__ns${counter++}`;
      header.push(`const ${id} = await __load(${JSON.stringify(from)});`);
    }
    for (const item of parseClauseNames(`{${names}}`)) {
      const [orig, alias] = item.split(": ");
      const publicName = alias ?? orig;
      exported.push({ publicName, expr: id === "" ? orig : `${id}.${orig}` });
    }
    return "";
  });

  const collects = exported.map(({ publicName, expr }) => `__collect(${JSON.stringify(publicName)}, () => ${expr});`);
  return `${header.join("\n")}\n${code}\n${collects.join("\n")}`;
}

async function loadModule(absPath) {
  if (cache.has(absPath)) return cache.get(absPath);
  const exported = {};
  cache.set(absPath, exported);

  const code = transform(readFileSync(absPath, "utf8"));
  const __load = async (spec) => {
    if (Object.prototype.hasOwnProperty.call(stubs, spec)) return stubs[spec];
    if (!spec.startsWith(".")) throw new Error(`缺少 stub: ${spec}`);
    return loadModule(resolve(dirname(absPath), spec));
  };

  const __collect = (name, getter) => {
    Object.defineProperty(exported, name, { enumerable: true, configurable: true, get: getter });
  };

  const runner = new vm.Script(`(async function(__load, __collect) {\n${code}\n})`, { filename: absPath });
  try {
    await runner.runInThisContext()(__load, __collect);
  } catch (error) {
    problems.push(`${absPath.slice(libRoot.length + 1)}: ${error.message}`);
  }
  return exported;
}

const mod = await loadModule(join(libRoot, "index.js"));

if (problems.length > 0) {
  for (const problem of problems) process.stdout.write(`PROBLEM ${problem}\n`);
  process.exitCode = 1;
} else {
  const missing = ["Config", "apply", "inject", "name"].filter((key) => mod[key] === undefined);
  if (missing.length > 0) {
    process.stdout.write(`PROBLEM 导出缺失: ${missing.join(", ")}\n`);
    process.exitCode = 1;
  } else {
    process.stdout.write(`OK 加载成功: ${Object.keys(mod).sort().join(", ")}\n`);
  }
}
