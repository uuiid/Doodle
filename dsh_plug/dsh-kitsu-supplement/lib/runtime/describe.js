// dsh-kitsu-supplement —— 实例的对外描述
//
// 把内部 record 投影成 kitsu_server_status 的单个 server 条目。

import { baseUrlOf, sanitizeForJson } from "../pure/common.js";

/**
 * @param {string} serverId
 * @param {any} record
 * @returns {any}
 */
export function describeServer(serverId, record) {
  const alive = record.exited !== true;
  return sanitizeForJson({
    serverId,
    running: alive,
    mode: record.mode,
    port: record.port,
    pid: record.pid,
    baseUrl: typeof record.port === "number" ? baseUrlOf(record.port) : undefined,
    cwd: record.cwd,
    exePath: record.exePath,
    exeKind: record.exeKind,
    configPath: record.configPath,
    command: record.command,
    uptimeMs: alive && typeof record.startedAt === "number" ? Date.now() - record.startedAt : 0,
    exitCode: record.exited === true && typeof record.exitCode === "number" ? record.exitCode : undefined,
    exitSignal: record.exited === true && typeof record.exitSignal === "string" ? record.exitSignal : undefined,
    adopted: record.adopted === true ? true : undefined,
  });
}
