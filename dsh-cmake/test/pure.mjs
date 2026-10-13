// dsh-cmake 纯函数单测：用 node:vm 从源码切片求值，无需安装 @deepseek-ai/* 依赖。
//
// 契约：lib/*.js 里 `//#region 插件入口` 之前的部分必须是不依赖任何 import 的纯逻辑。

import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import vm from 'node:vm';

const here = dirname(fileURLToPath(import.meta.url));
const libDir = join(here, '..', 'lib');

const MARKER = '//#region 插件入口';

/**
 * 把 lib 文件切到插件入口之前，剥掉 export 关键字后在 vm 里求值，返回导出的名字表。
 */
function loadPure(relPath) {
  const source = readFileSync(join(libDir, relPath), 'utf8');
  const cut = source.indexOf(MARKER);
  if (cut < 0) throw new Error(`${relPath}: missing marker ${MARKER}`);
  const body = source.slice(0, cut).replace(/^export\s+/gm, '');
  const context = vm.createContext({ console, Math, JSON, Object, Array, String, Number, RegExp, Map, Set, Error });
  const names = [...body.matchAll(/^(?:function|const|let|var)\s+([A-Za-z_$][\w$]*)/gm)].map((m) => m[1]);
  const script = new vm.Script(`${body}\n;({${names.join(',')}});`, { filename: relPath });
  return script.runInContext(context);
}

const D = loadPure('diagnostics.js');
const P = loadPure('presets.js');

let passed = 0;
let failed = 0;
const failures = [];

/**
 * @param {string} name
 * @param {() => void} fn
 */
function test(name, fn) {
  try {
    fn();
    passed += 1;
    process.stdout.write(`  ok  ${name}\n`);
  } catch (error) {
    failed += 1;
    failures.push({ name, error });
    process.stdout.write(`FAIL  ${name}\n      ${error.message}\n`);
  }
}

/**
 * @param {unknown} actual
 * @param {unknown} expected
 * @param {string} [what]
 */
function eq(actual, expected, what = 'value') {
  const a = JSON.stringify(actual);
  const b = JSON.stringify(expected);
  if (a !== b) throw new Error(`${what}: expected ${b}, got ${a}`);
}

function ok(cond, what) {
  if (!cond) throw new Error(what);
}

// ---------------------------------------------------------------- diagnostics

console.log('\ndiagnostics: MSVC / CMake / ninja 错误行提取');

test('MSVC error with line and column', () => {
  const d = D.parseDiagnostics('E:\\Doodle\\src\\a.cpp(123,45): error C2039: "foo": is not a member of "bar"');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'error', 'severity');
  eq(d[0].file, 'E:\\Doodle\\src\\a.cpp', 'file');
  eq(d[0].line, 123, 'line');
  eq(d[0].code, 'C2039', 'code');
  ok(d[0].message.includes('is not a member'), 'message');
});

test('MSVC fatal error without column', () => {
  const d = D.parseDiagnostics('src\\b.cpp(42): fatal error C1083: Cannot open include file: \'x.h\'');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'error', 'severity');
  eq(d[0].code, 'C1083', 'code');
  eq(d[0].line, 42, 'line');
});

test('MSVC warning is classified as warning, not error', () => {
  const d = D.parseDiagnostics('src\\c.cpp(7): warning C4100: unreferenced formal parameter');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'warning', 'severity');
  eq(d[0].code, 'C4100', 'code');
});

test('LINK error without file position', () => {
  const d = D.parseDiagnostics('LINK : fatal error LNK1181: cannot open input file \'foo.lib\'');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'error', 'severity');
  eq(d[0].code, 'LNK1181', 'code');
});

test('CMake Error at file:line', () => {
  const d = D.parseDiagnostics('CMake Error at CMakeLists.txt:12 (target_link_libraries):');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'error', 'severity');
  eq(d[0].file, 'CMakeLists.txt', 'file');
  eq(d[0].line, 12, 'line');
});

