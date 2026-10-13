//#region preset 解析（纯函数，可被 test/pure.mjs 通过源码切片单测）

/**
 * CMakePresets.json / CMakeUserPresets.json 的只读解析。
 *
 * 这里刻意只做纯计算，文件读取由调用方（lib/index.js）注入，便于单测。
 */

const KIND_FIELDS = {
  configure: 'configurePresets',
  build: 'buildPresets',
  test: 'testPresets',
  package: 'packagePresets',
  workflow: 'workflowPresets',
};

/**
 * 合并父 preset 与子 preset。
 *
 * 注意：CMake 对 `cacheVariables` 是**深度合并**（子键覆盖父的同名键，父的其余键保留），
 * 而不是整体替换。`Ninja_debug` 正是靠这个语义继承 `Ninja_build` 的
 * CMAKE_C_COMPILER/CMAKE_CXX_COMPILER。其余字段按子覆盖父处理。
 *
 * @param {Record<string, unknown>} parent
 * @param {Record<string, unknown>} child
 * @returns {Record<string, unknown>}
 */
function mergePreset(parent, child) {
  /** @type {Record<string, unknown>} */
  const merged = { ...parent };
  for (const [key, value] of Object.entries(child)) {
    if (key === 'inherits') continue;
    if (key === 'cacheVariables' && isPlainObject(parent[key]) && isPlainObject(value)) {
      merged[key] = { ...parent[key], ...value };
      continue;
    }
    merged[key] = value;
  }
  return merged;
}

/**
 * @param {unknown} value
 * @returns {boolean}
 */
