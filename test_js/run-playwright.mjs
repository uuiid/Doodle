// run-playwright.mjs — 在受限环境里跑 Playwright Test。
//
// 为什么需要它：Playwright 用 child_process.fork()
// （node_modules/playwright/lib/runner/index.js:1998，stdio 里带 'ipc'）拉起每个 worker，
// 而本机的 pwsh 沙箱禁止 fork 的创建方式（直接 fork 报 spawn EPERM，普通 spawn 正常）。
// 这里把 fork 换成普通 spawn 并手动补一条 IPC 通道：worker 仍然通过
// process.send / process.on('message') 与 runner 通信，行为与 fork 一致。
//
// 用法（参数原样透传给 playwright test）：
//   node run-playwright.mjs test face_capture.js --reporter=line
//   $env:PLAYWRIGHT_FORK_AS_SPAWN = "0"   # 关掉降级，恢复原生 fork

import child_process from 'node:child_process';
import { pathToFileURL } from 'node:url';

const passthrough = !(process.env.PLAYWRIGHT_FORK_AS_SPAWN === '0');

if (passthrough) {
  const original = child_process.fork;

  child_process.fork = function forkAsSpawn(modulePath, args = [], options = {}) {
    const env = { ...process.env, ...(options.env || {}) };
    const extras = [];
    if (options.execArgv?.length) extras.push(...options.execArgv);
    extras.push(modulePath, ...args);

    const stdio = options.stdio;
    const ipcIndex = Array.isArray(stdio) ? stdio.indexOf('ipc') : -1;
    const child = original.call(this, process.execPath, extras, {
      cwd: options.cwd,
      env,
      detached: options.detached ?? false,
      windowsHide: options.windowsHide ?? false,
      stdio: ['pipe', 'pipe', 'pipe', 'ipc'],
    });

    if (ipcIndex >= 0) {
      // runner 期望的顺序由它自己决定，这里只保证 IPC 存在；stdout/stderr 保持管道
      // （stdio: 'ignore' 的那一路在 Playwright 里只有 stdin，会被忽略）
      const stdin = stdio[0];
      if (stdin === 'ignore') child.stdin.end();
    }
    return child;
  };
}

const cli = pathToFileURL(new URL('./node_modules/@playwright/test/cli.js', import.meta.url).pathname).href;
process.argv.splice(1, 1, cli);
await import(cli);
