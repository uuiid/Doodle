// dsh-kitsu-supplement —— kitsu_server_sql：语句切分、目标库解析、执行
//
// 本文件只放纯逻辑：不依赖任何 import，也不访问 ctx / await 宿主资源。
// test/pure.mjs 读取 lib/pure/dispatch.js 里登记的文件逐个求值，契约与原先的纯逻辑区段一致。

import { MEMORY_DB, SQL_BUSY_TIMEOUT_MS, errorText, normalizePath, tailText, trimOrEmpty } from "./common.js";

/**
 * 把一段 sql 脚本切成单条语句。
 *
 * 轻量扫描而不是真解析：引号（' " ` 与 [ ] 标识符）、`--` 行注释、块注释里的分号都不算边界；
 * `CREATE TRIGGER ... BEGIN ... END;` 的内部语句靠 BEGIN/END 深度判断。
 * 分号之后只剩空白/注释的尾巴不算语句。
 *
 * @param {string} text
 * @returns {string[]}
 */
export function splitSqlStatements(text) {
  /** @type {string[]} */
  const statements = [];
  const source = String(text ?? "");
  let current = "";
  let quote = "";
  let depth = 0;
  let isTrigger = false;
  let isWordStart = true;
  for (let i = 0; i < source.length; i += 1) {
    const ch = source[i];
    const next = source[i + 1];
    if (quote !== "") {
      current += ch;
      isWordStart = false;
      if (ch === quote) {
        if (next === quote) {
          current += next;
          i += 1;
        } else {
          quote = "";
        }
      }
      continue;
    }
    if (ch === "'" || ch === '"' || ch === "`") {
      quote = ch;
      current += ch;
      isWordStart = false;
      continue;
    }
    if (ch === "[") {
      quote = "]";
      current += ch;
      isWordStart = false;
      continue;
    }
    if (ch === "-" && next === "-") {
      const end = source.indexOf("\n", i);
      if (end === -1) break;
      current += " ";
      i = end;
      isWordStart = true;
      continue;
    }
    if (ch === "/" && next === "*") {
      const end = source.indexOf("*/", i + 2);
      if (end === -1) break;
      current += " ";
      i = end + 1;
      isWordStart = true;
      continue;
    }
    if (/[A-Za-z_]/.test(ch) && isWordStart) {
      let word = "";
      let j = i;
      while (j < source.length && /[A-Za-z0-9_]/.test(source[j])) {
        word += source[j];
        j += 1;
      }
      const upper = word.toUpperCase();
      if (upper === "CREATE" && current.trim() === "" && !isTrigger) isTrigger = true;
      else if (isTrigger && upper === "BEGIN") depth += 1;
      else if (isTrigger && upper === "END" && depth > 0) depth -= 1;
      current += word;
      i = j - 1;
      isWordStart = false;
      continue;
    }
    if (ch === ";" && depth === 0) {
      const trimmed = current.trim();
      if (trimmed !== "") statements.push(trimmed);
      current = "";
      isTrigger = false;
      isWordStart = true;
      continue;
    }
    current += ch;
    isWordStart = !/[A-Za-z0-9_$]/.test(ch);
  }
  const tail = current.trim();
  if (tail !== "") statements.push(tail);
  return statements;
}

/**
 * 语句首关键字，用于拒绝跨库操作。
 *
 * @param {string} statement
 * @returns {string}
 */

/**
 * 语句首关键字，用于拒绝跨库操作。
 *
 * @param {string} statement
 * @returns {string}
 */
export function sqlVerb(statement) {
  const match = /^[A-Za-z]+/.exec(statement.trim());
  return match === null ? "" : match[0].toUpperCase();
}

/**
 * 校验一条语句能否交给本工具执行。
 *
 * ATTACH/DETACH 会把写入引到另一个库文件，直接拒绝。
 *
 * @param {string} statement
 * @returns {string|undefined}
 */

/**
 * 校验一条语句能否交给本工具执行。
 *
 * ATTACH/DETACH 会把写入引到另一个库文件，直接拒绝。
 *
 * @param {string} statement
 * @returns {string|undefined}
 */
export function rejectSqlStatement(statement) {
  const verb = sqlVerb(statement);
  if (verb === "ATTACH" || verb === "DETACH") {
    return `不允许 ${verb}：本工具只写目标库文件，跨库操作请手工处理。`;
  }
  return undefined;
}

