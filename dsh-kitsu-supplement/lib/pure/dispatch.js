// dsh-kitsu-supplement —— 纯逻辑文件的登记表
//
// test/pure.mjs 用 node:vm 逐个求值这些文件并合并导出，所以：
//   * 每份文件都必须是「零 import、零 ctx、零 await 宿主资源」的纯逻辑；
//   * 文件之间的同名依赖由 test/pure.mjs 按本数组顺序合并，前面的结果先注入；
//   * 新增纯逻辑文件时只改这里，不必再动测试里的硬编码路径。

export const PURE_FILES = ["common.js", "workspace.js", "start.js", "sql.js", "instances.js", "playwright.js"];
