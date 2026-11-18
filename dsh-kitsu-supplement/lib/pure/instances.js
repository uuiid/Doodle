// dsh-kitsu-supplement —— 实例表与槽位、快照
//
// 本文件只放纯逻辑：不依赖任何 import，也不访问 ctx / await 宿主资源。
// test/pure.mjs 读取 lib/pure/dispatch.js 里登记的文件逐个求值，契约与原先的纯逻辑区段一致。

import { DEFAULT_MODE, baseUrlOf, errorText, isPlainObject, normalizePath, parsePort, trimOrEmpty } from "./common.js";

/** 这些名字在本文件内就地定义，不跨文件 import：模块图的导出名不是加载期契约。 */

const MODES = ["production", "local"];

export const INSTANCE_STATE_VERSION = 1;

export const INSTANCE_STATE_FILE = "instances.json";

export const MAX_INSTANCES_PER_MODE = 1;

/**
 * 实例的模式分组：只算在跑的实例，已退出的只作历史。
 *
 * @param {Map<string, any>} records
 * @returns {Map<string, string[]>}
 */

/**
 * 实例的模式分组：只算在跑的实例，已退出的只作历史。
 *
 * @param {Map<string, any>} records
 * @returns {Map<string, string[]>}
 */
export function groupRunningByMode(records) {
  /** @type {Map<string, string[]>} */
  const groups = new Map(MODES.map((mode) => [mode, []]));
  for (const [id, record] of records ?? []) {
    if (record?.exited === true) continue;
    if (!MODES.includes(record?.mode)) continue;
    groups.get(record.mode).push(id);
  }
  return groups;
}

/**
 * 每个模式的槽位：每模式最多一个在跑实例。
 *
 * @param {Map<string, any>} records
 * @returns {{slots:Record<string, any|null>, usage:any, available:string[], duplicates:{mode:string, serverIds:string[]}[]}}
 */

/**
 * 每个模式的槽位：每模式最多一个在跑实例。
 *
 * @param {Map<string, any>} records
 * @returns {{slots:Record<string, any|null>, usage:any, available:string[], duplicates:{mode:string, serverIds:string[]}[]}}
 */
export function slotSummary(records) {
  const groups = groupRunningByMode(records);
  /** @type {Record<string, any|null>} */
  const slots = {};
  /** @type {string[]} */
  const available = [];
  /** @type {{mode:string, serverIds:string[]}[]} */
  const duplicates = [];
  /** @type {{limitPerMode:number, running:number, local:number, production:number}} */
  const usage = { limitPerMode: MAX_INSTANCES_PER_MODE, running: 0, local: 0, production: 0 };
  for (const mode of MODES) {
    const ids = groups.get(mode) ?? [];
    usage[mode] = ids.length;
    usage.running += ids.length;
    if (ids.length === 0) {
      slots[mode] = null;
      available.push(mode);
    } else if (ids.length > MAX_INSTANCES_PER_MODE) {
      duplicates.push({ mode, serverIds: [...ids] });
      slots[mode] = null;
    } else {
      const record = records.get(ids[0]);
      slots[mode] = {
        serverId: ids[0],
        port: record?.port,
        baseUrl: typeof record?.port === "number" ? baseUrlOf(record.port) : undefined,
        pid: record?.pid,
        uptimeMs: typeof record?.startedAt === "number" ? Date.now() - record.startedAt : 0,
      };
    }
  }
  return { slots, usage, available, duplicates };
}

/**
 * 某个模式当前被谁占着（在跑）。没有就返回 undefined。
 *
 * @param {Map<string, any>} records
 * @param {string} mode
 * @returns {string|undefined}
 */

/**
 * 某个模式当前被谁占着（在跑）。没有就返回 undefined。
 *
 * @param {Map<string, any>} records
 * @param {string} mode
 * @returns {string|undefined}
 */
export function findSlotHolder(records, mode) {
  if (!MODES.includes(mode)) return undefined;
  for (const [id, record] of records ?? []) {
    if (record?.exited === true) continue;
    if (record?.mode === mode) return id;
  }
  return undefined;
}

/**
 * 起服务前的准入判定：同一模式最多一个实例，replaceExisting 才允许替换。
 *
 * @param {{records:Map<string, any>, mode:string, replaceExisting?:boolean}} spec
 * @returns {{kind:string, holderId?:string, holderPort?:number, holderUptimeMs?:number, problem?:string}}
 */

/**
 * 起服务前的准入判定：同一模式最多一个实例，replaceExisting 才允许替换。
 *
 * @param {{records:Map<string, any>, mode:string, replaceExisting?:boolean}} spec
 * @returns {{kind:string, holderId?:string, holderPort?:number, holderUptimeMs?:number, problem?:string}}
 */
export function validateSlotRequest({ records, mode, replaceExisting }) {
  const holderId = findSlotHolder(records, mode);
  if (holderId === undefined) return { kind: "free" };
  const holder = records.get(holderId);
  const holderUptimeMs = typeof holder?.startedAt === "number" ? Date.now() - holder.startedAt : 0;
  if (replaceExisting === true) {
    return { kind: "replace", holderId, holderPort: holder?.port, holderUptimeMs };
  }
  return {
    kind: "occupied",
    holderId,
    holderPort: holder?.port,
    holderUptimeMs,
    problem:
      `${mode} 模式已经有一个实例在跑（${holderId}，端口 ${holder?.port ?? "?"}，已运行 ${holderUptimeMs} ms）。` +
      "同一模式最多一个：先 kitsu_server_stop 停掉它，或传 replaceExisting: true 让本次启动替换掉它。",
  };
}

