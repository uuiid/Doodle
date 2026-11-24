// 测试用模块解析钩子：插件目录下没有 node_modules，这里补两条解析：
//   1) dsh-doodle-workspace → 同仓库的真实文件；
//   2) @deepseek-ai/* → test/stubs/ 下的最小 stub（只够模块图求值，不实现宿主能力）。

import { existsSync } from "node:fs";
import { fileURLToPath, pathToFileURL } from "node:url";
import { dirname, resolve as resolvePath } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const workspaceEntry = resolvePath(here, "../../dsh-doodle-workspace/lib/workspace.js");

const STUB_DIR = resolvePath(here, "stubs");

const FILE_TARGETS = {
  "@deepseek-ai/dsh-tools": resolvePath(STUB_DIR, "dsh-tools.mjs"),
  "@deepseek-ai/schemastery": resolvePath(STUB_DIR, "schemastery.mjs"),
};

export function resolve(specifier, context, nextResolve) {
  if (specifier === "dsh-doodle-workspace") {
    if (!existsSync(workspaceEntry)) throw new Error(`共享包不存在：${workspaceEntry}`);
    return { url: pathToFileURL(workspaceEntry).href, shortCircuit: true };
  }
  const target = FILE_TARGETS[specifier];
  if (target !== undefined) {
    return { url: pathToFileURL(target).href, shortCircuit: true };
  }
  return nextResolve(specifier, context);
}
