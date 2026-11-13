// dsh-kitsu-supplement —— 进程内共享状态
//
// 实例表 registry、id 分配、playwright 运行互斥都放在这里：
// 只依赖纯逻辑，不依赖 process / server / tools，避免这几个模块互相循环引用。
// 可变绑定用 get/set 函数暴露，不导出 let 本身（ESM 的导入绑定只读）。

/** 本插件的实例表：serverId -> 实例记录。 */
export const registry = new Map();

/** 实例 id 自增种子。 */
let nextServerId = 1;

/** playwright 运行序号，只用于 runId。 */
let playwrightRunSeq = 1;

/**
 * 正在跑的 playwright 运行（同一时刻最多一个）。
 *
 * @type {{runId:string, startedAt:number, args:string[], cwd:string}|null}
 */
let playwrightRunning = null;

/**
 * 分配实例 id；只增不复用，保证 stop 后同一 id 不会指向新进程。
 *
 * @returns {string}
 */
export function allocateServerId() {
  const id = `kitsu-${nextServerId}`;
  nextServerId += 1;
  return id;
}

/**
 * 从状态文件认领实例后，把 id 种子推到已见过的最大值之后。
 *
 * @param {string} serverId
 */
export function advanceServerIdPast(serverId) {
  const parsed = /^kitsu-(\d+)$/.exec(serverId);
  if (parsed !== null) nextServerId = Math.max(nextServerId, Number.parseInt(parsed[1], 10) + 1);
}

/**
 * 分配 playwright run id（只用于日志与临时目录命名）。
 *
 * @returns {string}
 */
export function allocatePlaywrightRunId() {
  const stamp = new Date().toISOString().replace(/[-:TZ.]/g, "").slice(0, 14);
  const sequence = `${playwrightRunSeq++}`.padStart(3, "0");
  return `pw-${stamp}-${sequence}`;
}

/**
 * @returns {{runId:string, startedAt:number, args:string[], cwd:string}|null}
 */
export function getPlaywrightRunning() {
  return playwrightRunning;
}

/**
 * @param {{runId:string, startedAt:number, args:string[], cwd:string}|null} value
 */
export function setPlaywrightRunning(value) {
  playwrightRunning = value;
}

/**
 * 清空实例表（测试用）。
 */
export function resetRegistry() {
  registry.clear();
  nextServerId = 1;
}

/**
 * @param {string} serverId
 * @returns {any|undefined}
 */
export function getServer(serverId) {
  return registry.get(serverId);
}

/**
 * 按端口找仍存活的实例。
 *
 * @param {number} port
 * @returns {string[]}
 */
export function serversOnPort(port) {
  /** @type {string[]} */
  const ids = [];
  for (const [id, server] of registry) {
    if (server.exited === true || server.port !== port) continue;
    ids.push(id);
  }
  return ids;
}