/**
 * `0.0.0.0:50025` / `[::]:50025` -> 50025。
 *
 * @param {string} local
 * @returns {number|undefined}
 */

/**
 * 校验 kitsu_server_sql 的参数。
 *
 * @param {any} args
 * @returns {string[]}
 */
export function validateSqlArgs(args) {
  /** @type {string[]} */
  const problems = [];
  const hasSql = trimOrEmpty(args.sql) !== "";
  const files = Array.isArray(args.files) ? args.files : [];
  if (!hasSql && files.length === 0) problems.push("必须给 sql 片段或至少一个 files 路径。");
  if (args.files !== undefined && !Array.isArray(args.files)) problems.push("files 必须是字符串数组。");
  for (const file of files) {
    if (typeof file !== "string" || file.trim() === "") {
      problems.push(`files 含非法项 ${JSON.stringify(file)}（必须是非空绝对路径）。`);
    } else if (!/^([A-Za-z]:[\\/]|\\\\[^\\/]+)/.test(file)) {
      problems.push(`files 项必须是绝对路径，收到 ${JSON.stringify(file)}。`);
    } else if (!file.toLowerCase().endsWith(".sql")) {
      problems.push(`files 项必须以 .sql 结尾，收到 ${JSON.stringify(file)}。`);
    }
  }
  if (args.dbPath !== undefined) {
    const dbPath = trimOrEmpty(args.dbPath);
    if (dbPath === "") problems.push("dbPath 不能是空串；不给就沿用实例或插件默认值。");
    else if (dbPath === MEMORY_DB) problems.push(`${MEMORY_DB} 是进程内存库，外部写不进去；请给服务端传一个磁盘 dbPath。`);
    else if (!/^([A-Za-z]:[\\/]|\\\\[^\\/]+)/.test(dbPath)) problems.push(`dbPath 必须是绝对路径，收到 ${JSON.stringify(dbPath)}。`);
  }
  if (args.serverId !== undefined && trimOrEmpty(args.serverId) === "") problems.push("serverId 不能是空串。");
  return problems;
}

/**
 * 决定这次写入哪个库：参数 > 实例记录的 dbPath > 插件默认 db_path。
 *
 * @param {object} deps
 * @param {any} deps.args
 * @param {any} deps.config
 * @param {Map<string, any>} deps.registry
 * @returns {{dbPath:string|undefined, serverId:string|undefined, problem?:string}}
 */

/**
 * 决定这次写入哪个库：参数 > 实例记录的 dbPath > 插件默认 db_path。
 *
 * @param {object} deps
 * @param {any} deps.args
 * @param {any} deps.config
 * @param {Map<string, any>} deps.registry
 * @returns {{dbPath:string|undefined, serverId:string|undefined, problem?:string}}
 */
export function resolveSqlTarget({ args, config, registry }) {
  const wantedId = trimOrEmpty(args.serverId);
  /** @type {any} */
  let record;
  if (wantedId !== "") {
    record = registry.get(wantedId);
    if (record === undefined) return { dbPath: undefined, serverId: wantedId, problem: `找不到实例 ${wantedId}（先用 kitsu_server_status 查看）。` };
  }
  const explicit = trimOrEmpty(args.dbPath);
  const fromRecord = typeof record?.dbPath === "string" ? record.dbPath : "";
  const fallback = trimOrEmpty(config?.defaultConfigPaths?.db_path);
  const dbPath = explicit !== "" ? normalizePath(explicit) : fromRecord !== "" ? fromRecord : fallback;
  if (dbPath === "") {
    return {
      dbPath: undefined,
      serverId: wantedId === "" ? undefined : wantedId,
      problem: "无法确定数据库路径：请传 dbPath，或先起一个带 dbPath 的实例，或配置 defaultConfigPaths.db_path。",
    };
  }
  if (dbPath === MEMORY_DB) {
    return {
      dbPath,
      serverId: wantedId === "" ? undefined : wantedId,
      problem:
        `目标是内存库 ${MEMORY_DB}（服务端启动时没拿到 --dbPath，见 kitsu_supplement.cpp:175-178）：外部进程写不进去。` +
        "请用 dbPath 参数重启服务端后再填充数据。",
    };
  }
  return { dbPath, serverId: wantedId === "" ? undefined : wantedId };
}

