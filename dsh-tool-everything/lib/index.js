/**
 * dsh-tool-everything — Everything 驱动的文件/内容检索工具（宿主端）
 *
 * 设计要点（每一条都对应一个已经踩过的坑）：
 *
 * 1) **路径范围是硬要求**。Everything 的 `content:` 在不限定路径时会退化为
 *    实时全盘内容扫描，能把机器卡死。本工具因此：
 *      - 模型未给 `path` 时，回退到配置的 `scopes`，再回退到会话 workspace；
 *      - 三者都没有 → 直接报错，**绝不**发出不带范围的查询；
 *      - 范围解析为盘符根（`E:\`）或 UNC 根 → 直接拒绝。
 * 2) **范围只由参数/配置决定**，不会去解析模型写的查询字符串里的 `path:`；
 *    模型若自己写了 `path:`，则把它当作"已限定范围"直接透传（这是 Everything
 *    的原生语法，强行重写反而会破坏它的语义）。
 * 3) 传输优先 HTTP（`&json=1&path_column=1`，0.1s 级）；可选 `es.exe` 兜底。
 *    es.exe 走命名管道 IPC，在 DSH 沙箱下会被拒绝访问，所以只作兜底。
 * 4) 输出为**绝对路径**，便于直接接 read/grep/lsp。
 *
 * 注意：本文件按 DSH 插件惯例不使用任何顶层 `node:` 静态导入（见
 * dsh-auto-continue-429 的说明），Node 内置模块在 apply() 内懒加载。
 */

import { defineTool } from "@deepseek-ai/dsh-tools";
import z from "@deepseek-ai/schemastery";

//#region 常量与配置

/** Cordis 插件名（loader 诊断用）。 */
export const name = "tool-everything";

/** 本插件依赖的服务。 */
export const inject = ["tools", "systemPrompt"];

const DEFAULT_ENDPOINT = "http://127.0.0.1:8099/";
const DEFAULT_MAX_RESULTS = 50;
const DEFAULT_TIMEOUT_MS = 30000;
/**
 * 系统提示词里本工具指引段的排序值。默认 1390：紧随 `TOOL_EDIT`(1300)、
 * 先于 `TOOL_GLOB`(1400) 与 `TOOL_GREP`(1500)，即检索类指引的头一条。
 * 想顶到所有工具指引之前，设为 999（`TOOL_BASH` 为 1000）。
 */
const DEFAULT_GUIDANCE_ORDER = 1390;
/** 指引段名（提示词同一层内必须唯一，与内置 `tool:glob` / `tool:grep` 同惯例）。 */
const GUIDANCE_SECTION = "tool:esearch";
const HARD_MAX_RESULTS = 500;
const MAX_TIMER_DELAY_MS = 2147483647;

/** Everything 的 FILETIME 纪元（1601-01-01）到 Unix 纪元的毫秒差。 */
const FILETIME_EPOCH_DELTA_MS = 11644473600000;

/** 插件配置。 */
export const Config = z.object({
  /** Everything HTTP 服务地址，例如 `http://127.0.0.1:8099/`。 */
  endpoint: z.string().default(DEFAULT_ENDPOINT),
  /** `es.exe` 路径；配置后仅在 HTTP 传输失败时兜底（沙箱下可能被拒绝访问）。 */
  esPath: z.string().default(""),
  /** 允许的搜索根目录；模型未给 `path` 时取第一项。 */
  scopes: z.array(z.string()).default([]),
  /** 单次返回条数上限。 */
  maxResults: z.number().default(DEFAULT_MAX_RESULTS),
  /** 单次查询预算（毫秒）。 */
  timeoutMs: z.number().default(DEFAULT_TIMEOUT_MS),
  /** 系统提示词指引段的排序值（默认 1390，见 DEFAULT_GUIDANCE_ORDER）。 */
  guidanceOrder: z.number().default(DEFAULT_GUIDANCE_ORDER),
});

//#endregion

