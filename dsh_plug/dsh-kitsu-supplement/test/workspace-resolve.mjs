// 端到端解析冒烟：真正 apply() 插件、注册工具，再驱动「工作区 → 分支 → exe」解析链。
//
// 与 load-graph.mjs 一样用 loader hook 处理模块说明符：同仓库的 dsh-doodle-workspace
// 按真实文件加载（插件目录下没有 node_modules，裸包名解析不到），@deepseek-ai/* 用最小 stub。
// 整个过程不 spawn 任何进程。

import { registerHooks } from "node:module";
import { join } from "node:path";

const hooks = await import("./resolve-hooks.mjs");
registerHooks({ resolve: hooks.resolve });

const WORKSPACE = await import("dsh-doodle-workspace");
const mod = await import("../lib/index.js");

const problems = [];
const tools = new Map();

const fakeCtx = {
  get: () => undefined,
  tools: {
    register(definition) {
      tools.set(definition.name, definition);
      return () => {};
    },
  },
  systemPrompt: { section() {} },
  effect(fn) {
    fn();
  },
  subprocess: {
    spawn() {
      throw new Error("本检查不 spawn 进程");
    },
  },
};

await mod.apply(fakeCtx, {
  defaultConfigPaths: { db_path: "", kitsu_front_end_path: "", root: "", public_url_port: "" },
  stateDir: join(process.env.TEMP || ".", "kitsu-shared-workspace-check"),
});

for (const name of ["kitsu_server_start", "kitsu_server_status", "kitsu_server_stop", "kitsu_server_sql"]) {
  if (!tools.has(name)) problems.push(`未注册的工具: ${name}`);
}

const startTool = tools.get("kitsu_server_start");
const params = startTool?.parameters ?? {};
if (params.exeName === undefined) problems.push("kitsu_server_start 缺少 exeName 参数");
if (params.exePath !== undefined) problems.push("kitsu_server_start 仍然暴露 exePath 参数");
if (!String(startTool?.description ?? "").includes("WORKSPACE")) {
  problems.push("kitsu_server_start 的说明没有交代「按会话工作区解析」");
}

const SEP = String.fromCharCode(92);
const ROOT = `E:${SEP}Doodle`;
const buildDir = WORKSPACE.buildDirOf({ sourceDir: ROOT, branch: "master", presetName: WORKSPACE.KITSU_PRESET_DIR });
const exePath = WORKSPACE.exePathOf({ buildDir, exeName: WORKSPACE.DEFAULT_TEST_EXE_NAME });
const expected = `E:${SEP}build${SEP}master${SEP}Ninja_debug${SEP}bin${SEP}${WORKSPACE.DEFAULT_TEST_EXE_NAME}`;
if (exePath !== expected) problems.push(`exe 解析结果不对：${JSON.stringify(exePath)}`);

const judged = WORKSPACE.judgeExeResolution({
  workspaceRoot: ROOT,
  branch: "",
  exeName: WORKSPACE.DEFAULT_TEST_EXE_NAME,
  buildDir: "",
  exePath: "",
  exists: false,
});
if (judged.ok !== false) problems.push("缺少分支时 judgeExeResolution 应该判失败");
if (!judged.problems.some((item) => item.includes("git 分支"))) problems.push("失败原因没有点出 git 分支");

if (problems.length > 0) {
  for (const problem of problems) process.stdout.write(`PROBLEM ${problem}\n`);
  process.exitCode = 1;
} else {
  process.stdout.write(`OK 解析链: ${exePath}\n`);
  process.stdout.write("OK kitsu_server_start 只收 exeName，说明里交代了按会话工作区解析\n");
  process.stdout.write("OK 解析失败时返回问题清单\n");
}