/**
 * 在磁盘库上执行一批 sql。服务端对磁盘库调 journal_mode(wal)（sqlite_database.cpp:1335），
 * 但实测新库仍可能是 rollback journal；两种模式下外部提交的写入都不需要停服或重启。
 *
 * 连接级 PRAGMA 与服务端保持一致（sqlite_database.cpp:1310-1312），
 * 否则可能写出服务端读不了的库（尤其外键）。
 *
 * @param {{dbPath:string, batches:{label:string, sql:string}[], dryRun:boolean}} spec
 * @returns {Promise<{ok:boolean, statements:number, changes:number, results:any[], error?:string}>}
 */

/**
 * 在磁盘库上执行一批 sql。服务端对磁盘库调 journal_mode(wal)（sqlite_database.cpp:1335），
 * 但实测新库仍可能是 rollback journal；两种模式下外部提交的写入都不需要停服或重启。
 *
 * 连接级 PRAGMA 与服务端保持一致（sqlite_database.cpp:1310-1312），
 * 否则可能写出服务端读不了的库（尤其外键）。
 *
 * @param {{dbPath:string, batches:{label:string, sql:string}[], dryRun:boolean}} spec
 * @returns {Promise<{ok:boolean, statements:number, changes:number, results:any[], error?:string}>}
 */
export async function runSql({ dbPath, batches, dryRun }) {
  const { DatabaseSync } = await import("node:sqlite");
  /** @type {any} */
  let db;
  try {
    db = new DatabaseSync(dbPath, { timeout: SQL_BUSY_TIMEOUT_MS });
  } catch (error) {
    return { ok: false, statements: 0, changes: 0, results: [], error: `打不开数据库 ${dbPath}：${errorText(error)}` };
  }
  /** @type {any[]} */
  const results = [];
  let statements = 0;
  let changes = 0;
  let inTransaction = false;
  try {
    db.exec("PRAGMA foreign_keys = ON;");
    db.exec("PRAGMA synchronous = NORMAL;");
    db.exec("PRAGMA recursive_triggers = ON;");
    db.exec("BEGIN IMMEDIATE;");
    inTransaction = true;
    for (const batch of batches) {
      const list = splitSqlStatements(batch.sql);
      for (let index = 0; index < list.length; index += 1) {
        const statement = list[index];
        const rejection = rejectSqlStatement(statement);
        if (rejection !== undefined) throw new Error(`${batch.label} 第 ${index + 1} 条：${rejection}`);
        statements += 1;
        try {
          const stmt = db.prepare(statement);
          const rowChanges = dryRun ? 0 : Number(stmt.run()?.changes ?? 0);
          changes += rowChanges;
          results.push({ source: batch.label, index: index + 1, verb: sqlVerb(statement), changes: rowChanges });
        } catch (error) {
          return {
            ok: false,
            statements,
            changes: 0,
            results,
            error: `${batch.label} 第 ${index + 1} 条失败：${errorText(error)}\nSQL: ${tailText(statement, 400)}`,
          };
        }
      }
    }
    db.exec(dryRun ? "ROLLBACK;" : "COMMIT;");
    inTransaction = false;
    return { ok: true, statements, changes, results };
  } catch (error) {
    return { ok: false, statements, changes: 0, results, error: errorText(error) };
  } finally {
    if (inTransaction) {
      try {
        db.exec("ROLLBACK;");
      } catch {
        /* 事务可能已经因错误被 SQLite 回滚 */
      }
    }
    try {
      db.close();
    } catch {
      /* 连接已经不可用，无需再抛 */
    }
  }
}

/**
 * 从 stdout 里找监听协程打印的端口行：第一行纯数字就是端口。
 *
 * 端口以入参为准（http_listener.cpp:35 仍会把真实端口打到 stdout），
 * 这里只用于失败诊断时把 stdout 里报告过的端口一并报给调用方。
 *
 * @param {string} text
 * @returns {number|undefined}
 */

/**
 * 从 stdout 里找监听协程打印的端口行：第一行纯数字就是端口。
 *
 * 端口以入参为准（http_listener.cpp:35 仍会把真实端口打到 stdout），
 * 这里只用于失败诊断时把 stdout 里报告过的端口一并报给调用方。
 *
 * @param {string} text
 * @returns {number|undefined}
 */
export function findPortLine(text) {
  for (const raw of String(text ?? "").split(/\r?\n/)) {
    const line = raw.trim();
    if (line === "") continue;
    if (!/^[0-9]{1,5}$/.test(line)) return undefined;
    const port = Number.parseInt(line, 10);
    return port >= 1 && port <= 65535 ? port : undefined;
  }
  return undefined;
}
