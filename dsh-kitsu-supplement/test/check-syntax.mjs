// 语法检查 lib/ 下所有模块（ESM），replace 掉原来只检查 index.js 的 node --check。
import { readdirSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { dirname, join, relative } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const libRoot = join(here, "..", "lib");

function listFiles(dir) {
  const out = [];
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    const full = join(dir, entry.name);
    if (entry.isDirectory()) out.push(...listFiles(full));
    else if (entry.name.endsWith(".js")) out.push(full);
  }
  return out;
}

let failed = 0;
for (const file of listFiles(libRoot)) {
  try {
    execFileSync(process.execPath, ["--check", file], { stdio: "pipe" });
    process.stdout.write(`  ok  ${relative(libRoot, file)}\n`);
  } catch (error) {
    failed += 1;
    const detail = String(error.stderr ?? error.message).split("\n").slice(0, 6).join("\n");
    process.stdout.write(`FAIL  ${relative(libRoot, file)}\n${detail}\n`);
  }
}

if (failed > 0) {
  process.stdout.write(`\n${failed} file(s) failed syntax check\n`);
  process.exitCode = 1;
} else {
  process.stdout.write("\nall files pass syntax check\n");
}