test('CMake Warning at file:line is warning', () => {
  const d = D.parseDiagnostics('CMake Warning at cmake/foo.cmake:9 (message):');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'warning', 'severity');
});

test('ninja build stopped', () => {
  const d = D.parseDiagnostics('ninja: build stopped: subcommand failed.');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'error', 'severity');
  ok(d[0].code.includes('ninja'), 'code mentions ninja');
});

test('FAILED: target line', () => {
  const d = D.parseDiagnostics('FAILED: src/CMakeFiles/foo.dir/a.cpp.obj');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'error', 'severity');
});

test('gcc/clang style error', () => {
  const d = D.parseDiagnostics('src/d.cpp:88:5: error: expected \';\' before \'}\'');
  eq(d.length, 1, 'count');
  eq(d[0].severity, 'error', 'severity');
  eq(d[0].line, 88, 'line');
});

test('benign output yields no diagnostics', () => {
  const text = [
    '-- Configuring done (12.3s)',
    '-- Generating done (0.4s)',
    '-- Build files have been written to: E:/Doodle/build/Ninja_debug',
    '[1/25] Building CXX object src/CMakeFiles/x.dir/a.cpp.obj',
    '[25/25] Linking CXX executable test_main.exe',
  ].join('\n');
  eq(D.parseDiagnostics(text).length, 0, 'count');
});

test('empty and non-string inputs are safe', () => {
  eq(D.parseDiagnostics('').length, 0, 'empty');
  eq(D.parseDiagnostics(undefined).length, 0, 'undefined');
  eq(D.parseDiagnostics(null).length, 0, 'null');
});

test('dedupe collapses identical diagnostics', () => {
  const one = D.parseDiagnostics('a.cpp(1,1): error C2039: x');
  const dup = D.dedupeDiagnostics([...one, ...one, ...one]);
  eq(dup.length, 1, 'count');
});

console.log('\ndiagnostics: decideOutcome 判定矩阵');

test('exit 0 + no errors -> ok', () => {
  const r = D.decideOutcome(0, []);
  eq(r.ok, true, 'ok');
});

test('exit nonzero -> not ok', () => {
  const r = D.decideOutcome(1, []);
  eq(r.ok, false, 'ok');
  ok(r.reason.includes('1'), 'reason mentions exit code');
});

test('exit 0 but error diagnostics -> not ok (conservative)', () => {
  const errs = D.parseDiagnostics('CMake Error at CMakeLists.txt:1 (project):');
  const r = D.decideOutcome(0, errs);
  eq(r.ok, false, 'ok');
});

test('exit nonzero + errors -> not ok and reason counts them', () => {
  const errs = D.parseDiagnostics([
    'a.cpp(1,1): error C2039: x',
    'b.cpp(2,2): error C2065: y',
  ].join('\n'));
  const r = D.decideOutcome(2, errs);
  eq(r.ok, false, 'ok');
  ok(r.reason.includes('2'), 'reason counts 2 errors');
});

test('warnings alone do not fail the run', () => {
  const warns = D.parseDiagnostics('a.cpp(1): warning C4100: unused');
  const r = D.decideOutcome(0, warns);
  eq(r.ok, true, 'ok');
});

test('aborted always fails', () => {
  const r = D.decideOutcome(0, [], { aborted: true });
  eq(r.ok, false, 'ok');
  eq(r.reason, 'aborted', 'reason');
});

test('null exit code fails', () => {
  const r = D.decideOutcome(null, []);
  eq(r.ok, false, 'ok');
});

console.log('\ndiagnostics: 输出截断与校验');