//#region 纯工具函数（可单测）

/** 去掉末尾路径分隔符（保留盘符根的 `E:\` 与 UNC 根）。 */
function trimTrailingSeparators(value) {
  const trimmed = value.replace(/[\\/]+$/, "");
  return trimmed === "" ? value : trimmed;
}

/** 是否盘符根（`E:\`）或 UNC 根（`\\server\share`）。这类范围等同全盘，必须拒绝。 */
function isFilesystemRoot(value) {
  const normalized = value.replace(/\//g, "\\");
  if (/^[A-Za-z]:\\?$/.test(normalized)) return true;
  if (/^\\\\[^\\]+\\[^\\]+\\?$/.test(normalized)) return true;
  return false;
}

/** 去掉 Windows 长路径前缀，统一为普通绝对路径。 */
function stripLongPathPrefix(value) {
  let result = value;
  if (result.startsWith("\\\\?\\UNC\\")) result = "\\\\" + result.slice(8);
  else if (result.startsWith("\\\\?\\")) result = result.slice(4);
  return result;
}

/**
 * 会话工作目录：`exec` 上可能有 `cwd`；没有就返回 undefined。
 * 故意不引 node:path —— 顶层不出现 node 内置导入是插件的硬约束。
 */
function sessionCwd(exec) {
  const candidate = exec?.cwd ?? exec?.workspace?.cwd ?? exec?.agent?.cwd;
  return typeof candidate === "string" && candidate !== "" ? candidate : undefined;
}

/** 把任一对象/字符串转成可读的错误文本。 */
function errorText(error) {
  if (error === null || error === undefined) return "unknown error";
  if (typeof error === "string") return error;
  if (error instanceof Error) return error.message;
  if (typeof error.message === "string") return error.message;
  return String(error);
}

/** Everything 的 FILETIME（自 1601 起的 100ns 数）转 ISO-8601。 */
function filetimeToIso(value) {
  if (typeof value !== "number" || !Number.isFinite(value) || value <= 0) return undefined;
  const ms = value / 10000 - FILETIME_EPOCH_DELTA_MS;
  const date = new Date(ms);
  return Number.isNaN(date.getTime()) ? undefined : date.toISOString();
}

/** 把模型给的模式拆成词元，保留引号包裹的内容（Everything 用引号做转义/分组）。 */
function tokenizePattern(pattern) {
  const tokens = [];
  let current = "";
  let quoted = false;
  for (const char of pattern) {
    if (char === '"') {
      current += char;
      quoted = !quoted;
      continue;
    }
    if (!quoted && /\s/.test(char)) {
      if (current !== "") {
        tokens.push(current);
        current = "";
      }
      continue;
    }
    current += char;
  }
  if (current !== "") tokens.push(current);
  return tokens;
}

/**
 * 判断模型自己写的模式里是否已经限定了范围。
 * 只看带引号的词元是否以 `path:` 开头 —— 与工具自身的 `path` 参数语义一致，
 * 不会误判 `content:foo` 之类。
 */
function patternHasScope(pattern) {
  return tokenizePattern(pattern).some((token) => token.toLowerCase().startsWith("path:"));
}

/** 转义 Everything 查询里的普通字面量（运算字符需加引号）。 */
function escapeLiteral(value) {
  if (value === "") return value;
  if (/[\s"|!<>]/.test(value)) return `"${value.replace(/"/g, '""')}"`;
  return value;
}

/**
 * 构造搜索词元。注意：`pattern` **不加引号** —— 它是 Everything 语法本身
 * （`content:<a b>`、`ext:cpp;h`、通配符、`!` 取反等都要靠元字符生效）。
 */
function buildTerms(input) {
  const terms = [];
  if (input.raw === true) {
    // 高级用法：整串当作 Everything 查询，只补范围。
    terms.push(input.pattern);
  } else if (input.content === true) {
    terms.push(`content:${input.pattern}`);
    if (input.ext !== "") terms.push(`ext:${input.ext}`);
  } else {
    terms.push(input.pattern);
    if (input.ext !== "") terms.push(`ext:${input.ext}`);
  }
  if (input.regex === true) terms.push("regex:");
  if (input.caseSensitive === true) terms.push("case:");
  if (input.foldersOnly === true) terms.push("/ad");
  return terms.filter((term) => term !== "");
}

/** 拼出最终发往 Everything 的查询字符串。 */
function buildQuery(input, scope) {
  const terms = buildTerms(input);
  if (scope !== undefined) terms.push(`path:${scope}`);
  return terms.join(" ").replace(/\s+/g, " ").trim();
}

/** 把 HTTP JSON 结果归一化为绝对路径条目。 */
function normalizeResults(payload, maxResults) {
  const rawResults = Array.isArray(payload?.results) ? payload.results : [];
  const items = [];
  for (const entry of rawResults.slice(0, maxResults)) {
    if (entry === null || typeof entry !== "object") continue;
    const entryName = typeof entry.name === "string" ? entry.name : "";
    const entryPath = typeof entry.path === "string" ? entry.path : "";
    const absolute = stripLongPathPrefix(
      entryPath === "" ? entryName : joinPath(entryPath, entryName),
    );
    const item = { path: absolute };
    if (entry.type === "folder") item.kind = "folder";
    else item.kind = "file";
    if (typeof entry.size === "number") item.sizeBytes = entry.size;
    const modified = filetimeToIso(entry.date_modified);
    if (modified !== undefined) item.dateModified = modified;
    items.push(item);
  }
  const total = typeof payload?.totalResults === "number" ? payload.totalResults : items.length;
  return { total, items };
}

/** 简单路径拼接（不引 node:path，保持模块在任意宿主下可解析）。 */
function joinPath(directory, leaf) {
  const head = directory.replace(/[\\/]+$/, "");
  const tail = leaf.replace(/^[\\/]+/, "");
  if (head === "") return tail;
  if (tail === "") return head;
  const separator = head.includes("\\") || /^[A-Za-z]:/.test(head) ? "\\" : "/";
  return head + separator + tail;
}

/** 归一化 maxResults 到 [1, HARD_MAX_RESULTS]。 */
function clampMaxResults(value) {
  const numeric = typeof value === "number" && Number.isFinite(value) ? Math.floor(value) : DEFAULT_MAX_RESULTS;
  return Math.min(Math.max(numeric, 1), HARD_MAX_RESULTS);
}

/** 归一化超时到 Node 可表示的范围。 */
function clampTimeout(value) {
  const numeric = typeof value === "number" && Number.isFinite(value) ? Math.floor(value) : DEFAULT_TIMEOUT_MS;
  return Math.min(Math.max(numeric, 1), MAX_TIMER_DELAY_MS);
}

/**
 * 解析出这次查询的范围；无可用范围或范围过宽时抛错（护栏）。
 * 故意不解析模型写的查询串里的 `path:` —— 范围只由参数与配置决定。
 */
function resolveScope(requestedPath, configuredScopes, exec) {
  const explicit = typeof requestedPath === "string" ? requestedPath.trim() : "";
  if (explicit !== "") {
    const normalized = trimTrailingSeparators(explicit);
    if (isFilesystemRoot(normalized)) {
      throw new Error(
        `esearch refused: "${explicit}" is a filesystem root, which makes Everything scan the whole volume. ` +
          "Pass a narrower directory (for example E:\\Doodle\\src).",
      );
    }
    return normalized;
  }
  for (const candidate of configuredScopes) {
    if (typeof candidate === "string" && candidate.trim() !== "") {
      return trimTrailingSeparators(candidate.trim());
    }
  }
  const cwd = sessionCwd(exec);
  if (cwd !== undefined) return trimTrailingSeparators(cwd);
  throw new Error(
    "esearch refused: no path scope. Without a scope an Everything content search scans entire volumes " +
      "and can freeze the machine, so this tool never runs unscoped. Pass `path`, or configure `scopes` for this plugin.",
  );
}

/**
 * 生成系统提示词里的检索指引文案。
 *
 * 它与内置 `glob`/`grep` 的指引段（`tool:glob` / `tool:grep`）是同一机制，靠 `order`
 * 排在它们之前，所以文案必须显式说明"优先用 esearch"；未注册的工具名不会被提及，
 * 免得指引指向一个当前不可用的工具。
 *
 * @param hasGlob - 内置 `glob` 工具是否在可见工具集里。
 * @param hasGrep - 内置 `grep` 工具是否在可见工具集里。
 * @returns 指引段文本（永不为空；是否渲染由调用方按 esearch 是否可见决定）。
 */
function esearchGuidanceText(hasGlob, hasGrep) {
  const alternatives = [hasGlob ? "glob" : "", hasGrep ? "grep" : ""].filter(Boolean);
  const instead = alternatives.length === 0 ? "" : ` instead of ${alternatives.join(" or ")} or any shell search`;
  const fallback =
    alternatives.length === 0
      ? ""
      : ` Use ${alternatives.join("/")} only when esearch reports no matches or Everything is unreachable.`;
  return (
    `Use the esearch tool${instead} for filename and content search: it queries the Windows Everything index ` +
    "and returns in about a tenth of a second, without walking the filesystem. Always pass a `path` scope." +
    fallback
  );
}

//#endregion

//#region 传输：HTTP 与 es.exe 兜底

/** 用 Everything HTTP 接口查询，返回归一化结果。 */
async function queryHttp(client, query, limit, signal) {
  const url = new URL(client.endpoint);
  url.searchParams.set("search", query);
  url.searchParams.set("json", "1");
  url.searchParams.set("path_column", "1");
  url.searchParams.set("size_column", "1");
  url.searchParams.set("date_modified_column", "1");
  url.searchParams.set("count", String(limit));

  const response = await fetch(url, { signal, headers: { accept: "application/json" } });
  if (!response.ok) {
    throw new Error(`Everything HTTP ${response.status} ${response.statusText}`);
  }
  const text = await response.text();
  let payload;
  try {
    payload = JSON.parse(text);
  } catch {
    throw new Error(`Everything HTTP returned non-JSON payload: ${text.slice(0, 200)}`);
  }
  return normalizeResults(payload, limit);
}

/**
 * 用 es.exe 查询（可选兜底）。es.exe 通过命名管道与 Everything 通信，
 * 在受限沙箱下会“拒绝访问”，因此这里只作为 HTTP 失败后的第二选择。
 * 内容搜索必须用 `-path` 选项限定范围 —— 写成查询串里的 `path:` 会静默返回 0 条。
 */
async function queryEsExecutable(client, query, limit, scope, signal) {
  const { spawn } = await import("node:child_process");
  const args = [];
  if (scope !== undefined && !patternHasScope(query)) {
    // 从查询里剥掉工具自己加上的 path: 词元，改用 -path 选项（es.exe 的正确用法）。
    args.push("-path", scope);
  }
  args.push("-json", "-path-column", "-size", "-dm", "-n", String(limit), query.replace(/\s*path:[^\s]*/g, "").trim());

  const child = spawn(client.esPath, args, { windowsHide: true, signal });
  let stdout = "";
  let stderr = "";
  child.stdout.setEncoding("utf8");
  child.stderr.setEncoding("utf8");
  child.stdout.on("data", (chunk) => {
    stdout += chunk;
  });
  child.stderr.on("data", (chunk) => {
    stderr += chunk;
  });
  const exitCode = await new Promise((resolve, reject) => {
    child.on("error", reject);
    child.on("close", resolve);
  });
  if (exitCode !== 0 && stdout.trim() === "") {
    throw new Error(`es.exe exited with code ${exitCode}: ${stderr.trim().slice(0, 200)}`);
  }
  let parsed;
  try {
    parsed = JSON.parse(stdout);
  } catch {
    throw new Error(`es.exe returned non-JSON output: ${stdout.trim().slice(0, 200)}`);
  }
  const rows = Array.isArray(parsed) ? parsed : (parsed?.results ?? []);
  return normalizeResults({ totalResults: rows.length, results: rows }, limit);
}

//#endregion

//#region 工具实现

/** 执行一次检索并归一化结果；HTTP 失败且配置了 es.exe 时兜底。 */
async function search(client, query, scope, limit, signal) {
  try {
    return { ...(await queryHttp(client, query, limit, signal)), transport: "http" };
  } catch (httpError) {
    if (signal.aborted) throw httpError;
    if (client.esPath === "") {
      throw new Error(
        `Everything HTTP transport failed (${errorText(httpError)}). ` +
          "Check that Everything is running and its HTTP server is enabled, or configure `esPath` for the es.exe fallback.",
      );
    }
    try {
      const result = await queryEsExecutable(client, query, limit, scope, signal);
      return { ...result, transport: "es.exe" };
    } catch (esError) {
      throw new Error(
        `both Everything transports failed: HTTP -> ${errorText(httpError)}; es.exe -> ${errorText(esError)}`,
      );
    }
  }
}

//#endregion

//#region 插件入口

/**
 * 注册 `esearch` 工具。
 * @param ctx - 插件上下文（依赖 `tools` 服务）。
 * @param config - 已解析的插件配置。
 */
export function apply(ctx, config) {
  const resolved = config ?? {};
  const client = {
    endpoint: typeof resolved.endpoint === "string" && resolved.endpoint !== "" ? resolved.endpoint : DEFAULT_ENDPOINT,
    esPath: typeof resolved.esPath === "string" ? resolved.esPath : "",
  };
  const configuredScopes = Array.isArray(resolved.scopes) ? resolved.scopes : [];
  const defaultMaxResults = clampMaxResults(resolved.maxResults ?? DEFAULT_MAX_RESULTS);
  const timeoutMs = clampTimeout(resolved.timeoutMs ?? DEFAULT_TIMEOUT_MS);
  const guidanceOrder = resolved.guidanceOrder ?? DEFAULT_GUIDANCE_ORDER;
  if (!Number.isFinite(guidanceOrder)) {
    throw new Error(`tool-everything: guidanceOrder must be a finite number, got ${String(resolved.guidanceOrder)}`);
  }

  // 系统提示词指引段：让模型优先用 esearch，而不是 glob/grep/命令行检索。
  // 段序由 guidanceOrder 决定（默认 1390，即紧邻内置 tool:glob / tool:grep 之前）；
  // 当 esearch 不在可见工具集里时渲染为空串，等于不注册。
  ctx.systemPrompt.section({
    name: GUIDANCE_SECTION,
    order: guidanceOrder,
    text: ({ scope }) =>
      ctx.tools.get("esearch", scope) === undefined
        ? ""
        : esearchGuidanceText(ctx.tools.get("glob", scope) !== undefined, ctx.tools.get("grep", scope) !== undefined),
  });

  ctx.tools.register(
    defineTool({
      name: "esearch",
      description:
        "Search files with the Everything index (Windows). Fast whole-repository filename or content search, " +
        "far cheaper than walking the filesystem. A path scope is REQUIRED: pass `path`, or the tool falls back " +
        "to its configured scope / the session workspace and refuses to run unscoped (an unscoped content search " +
        "scans entire volumes). Content search hits Everything's content index when available and otherwise " +
        "falls back to a scope-limited on-demand scan. Returns absolute paths.",
      parameters: {
        pattern: {
          type: "string",
          required: true,
          description:
            "Filename pattern, or the content needle when `content` is true. Everything syntax is passed through " +
            "(wildcards *, ?, ext:cpp;h, !negation, \"...\" quoting); escape literal spaces in filenames with quotes.",
        },
        path: {
          type: "string",
          description:
            "Directory to search under. Defaults to the plugin's configured scope, then the session workspace. " +
            "Required in effect: a filesystem root (E:\\) is refused.",
        },
        content: {
          type: "boolean",
          description:
            "Search file CONTENTS instead of filenames (Everything `content:`). Slower than a filename search; " +
            "always scope it with a narrow `path`.",
        },
        regex: {
          type: "boolean",
          description: "Treat the pattern as a regular expression (Everything `regex:` modifier).",
        },
        caseSensitive: {
          type: "boolean",
          description: "Match case (Everything `case:` modifier).",
        },
        ext: {
          type: "string",
          description: "Restrict filename searches to extensions, semicolon separated (e.g. `cpp;h`).",
        },
        foldersOnly: {
          type: "boolean",
          description: "Return only folders (Everything `/ad`).",
        },
        rawQuery: {
          type: "boolean",
          description:
            "Treat `pattern` as a complete Everything query verbatim instead of wrapping it in content:/ext:. " +
            "Use for advanced syntax such as `content:<alpha beta> ext:cpp`.",
        },
        maxResults: {
          type: "number",
          description: `Maximum items to return (default ${DEFAULT_MAX_RESULTS}, hard cap ${HARD_MAX_RESULTS}).`,
        },
      },
      output: {
        // 原始 JSON Schema（canonical 输出契约要求），不是 schemastery 校验器。
        schema: {
          type: "object",
          additionalProperties: false,
          properties: {
            query: { type: "string", required: true },
            scope: { type: "string", required: true },
            transport: { type: "string", required: true },
            total: { type: "number", required: true },
            truncated: { type: "boolean", required: true },
            items: {
              type: "array",
              required: true,
              items: {
                type: "object",
                additionalProperties: false,
                properties: {
                  path: { type: "string", required: true },
                  kind: { type: "string", enum: ["file", "folder"] },
                  sizeBytes: { type: "number" },
                  dateModified: { type: "string" },
                },
              },
            },
          },
        },
        render(args, value) {
          const lines = [
            `scope: ${value.scope}`,
            `query: ${value.query}`,
            `transport: ${value.transport}`,
            `total: ${value.total}${value.truncated ? `, showing first ${value.items.length}` : ""}`,
          ];
          if (value.items.length === 0) {
            lines.push("", "no matches");
          } else {
            lines.push("");
            for (const item of value.items) {
              const suffix = item.kind === "folder" ? "  [folder]" : "";
              lines.push(`${item.path}${suffix}`);
            }
            if (value.truncated) {
              lines.push("", `(${value.total - value.items.length} more not shown; narrow the pattern or raise maxResults)`);
            }
          }
          return [{ type: "text", text: lines.join("\n") }];
        },
      },
      timeoutMs,
      isConcurrencySafe() {
        return true;
      },
      async execute(args, exec) {
        const rawQuery = args.rawQuery === true;
        const needScope = !(rawQuery && patternHasScope(args.pattern));
        const scope = needScope
          ? resolveScope(args.path, configuredScopes, exec)
          : undefined;
        const limit = clampMaxResults(args.maxResults ?? defaultMaxResults);
        const query = buildQuery(
          {
            pattern: args.pattern,
            content: args.content === true,
            regex: args.regex === true,
            caseSensitive: args.caseSensitive === true,
            ext: typeof args.ext === "string" ? args.ext : "",
            foldersOnly: args.foldersOnly === true,
            raw: rawQuery,
          },
          scope,
        );
        const signal = AbortSignal.any(
          exec.signal === undefined ? [AbortSignal.timeout(timeoutMs)] : [exec.signal, AbortSignal.timeout(timeoutMs)],
        );
        const result = await search(client, query, scope, limit, signal);
        return {
          query,
          scope: scope ?? "(query-scoped)",
          transport: result.transport,
          total: result.total,
          truncated: result.total > result.items.length,
          items: result.items,
        };
      },
    }),
  );
}

//#endregion