function isPlainObject(value) {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

/**
 * 解析出某个 preset 的完整继承链（含自身），从最基类到自身。
 *
 * @param {Record<string, any>} doc 已解析的 JSON
 * @param {string} kind configure | build | test | package | workflow
 * @param {string} name
 * @returns {{chain: string[], merged: Record<string, any>|undefined, missing: string[]}}
 */
export function resolvePresetChain(doc, kind, name) {
  const field = KIND_FIELDS[kind];
  if (!field || !doc || typeof doc !== 'object') {
    return { chain: [], merged: undefined, missing: [name] };
  }
  const list = Array.isArray(doc[field]) ? doc[field] : [];
  /** @type {Map<string, any>} */
  const byName = new Map();
  for (const p of list) {
    if (p && typeof p === 'object' && typeof p.name === 'string') byName.set(p.name, p);
  }

  /** @type {string[]} */
  const chain = [];
  /** @type {string[]} */
  const missing = [];
  /** @type {Record<string, any>|undefined} */
  let merged;

  const seen = new Set();
  let current = name;
  const stack = [];
  while (typeof current === 'string' && current.length > 0) {
    if (seen.has(current)) {
      missing.push(`inheritance cycle at ${current}`);
      break;
    }
    seen.add(current);
    const node = byName.get(current);
    if (!node) {
      missing.push(current);
      break;
    }
    stack.push(node);
    current = typeof node.inherits === 'string' ? node.inherits : undefined;
  }

  for (let i = stack.length - 1; i >= 0; i -= 1) {
    const node = stack[i];
    chain.push(node.name);
    merged = merged === undefined ? { ...node } : mergePreset(merged, node);
  }

  return { chain, merged, missing };
}

/**
 * 删掉值为 undefined 的键。
 *
 * 工具的 output schema 声明了 `additionalProperties: false`，而 value 中一个
 * 值为 undefined 的键在 JSON 序列化后既不属于声明过的类型、也无法无损表示，
 * 会被 Host 判为 “value is not lossless JSON”。可选字段必须真正缺省。
 *
 * @param {Record<string, any>} obj
 * @returns {Record<string, any>}
 */
function pruneUndefined(obj) {
  /** @type {Record<string, any>} */
  const out = {};
  for (const [key, value] of Object.entries(obj)) {
    if (value !== undefined) out[key] = value;
  }
  return out;
}

/**
 * 汇总一份 preset 文档，产出面向模型的紧凑视图。
 *
 * @param {Record<string, any>} doc
 * @param {{sourceDir: string}} vars
 * @param {string[]} kinds 要包含的 kind
 * @returns {{presets: Record<string, any[]>, cmakeMinimumRequired?: string, version?: number}}
 */
export function summarizePresets(doc, vars, kinds = ['configure', 'build']) {
  /** @type {Record<string, any[]>} */
  const presets = {};
  if (!doc || typeof doc !== 'object') return { presets };

  for (const kind of kinds) {
    const field = KIND_FIELDS[kind];
    if (!field) continue;
    const list = Array.isArray(doc[field]) ? doc[field] : [];
    /** @type {any[]} */
    const rows = [];
    for (const p of list) {
      if (!p || typeof p !== 'object' || typeof p.name !== 'string') continue;
      const { chain, merged } = resolvePresetChain(doc, kind, p.name);
      const binaryDir = typeof merged?.binaryDir === 'string'
        ? expandPresetVarsInline(merged.binaryDir, { sourceDir: vars.sourceDir, presetName: p.name })
        : undefined;
      rows.push(pruneUndefined({
        name: p.name,
        hidden: p.hidden === true,
        displayName: typeof p.displayName === 'string' ? p.displayName : undefined,
        description: typeof p.description === 'string' ? p.description : undefined,
        inherits: chain.length > 1 ? chain.slice(0, -1) : [],
        generator: typeof merged?.generator === 'string' ? merged.generator : undefined,
        binaryDir,
        // 用 merged 而不是 p：configurePreset 常常由祖先 preset 提供
        // （例如 debug_doodle2 → debug1 → Ninja_debug），只读自身字段会漏掉。
        configurePreset: typeof merged?.configurePreset === 'string' ? merged.configurePreset : undefined,
        targets: Array.isArray(p.targets) ? p.targets.slice() : undefined,
        cacheVariables: merged?.cacheVariables && typeof merged.cacheVariables === 'object'
          ? Object.keys(merged.cacheVariables)
          : undefined,
      }));
    }
    presets[kind] = rows;
  }

  /** @type {any} */
  const out = { presets };
  if (typeof doc.version === 'number') out.version = doc.version;
  if (typeof doc.cmakeMinimumRequired === 'object' && doc.cmakeMinimumRequired) {
    out.cmakeMinimumRequired = doc.cmakeMinimumRequired.version;
  }
  return out;
}

/**
 * 与 diagnostics.expandPresetVars 同构，但在此文件内独立，避免跨文件耦合。
 *
 * @param {string} value
 * @param {{sourceDir:string, presetName:string}} vars
 * @returns {string}
 */
function expandPresetVarsInline(value, vars) {
  return value.replace(/\$\{(\w+)\}/g, (whole, key) => {
    if (key === 'sourceDir') return vars.sourceDir;
    if (key === 'presetName') return vars.presetName;
    return whole;
  });
}

/**
 * 由 configure preset 推导其构建目录（绝对路径）。
 *
 * @param {Record<string, any>} doc
 * @param {string} presetName
 * @param {string} sourceDir
 * @returns {string|undefined}
 */
export function binaryDirForConfigurePreset(doc, presetName, sourceDir) {
  const { merged } = resolvePresetChain(doc, 'configure', presetName);
  const raw = typeof merged?.binaryDir === 'string' ? merged.binaryDir : undefined;
  if (!raw) return undefined;
  return expandPresetVarsInline(raw, { sourceDir, presetName });
}

/**
 * 由 build preset 推导其对应的 configure preset 名。
 *
 * @param {Record<string, any>} doc
 * @param {string} presetName
 * @returns {string|undefined}
 */
export function configurePresetForBuildPreset(doc, presetName) {
  const { merged } = resolvePresetChain(doc, 'build', presetName);
  const raw = merged?.configurePreset;
  return typeof raw === 'string' ? raw : undefined;
}

//#endregion

//#region 插件入口
// 下面这行以下的内容依赖 @deepseek-ai/* 与 DSH 服务，不被纯函数单测加载。
//#endregion