/**
 * 实例表快照文件路径。instancesFile 给绝对路径就整个覆盖，否则落在 stateDir（空 = 系统临时目录）下。
 *
 * @param {{stateDir?:string, instancesFile?:string, tmpRoot:string}} spec
 * @returns {string}
 */

/**
 * 实例表快照文件路径。instancesFile 给绝对路径就整个覆盖，否则落在 stateDir（空 = 系统临时目录）下。
 *
 * @param {{stateDir?:string, instancesFile?:string, tmpRoot:string}} spec
 * @returns {string}
 */
export function instancesPathOf({ stateDir, instancesFile, tmpRoot }) {
  const base = trimOrEmpty(stateDir) === "" ? normalizePath(tmpRoot) : normalizePath(stateDir);
  const explicitFile = trimOrEmpty(instancesFile);
  if (explicitFile !== "") {
    const normalized = normalizePath(explicitFile);
    return /^([A-Za-z]:[\\/]|\\\\[^\\/]+)/.test(normalized) ? normalized : `${base}\\${normalized}`;
  }
  return `${base}\\${INSTANCE_STATE_FILE}`;
}

/**
 * 把实例表投影成可落盘的快照（只留元数据，不存句柄）。
 *
 * @param {Map<string, any>} records
 * @returns {any}
 */

/**
 * 把实例表投影成可落盘的快照（只留元数据，不存句柄）。
 *
 * @param {Map<string, any>} records
 * @returns {any}
 */
export function serializeInstances(records) {
  /** @type {any[]} */
  const servers = [];
  for (const [serverId, record] of records ?? []) {
    servers.push({
      serverId,
      mode: record?.mode,
      port: record?.port,
      pid: record?.pid,
      exePath: record?.exePath,
      cwd: record?.cwd,
      dbPath: record?.dbPath,
      command: record?.command,
      startedAt: record?.startedAt,
      exited: record?.exited === true,
      exitCode: typeof record?.exitCode === "number" ? record.exitCode : undefined,
    });
  }
  return { version: INSTANCE_STATE_VERSION, savedAt: Date.now(), servers };
}

/**
 * 解析快照文件。损坏或版本不符都返回 problem，调用方降级为「没有历史」。
 *
 * @param {string} text
 * @returns {{records:any[], problem?:string}}
 */

/**
 * 解析快照文件。损坏或版本不符都返回 problem，调用方降级为「没有历史」。
 *
 * @param {string} text
 * @returns {{records:any[], problem?:string}}
 */
export function parseInstancesState(text) {
  let parsed;
  try {
    parsed = JSON.parse(String(text ?? ""));
  } catch (error) {
    return { records: [], problem: `快照不是合法 JSON：${errorText(error)}` };
  }
  if (!isPlainObject(parsed)) return { records: [], problem: "快照不是对象。" };
  if (parsed.version !== INSTANCE_STATE_VERSION) {
    return { records: [], problem: `快照版本 ${String(parsed.version)} 与期望的 ${INSTANCE_STATE_VERSION} 不一致。` };
  }
  if (!Array.isArray(parsed.servers)) return { records: [], problem: "快照缺少 servers 数组。" };
  /** @type {any[]} */
  const records = [];
  for (const item of parsed.servers) {
    if (!isPlainObject(item)) continue;
    const serverId = trimOrEmpty(item.serverId);
    if (serverId === "") continue;
    records.push({
      serverId,
      mode: MODES.includes(item.mode) ? item.mode : DEFAULT_MODE,
      port: parsePort(item.port),
      pid: typeof item.pid === "number" ? item.pid : undefined,
      exePath: trimOrEmpty(item.exePath) || undefined,
      cwd: trimOrEmpty(item.cwd) || undefined,
      dbPath: trimOrEmpty(item.dbPath) || undefined,
      command: trimOrEmpty(item.command) || undefined,
      startedAt: typeof item.startedAt === "number" ? item.startedAt : Date.now(),
      exited: item.exited === true,
      exitCode: typeof item.exitCode === "number" ? item.exitCode : null,
    });
  }
  return { records };
}

/**
 * 快照里的实例是否还活着：端口上能查到它的 pid 才算。
 *
 * @param {any} item
 * @param {Map<number, number[]>} livePidsByPort
 * @returns {boolean}
 */

/**
 * 快照里的实例是否还活着：端口上能查到它的 pid 才算。
 *
 * @param {any} item
 * @param {Map<number, number[]>} livePidsByPort
 * @returns {boolean}
 */
export function instanceLooksAlive(item, livePidsByPort) {
  if (typeof item?.port !== "number" || typeof item?.pid !== "number") return false;
  const pids = livePidsByPort?.get(item.port);
  return Array.isArray(pids) && pids.includes(item.pid);
}

/**
 * 合并快照与实时探测：快照里「在跑但本进程没有句柄、且端口上查不到它的 pid」的实例标成 exited。
 * 本进程认识（live 里有）的 serverId 一律跳过，实时记录优先。
 *
 * @param {{saved:any[], live:Map<string, any>, livePidsByPort:Map<number, number[]>}} spec
 * @returns {any[]}
 */

/**
 * 合并快照与实时探测：快照里「在跑但本进程没有句柄、且端口上查不到它的 pid」的实例标成 exited。
 * 本进程认识（live 里有）的 serverId 一律跳过，实时记录优先。
 *
 * @param {{saved:any[], live:Map<string, any>, livePidsByPort:Map<number, number[]>}} spec
 * @returns {any[]}
 */
export function mergeInstancesState({ saved, live, livePidsByPort }) {
  /** @type {any[]} */
  const merged = [];
  for (const item of saved ?? []) {
    if (live?.has(item.serverId)) continue;
    const alive = item.exited === true ? false : instanceLooksAlive(item, livePidsByPort);
    merged.push({ ...item, exited: !alive });
  }
  return merged;
}
