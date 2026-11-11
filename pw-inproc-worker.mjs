// pw-inproc-worker.mjs — 让 Playwright Test 的 worker 在**当前进程里**跑。
//
// 为什么需要它：Playwright runner 用 child_process.fork(..., stdio 里带 'ipc') 拉起 worker
// （node_modules/playwright/lib/runner/index.js:1998）。受限环境（本机 pwsh 沙箱）禁止带
// 'ipc' 通道的子进程，直接抛 spawn EPERM，而普通 spawn 是允许的 —— 于是整套用例一条都跑不起来。
//
// 做法：不 fork。用 EventEmitter 冒充子进程（runner 只用到 on/once/off/emit/send/kill/pid/
// stdout/stderr），再对 Module._load 装一个只在 worker 模块里生效的钩子，把
// `require("node:process")` 换成假的 process（send / on('message') / on('disconnect') / env / exit），
// 然后在本进程里 import 真正的 worker 入口。Playwright 的 ready / __init__ / __dispatch__ /
// __stop__ 握手照旧，只是通道从 IPC 变成内存里的 EventEmitter。
//
// 用法：
//   node --import ./pw-inproc-worker.mjs node_modules/@playwright/test/cli.js test face_capture.js --reporter=line
// 关掉（回到原生 fork）：
//   $env:PW_INPROC_WORKER = "0"

import child_process from 'node:child_process';
import Module from 'node:module';
import { EventEmitter } from 'node:events';
import { pathToFileURL } from 'node:url';
import { Writable } from 'node:stream';

if (process.env.PW_INPROC_WORKER !== '0') install();

function install() {
  const loader = Module._load;

  Module._load = function patchedLoad(request, parent, isMain) {
    const result = loader.call(this, request, parent, isMain);

    if (request === 'node:child_process' || request === 'child_process') {
      return patchChildProcess(result);
    }
    if ((request === 'node:process' || request === 'process') && inWorkerModule(parent)) {
      return currentFakeProcess ?? result;
    }
    return result;
  };
}

let currentFakeProcess = null;
const patchedModules = new WeakSet();

function patchChildProcess(mod) {
  if (patchedModules.has(mod)) return mod;
  patchedModules.add(mod);

  const originalFork = mod.fork;
  mod.fork = function forkInProcess(modulePath, argsOrOptions, maybeOptions) {
    return startInProcess(modulePath, Array.isArray(argsOrOptions) ? argsOrOptions : [], Array.isArray(argsOrOptions) ? maybeOptions || {} : argsOrOptions || {}, originalFork);
  };
  return mod;
}

function inWorkerModule(parent) {
  if (!parent || typeof parent.filename !== 'string') return false;
  return parent.filename.includes(`${'playwright'}${sep()}lib${sep()}worker${sep()}`);
}

function sep() {
  return process.platform === 'win32' ? '\\' : '/';
}

function startInProcess(entryScript, args, options, originalFork) {
  const child = new EventEmitter();
  const fake = new EventEmitter();

  child.pid = process.pid;
  child.connected = true;
  child.killed = false;
  child.exitCode = null;
  child.signalCode = null;
  child.stdin = null;
  child.stdout = new Writable({
    write(chunk, _enc, cb) {
      process.stdout.write(chunk);
      cb();
    },
  });
  child.stderr = new Writable({
    write(chunk, _enc, cb) {
      process.stderr.write(chunk);
      cb();
    },
  });
  child.send = (message, callback) => {
    queueMicrotask(() => {
      try {
        fake.emit('message', structuredClone(message));
        callback?.(null);
      } catch (err) {
        callback?.(err);
      }
    });
    return true;
  };
  child.disconnect = () => {
    if (!child.connected) return;
    child.connected = false;
    fake.emit('disconnect');
  };
  child.kill = () => {
    child.disconnect();
    return true;
  };
  child.ref = () => child;
  child.unref = () => child;

  fake.send = (message, callback) => { console.error('[inproc] worker->runner', message && message.method); return child.send(message, callback); };
  fake.disconnect = () => child.disconnect();
  fake.pid = process.pid;
  fake.argv = process.argv;
  fake.platform = process.platform;
  fake.execPath = process.execPath;
  fake.env = { ...process.env, ...(options.env || {}) };
  fake.exit = (code = 0) => child.emit('exit', code, null);
  fake.cwd = () => process.cwd();
  fake.uptime = () => process.uptime();
  fake.hrtime = process.hrtime.bind(process);
  fake.memoryUsage = process.memoryUsage.bind(process);

  currentFakeProcess = fake;
  const previousEnv = { ...process.env };
  Object.assign(process.env, options.env || {});

  queueMicrotask(async () => {
    try {
      console.error('[inproc] start', entryScript);
      child.emit('spawn');
      await import(pathToFileURL(entryScript).href);
    } catch (err) {
      console.error('[inproc] import failed', err);
      child.stderr.write(String(err?.stack || err));
      Object.assign(process.env, previousEnv);
      child.emit('exit', 1, null);
    }
  });

  void args;
  void originalFork;
  return child;
}