test('output truncation never emits a lone surrogate', () => {
  // 回归：cmake/vcpkg 输出里有非 BMP 字符（如 emoji、部分进度条字形）。
  // 若按 UTF-16 码元直接 slice，会切出孤立代理项；JSON 序列化后无法无损还原，
  // Host 会以 "value is not lossless JSON" 拒绝整次工具调用。
  const lone = /[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/;
  const face = '\u{1F600}';            // 4 字节，占 2 个 UTF-16 码元
  const noise = `build ok ${face} progress ${face} done ${face}`.repeat(400);

  for (const head of [1, 2, 3, 4, 5, 7, 4096]) {
    for (const tail of [1, 2, 3, 4, 5, 7, 4096]) {
      const out = D.truncateOutput(noise, head, tail);
      if (lone.test(out.head)) throw new Error(`head has lone surrogate (head=${head}, tail=${tail})`);
      if (lone.test(out.tail)) throw new Error(`tail has lone surrogate (head=${head}, tail=${tail})`);
      // 也要能无损往返
      eq(JSON.parse(JSON.stringify(out.head)), out.head, `head round trip (head=${head}, tail=${tail})`);
      eq(JSON.parse(JSON.stringify(out.tail)), out.tail, `tail round trip (head=${head}, tail=${tail})`);
    }
  }
});

test('output truncation honors the byte budget', () => {
  const s = 'a'.repeat(100) + '\u{1F600}'.repeat(100);   // 100 + 400 字节 = 500
  const out = D.truncateOutput(s, 200, 200);
  eq(out.truncated, true, 'is truncated');
  eq(D.utf8ByteLength(out.head) <= 200, true, `head within budget (got ${D.utf8ByteLength(out.head)})`);
  eq(D.utf8ByteLength(out.tail) <= 200, true, `tail within budget (got ${D.utf8ByteLength(out.tail)})`);
  eq(out.totalBytes, 500, 'totalBytes reports the true size');
});

test('short output is not truncated', () => {
  const t = D.truncateOutput('hello', 100, 100);
  eq(t.truncated, false, 'truncated');
  eq(t.head, 'hello', 'head');
  eq(t.tail, '', 'tail');
  eq(t.totalBytes, 5, 'totalBytes');
});

test('long output keeps head and tail, drops middle', () => {
  const head = 'H'.repeat(100);
  const mid = 'M'.repeat(5000);
  const tail = 'T'.repeat(100);
  const t = D.truncateOutput(head + mid + tail, 100, 100);
  eq(t.truncated, true, 'truncated');
  eq(t.head, head, 'head preserved');
  eq(t.tail, tail, 'tail preserved');
  ok(!t.head.includes('M') && !t.tail.includes('M'), 'middle dropped');
  eq(t.totalBytes, 5200, 'totalBytes');
});

test('utf8ByteLength counts multibyte correctly', () => {
  eq(D.utf8ByteLength('abc'), 3, 'ascii');
  eq(D.utf8ByteLength('中文'), 6, 'cjk');
  eq(D.utf8ByteLength('é'), 2, 'latin1 supplement');
});

test('isSafePresetName accepts CMake names', () => {
  eq(D.isSafePresetName('Ninja_debug'), true, 'Ninja_debug');
  eq(D.isSafePresetName('Ninja_RelWithDebInfo'), true, 'Ninja_RelWithDebInfo');
  eq(D.isSafePresetName('vs2022'), true, 'vs2022');
});

test('isSafePresetName rejects injection attempts', () => {
  eq(D.isSafePresetName('a; rm -rf /'), false, 'semicolon');
  eq(D.isSafePresetName('a b'), false, 'space');
  eq(D.isSafePresetName('--preset'), false, 'leading dash would parse as an option');
  eq(D.isSafePresetName('-DCMAKE_BUILD_TYPE=Debug'), false, 'leading dash cache var');
  eq(D.isSafePresetName(''), false, 'empty');
  eq(D.isSafePresetName(null), false, 'null');
  eq(D.isSafePresetName('x'.repeat(201)), false, 'too long');
});

test('isSafePresetName accepts names containing a dash after the first char', () => {
  eq(D.isSafePresetName('Ninja-RelWithDebInfo'), true, 'interior dash');
  eq(D.isSafePresetName('ninja.debug'), true, 'interior dot');
});

test('isSafeRelativeDir accepts build/Ninja_debug', () => {
  eq(D.isSafeRelativeDir('build/Ninja_debug'), true, 'forward slash');
  eq(D.isSafeRelativeDir('build\\Ninja_debug'), true, 'backslash');
  eq(D.isSafeRelativeDir('build'), true, 'simple');
});

test('isSafeRelativeDir rejects escapes and absolute paths', () => {
  eq(D.isSafeRelativeDir('../build'), false, 'dotdot');
  eq(D.isSafeRelativeDir('build/../../etc'), false, 'nested dotdot');
  eq(D.isSafeRelativeDir('C:/Windows'), false, 'drive letter');
  eq(D.isSafeRelativeDir('\\\\server\\share'), false, 'unc');
  eq(D.isSafeRelativeDir('/abs'), false, 'leading slash');
  eq(D.isSafeRelativeDir(''), false, 'empty');
});

// ------------------------------------------------------------------- presets

console.log('\npresets: 继承链与变量展开');

const DOC = {
  version: 6,
  cmakeMinimumRequired: { major: 3, minor: 20, version: '3.20.0' },
  configurePresets: [
    {
      name: 'Ninja_build',
      hidden: true,
      generator: 'Ninja',
      binaryDir: '${sourceDir}/build/${presetName}',
      cacheVariables: { CMAKE_C_COMPILER: 'cl', CMAKE_CXX_COMPILER: 'cl' },
    },
    { name: 'Ninja_debug', inherits: 'Ninja_build', cacheVariables: { CMAKE_BUILD_TYPE: 'Debug' } },
    { name: 'Ninja_RelWithDebInfo', inherits: 'Ninja_build', cacheVariables: { CMAKE_BUILD_TYPE: 'RelWithDebInfo' } },
  ],
  buildPresets: [
    { name: 'debug', configurePreset: 'Ninja_debug', hidden: true },
    { name: 'debug_doodle', inherits: 'debug', targets: ['doodle_kitsu_supplement', 'test_main'] },
  ],
};

test('resolvePresetChain walks inherits from base to leaf', () => {
  const r = P.resolvePresetChain(DOC, 'configure', 'Ninja_debug');
  eq(r.chain, ['Ninja_build', 'Ninja_debug'], 'chain');
  eq(r.missing, [], 'missing');
});

test('merged preset carries parent cacheVariables', () => {
  const r = P.resolvePresetChain(DOC, 'configure', 'Ninja_debug');
  eq(r.merged.generator, 'Ninja', 'generator from parent');
  eq(r.merged.cacheVariables.CMAKE_BUILD_TYPE, 'Debug', 'child wins');
  eq(r.merged.cacheVariables.CMAKE_C_COMPILER, 'cl', 'parent preserved');
});

test('build preset inherits targets resolution', () => {
  const r = P.resolvePresetChain(DOC, 'build', 'debug_doodle');
  eq(r.chain, ['debug', 'debug_doodle'], 'chain');
  eq(r.merged.targets, ['doodle_kitsu_supplement', 'test_main'], 'targets');
  eq(r.merged.configurePreset, 'Ninja_debug', 'configurePreset inherited');
});

test('missing preset is reported, not thrown', () => {
  const r = P.resolvePresetChain(DOC, 'configure', 'Nope');
  eq(r.missing, ['Nope'], 'missing');
  eq(r.merged, undefined, 'merged undefined');
});

test('inheritance cycle is detected', () => {
  const cyclic = {
    configurePresets: [
      { name: 'a', inherits: 'b' },
      { name: 'b', inherits: 'a' },
    ],
  };
  const r = P.resolvePresetChain(cyclic, 'configure', 'a');
  ok(r.missing.some((m) => m.includes('cycle')), 'cycle reported');
});

test('summarizePresets expands binaryDir with presetName', () => {
  const s = P.summarizePresets(DOC, { sourceDir: 'E:/Doodle' }, ['configure']);
  const ninjaDebug = s.presets.configure.find((p) => p.name === 'Ninja_debug');
  eq(ninjaDebug.binaryDir, 'E:/Doodle/build/Ninja_debug', 'binaryDir');
  eq(ninjaDebug.generator, 'Ninja', 'generator');
  eq(ninjaDebug.inherits, ['Ninja_build'], 'inherits chain');
  ok(ninjaDebug.cacheVariables.includes('CMAKE_BUILD_TYPE'), 'cache var keys');
});

test('summarizePresets marks hidden presets', () => {
  const s = P.summarizePresets(DOC, { sourceDir: 'E:/Doodle' }, ['configure']);
  eq(s.presets.configure.find((p) => p.name === 'Ninja_build').hidden, true, 'hidden');
  eq(s.presets.configure.find((p) => p.name === 'Ninja_debug').hidden, false, 'not hidden');
});

test('binaryDirForConfigurePreset resolves the real Doodle layout', () => {
  eq(P.binaryDirForConfigurePreset(DOC, 'Ninja_debug', 'E:/Doodle'), 'E:/Doodle/build/Ninja_debug', 'path');
});

test('configurePresetForBuildPreset resolves through inheritance', () => {
  eq(P.configurePresetForBuildPreset(DOC, 'debug_doodle'), 'Ninja_debug', 'via parent');
  eq(P.configurePresetForBuildPreset(DOC, 'nope'), undefined, 'unknown -> undefined');
});

// --------------------------------------------------- lossless JSON 回归
//
// 背景：工具的 output schema 用 additionalProperties:false 声明字段，
// 而 value 里一个「值为 undefined 的键」既不属于声明类型、也无法无损表示，
// 会被 Host 拒绝（"value is not lossless JSON"）——整次工具调用作废。
// 因此 summarizePresets 产出的每一行都不能残留 undefined 键。

/** 深查对象图里是否还有值为 undefined 的键；返回命中的路径列表。 */
function findUndefinedKeys(value, path = '$', hits = []) {
  if (Array.isArray(value)) {
    value.forEach((v, i) => findUndefinedKeys(v, `${path}[${i}]`, hits));
  } else if (value && typeof value === 'object') {
    for (const [k, v] of Object.entries(value)) {
      if (v === undefined) hits.push(`${path}.${k}`);
      else findUndefinedKeys(v, `${path}.${k}`, hits);
    }
  }
  return hits;
}

test('summarizePresets rows carry no undefined-valued keys', () => {
  const s = P.summarizePresets(DOC, { sourceDir: 'E:/Doodle' }, ['configure', 'build']);
  const hits = findUndefinedKeys(s);
  eq(hits.length, 0, `undefined keys: ${hits.join(', ')}`);
  // 反向确认测试本身有效：这些行确实会触发 pruneUndefined
  // （Ninja_debug 没有 description / targets，debug1 的 targets 也缺省）
  const rows = [...s.presets.configure, ...s.presets.build];
  const withoutOptional = rows.filter((r) => !('description' in r) || !('targets' in r));
  if (withoutOptional.length === 0) throw new Error('fixture no longer exercises optional fields');
});

test('every preset row survives a JSON round trip', () => {
  const s = P.summarizePresets(DOC, { sourceDir: 'E:/Doodle' }, ['configure', 'build']);
  const round = JSON.parse(JSON.stringify(s));
  eq(JSON.stringify(round), JSON.stringify(s), 'JSON round trip is lossless');
  // 显式检查：JSON.stringify 会静默丢弃 undefined 键，所以往返后键集合必须不变
  for (const [kind, rows] of Object.entries(s.presets)) {
    rows.forEach((row, i) => {
      const keys = Object.keys(row).sort().join(',');
      const rt = Object.keys(round.presets[kind][i]).sort().join(',');
      eq(rt, keys, `${kind}[${i}] (${row.name}) keys stable across JSON`);
    });
  }
});

// -------------------------------------------------------------------- report

console.log(`\n${passed} passed, ${failed} failed\n`);
if (failed > 0) {
  for (const f of failures) {
    console.log(`FAILED: ${f.name}\n${f.error.stack}\n`);
  }
  process.exit(1);
}
