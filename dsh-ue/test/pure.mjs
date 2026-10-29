// test/pure.mjs —— 对 lib/ 里的纯函数做快速回归（不接触 DSH 服务、不启动构建）。
//
// 运行：node test/pure.mjs

import assert from 'node:assert/strict';

import {
  decideOutcome,
  dedupeDiagnostics,
  parseDiagnostics,
  readUbtSummary,
  truncateOutput,
} from '../lib/parse.js';
import {
  buildLaunchArgv,
  isForwardableUbtArg,
  isSafeRelativePath,
  moduleTargetName,
  readEngineAssociation,
  resolveTarget,
} from '../lib/target.js';

let passed = 0;

/**
 * @param {string} label
 * @param {() => void} body
 */
function test(label, body) {
  try {
    body();
    passed += 1;
    console.log(`ok   ${label}`);
  } catch (error) {
    console.error(`FAIL ${label}`);
    throw error;
  }
}

// ------------------------------------------------------------------ 解析：MSVC

test('解析 MSVC 编译错误的位置与代码', () => {
  const lines = [
    '正在编译...',
    "D:\\p\\Foo.cpp(123,45): error C2039: 'Bar': is not a member of 'FBaz'",
    'D:\\p\\Foo.cpp(123,45): note: see declaration of FBaz',
    'D:\\p\\Foo.cpp(7,1): warning C4100: unreferenced formal parameter',
  ].join('\n');
  const parsed = parseDiagnostics(lines);
  assert.equal(parsed.length, 3);
  const [error, note, warning] = parsed;
  assert.equal(error.severity, 'error');
  assert.equal(error.file, 'D:\\p\\Foo.cpp');
  assert.equal(error.line, 123);
  assert.equal(error.column, 45);
  assert.equal(error.code, 'C2039');
  assert.equal(note.severity, 'note');
  assert.equal(note.code, 'MSVC/note');
  assert.equal(warning.severity, 'warning');
  assert.equal(warning.code, 'C4100');
});

test('UBT 的 "模块编译失败" 摘要行记为 UBT/CompileFailed', () => {
  const parsed = parseDiagnostics([
    'LogCompile     : error: D:\\p\\Foo.cpp',
    '   fatal error C1083: 无法打开包含文件',
    'LogCompile     : 一个或多个模块编译失败（1 个）。详见日志。',
  ].join('\n'));
  const summary = parsed.find((item) => item.code === 'UBT/CompileFailed');
  assert.ok(summary, '应解析出“模块编译失败”这一行');
  assert.match(summary.message, /模块编译失败/);

  const fileError = parsed.find((item) => item.message.includes('Foo.cpp'));
  assert.ok(fileError, '应解析出 LogCompile : error: <文件> 这一行');
  assert.equal(fileError.code, 'UBT/Error');
  assert.match(fileError.message, /无法打开包含文件/);
});

test('MSVC 的 fatal error 行单独成为一条诊断', () => {
  const parsed = parseDiagnostics('   fatal error C1083: 无法打开包含文件');
  assert.equal(parsed.length, 1);
  assert.equal(parsed[0].severity, 'error');
  assert.equal(parsed[0].code, 'C1083');
});

test('解析 BuildException 与 dotnet 错误', () => {
  const lines = [
    'LogBuildException: error: 生成 DoodleLiveLinkEditor 时发生异常',
    'C:\\x\\UnrealBuildTool.csproj : error MSB1008: 只能指定一个项目。',
    'LogHttp : Warning: 忽略证书',
  ].join('\n');
  const parsed = parseDiagnostics(lines);
  assert.equal(parsed.length, 3);
  assert.equal(parsed[0].code, 'UBT/Exception');
  assert.equal(parsed[1].code, 'MSB1008');
  assert.equal(parsed[1].severity, 'error');
  assert.equal(parsed[2].code, 'UBT/Warning');
  assert.equal(parsed[2].severity, 'warning');
});

test('去重：同一诊断重复出现只保留一条', () => {
  const line = 'D:\\p\\Foo.cpp(1,1): error C2065: x: 未声明的标识符';
  const parsed = dedupeDiagnostics(parseDiagnostics(`${line}\n${line}`));
  assert.equal(parsed.length, 1);
});

test('读取 UBT Result 行与总耗时', () => {
  const summary = readUbtSummary([
    'Target: DoodleLiveLinkEditor',
    'Total execution time: 12.34 seconds.',
    'Result: Succeeded',
  ].join('\n'));
  assert.equal(summary.result, 'Succeeded');
  assert.equal(summary.totalTimeSeconds, 12.34);

  const failed = readUbtSummary('Result: Failed (OtherCompilationError)');
  assert.equal(failed.result, 'Failed');
  assert.equal(failed.detail, 'OtherCompilationError');
});

test('UBT Result: Failed 但退出码 0 —— 判失败', () => {
  const verdict = decideOutcome({ exitCode: 0, aborted: false, result: 'Failed', resultDetail: 'OtherCompilationError', errors: [] });
  assert.equal(verdict.ok, false);
  assert.equal(verdict.code, 'ubt-failed');
});

test('编译错误优先于 Result 行', () => {
  const errors = parseDiagnostics('D:\\p\\A.cpp(3,4): error C1000: boom');
  const verdict = decideOutcome({ exitCode: 0, aborted: false, result: 'Failed', errors });
  assert.equal(verdict.ok, false);
  assert.equal(verdict.code, 'compile-failed');
  assert.match(verdict.reason, /1 compile error/);
});

test('链接错误与编译错误区分开', () => {
  const errors = parseDiagnostics('LINK : fatal error LNK1181: 无法打开输入文件');
  const verdict = decideOutcome({ exitCode: 6, aborted: false, errors });
  assert.equal(verdict.code, 'link-failed');
});

test('成功：退出码 0 + Result: Succeeded', () => {
  const verdict = decideOutcome({ exitCode: 0, aborted: false, result: 'Succeeded', errors: [] });
  assert.equal(verdict.ok, true);
  assert.equal(verdict.code, 'ok');
});

test('退出码 0 但没有 Result 行 —— 判失败（不谎报成功）', () => {
  const verdict = decideOutcome({ exitCode: 0, aborted: false, errors: [] });
  assert.equal(verdict.ok, false);
  assert.equal(verdict.code, 'no-result');
});

test('退出码 999 带脚本失败提示', () => {
  const verdict = decideOutcome({ exitCode: 999, aborted: false, errors: [] });
  assert.equal(verdict.ok, false);
  assert.equal(verdict.code, 'exit-code');
  assert.match(verdict.reason, /Build\.bat 脚本自身/);
});

test('被取消一定判失败', () => {
  const verdict = decideOutcome({ exitCode: null, aborted: true, result: 'Succeeded', errors: [] });
  assert.equal(verdict.ok, false);
  assert.equal(verdict.code, 'cancelled');
});

test('Result: Succeeded 与退出码冲突时判失败', () => {
  const verdict = decideOutcome({ exitCode: 5, aborted: false, result: 'Succeeded', errors: [] });
  assert.equal(verdict.ok, false);
  assert.equal(verdict.code, 'inconsistent');
});

test('头尾截断不会切开代理对', () => {
  const text = `${'😀'.repeat(10)}\n${'x'.repeat(100)}`;
  const cut = truncateOutput(text, 8, 8);
  assert.equal(cut.truncated, true);
  for (const part of [cut.head, cut.tail]) {
    assert.equal(part.isWellFormed(), true);
  }
});

// ------------------------------------------------------------------ 参数校验

test('extraArgs 只接受开关形状且拒绝核心选项', () => {
  assert.equal(isForwardableUbtArg('-WaitMutex'), true);
  assert.equal(isForwardableUbtArg('--cook'), true);
  assert.equal(isForwardableUbtArg('-MaxParallelActions=4'), true);
  assert.equal(isForwardableUbtArg('-project=X'), false);
  assert.equal(isForwardableUbtArg('-Target=X'), false);
  assert.equal(isForwardableUbtArg('-Mode=QueryTargets'), false);
  assert.equal(isForwardableUbtArg('Win64'), false);
  assert.equal(isForwardableUbtArg('-Log="a b"'), false);
});

test('projectFile 必须是 repoRoot 内的相对路径', () => {
  assert.equal(isSafeRelativePath('DoodleLiveLink/DoodleLiveLink.uproject'), true);
  assert.equal(isSafeRelativePath('..\\escape.uproject'), false);
  assert.equal(isSafeRelativePath('E:\\abs\\X.uproject'), false);
  assert.equal(isSafeRelativePath('\\\\server\\share\\X.uproject'), false);
});

test('从 .uproject 推 target 名', () => {
  assert.equal(moduleTargetName('E:\\Doodle\\DoodleLiveLink\\DoodleLiveLink.uproject'), 'DoodleLiveLink');
});

test('读 EngineAssociation', () => {
  assert.equal(readEngineAssociation('{"EngineAssociation":"5.7"}'), '5.7');
  assert.equal(readEngineAssociation('not json'), undefined);
});

test('cmd 启动行：/d /s /c + 逐项独立 argv', () => {
  const argv = buildLaunchArgv({
    comspec: 'C:\\WINDOWS\\system32\\cmd.exe',
    buildBat: 'E:\\Unreal Engine\\Engine\\Build\\BatchFiles\\Build.bat',
    args: ['GameEditor', 'Win64', 'Development', '-Project=E:\\My Game\\Game.uproject'],
  });
  assert.deepEqual(argv, [
    'C:\\WINDOWS\\system32\\cmd.exe',
    '/d',
    '/s',
    '/c',
    'E:\\Unreal Engine\\Engine\\Build\\BatchFiles\\Build.bat',
    'GameEditor',
    'Win64',
    'Development',
    '-Project=E:\\My Game\\Game.uproject',
  ]);
});

test('cmd 启动行的 argv 项里不含字面反斜杠转义', () => {
  const argv = buildLaunchArgv({
    comspec: 'C:\\WINDOWS\\system32\\cmd.exe',
    buildBat: 'E:\\Unreal Engine\\Engine\\Build\\BatchFiles\\Build.bat',
    args: ['GameEditor', 'Win64', 'Development', '-Project=E:\\My Game\\Game.uproject'],
  });
  for (const part of argv) {
    assert.equal(part.includes('\\"'), false, `argv 项不应包含 \\"：${part}`);
  }
});

// ------------------------------------------------------------------ 目标解析

const CONTEXT = {
  repoRoot: 'E:\\Doodle',
  defaultProjectFileRel: 'DoodleLiveLink/DoodleLiveLink.uproject',
  engineRoot: 'E:\\UnrealEngine',
  targetProjectFileRel: 'DoodleLiveLink/DoodleLiveLink.uproject',
  pluginBuildScriptRel: '',
  defaultPlatform: 'Win64',
  defaultConfiguration: 'Development',
};

const CONTEXT_WITH_TARGET = {
  ...CONTEXT,
  defaultTarget: 'DoodleLiveLink',
};

const ENGINE_AND_PROJECT = [
  'E:\\UnrealEngine\\Engine\\Build\\BatchFiles\\Build.bat',
  'E:\\Doodle\\DoodleLiveLink\\DoodleLiveLink.uproject',
];

/**
 * @param {string[]} known
 * @returns {(value:string)=>boolean}
 */
function existsFrom(known) {
  const set = new Set(known.map((item) => item.toLowerCase()));
  return (value) => set.has(String(value).toLowerCase());
}

test('默认：编译默认工程的编辑器 target', () => {
  const outcome = resolveTarget({}, CONTEXT, existsFrom(ENGINE_AND_PROJECT));
  assert.equal(outcome.ok, true);
  assert.equal(outcome.resolved.target, 'DoodleLiveLinkEditor');
  assert.deepEqual(outcome.resolved.ubtArgs, [
    'DoodleLiveLinkEditor',
    'Win64',
    'Development',
    '-Project=E:\\Doodle\\DoodleLiveLink\\DoodleLiveLink.uproject',
  ]);
  assert.equal(outcome.resolved.cwd, 'E:\\Doodle\\DoodleLiveLink');
});

test('config.defaultTarget 优先于 `<工程名>Editor`', () => {
  const outcome = resolveTarget({}, CONTEXT_WITH_TARGET, existsFrom(ENGINE_AND_PROJECT));
  assert.equal(outcome.ok, true);
  assert.equal(outcome.resolved.target, 'DoodleLiveLink');
  assert.deepEqual(outcome.resolved.ubtArgs, [
    'DoodleLiveLink',
    'Win64',
    'Development',
    '-Project=E:\\Doodle\\DoodleLiveLink\\DoodleLiveLink.uproject',
  ]);
});

test('显式 target 优先于 config.defaultTarget', () => {
  const outcome = resolveTarget(
    { target: 'DoodleLiveLinkLauncher' },
    CONTEXT_WITH_TARGET,
    existsFrom(ENGINE_AND_PROJECT),
  );
  assert.equal(outcome.ok, true);
  assert.equal(outcome.resolved.target, 'DoodleLiveLinkLauncher');
});

test('config.defaultTarget 也用于 projectFile 与 scope=absolute 分支', () => {
  const engineAndGame = [
    ...ENGINE_AND_PROJECT,
    'E:\\Doodle\\Other\\Other.uproject',
    'D:\\Games\\MyGame\\MyGame.uproject',
  ];
  const byProjectFile = resolveTarget({ projectFile: 'Other/Other.uproject' }, CONTEXT_WITH_TARGET, existsFrom(engineAndGame));
  assert.equal(byProjectFile.resolved.target, 'DoodleLiveLink');

  const byAbsolute = resolveTarget(
    { scope: 'absolute', projectAbsolute: 'D:\\Games\\MyGame\\MyGame.uproject' },
    CONTEXT_WITH_TARGET,
    existsFrom(engineAndGame),
  );
  assert.equal(byAbsolute.resolved.target, 'DoodleLiveLink');
});

test('config.defaultTarget 不影响 engine=true（仍是 UnrealEditor）', () => {
  const outcome = resolveTarget({ engine: true }, CONTEXT_WITH_TARGET, existsFrom(ENGINE_AND_PROJECT));
  assert.equal(outcome.resolved.target, 'UnrealEditor');
});

test('engine=true 改成编译 UnrealEditor', () => {
  const outcome = resolveTarget(
    { engine: true },
    CONTEXT,
    existsFrom([
      'E:\\UnrealEngine\\Engine\\Build\\BatchFiles\\Build.bat',
      'E:\\Doodle\\DoodleLiveLink\\DoodleLiveLink.uproject',
    ]),
  );
  assert.equal(outcome.ok, true);
  assert.equal(outcome.resolved.target, 'UnrealEditor');
  assert.equal(outcome.resolved.engineTarget, true);
});

test('显式 target 与 extraArgs 透传', () => {
  const outcome = resolveTarget(
    { target: 'UnrealPak', extraArgs: ['-WaitMutex'] },
    CONTEXT,
    existsFrom([
      'E:\\UnrealEngine\\Engine\\Build\\BatchFiles\\Build.bat',
      'E:\\Doodle\\DoodleLiveLink\\DoodleLiveLink.uproject',
    ]),
  );
  assert.equal(outcome.ok, true);
  assert.equal(outcome.resolved.target, 'UnrealPak');
  assert.equal(outcome.resolved.ubtArgs.at(-1), '-WaitMutex');
});

test('scope=absolute 支持仓库外工程', () => {
  const outcome = resolveTarget(
    { scope: 'absolute', projectAbsolute: 'D:\\Games\\MyGame\\MyGame.uproject' },
    CONTEXT,
    existsFrom([
      'E:\\UnrealEngine\\Engine\\Build\\BatchFiles\\Build.bat',
      'D:\\Games\\MyGame\\MyGame.uproject',
    ]),
  );
  assert.equal(outcome.ok, true);
  assert.equal(outcome.resolved.target, 'MyGameEditor');
  assert.equal(outcome.resolved.cwd, 'D:\\Games\\MyGame');
});

test('pluginBuildScript 优先于默认工程，但显式 projectFile 优先于脚本', () => {
  const context = { ...CONTEXT, pluginBuildScriptRel: 'script/uePlug/BuildDooodlePlug.bat' };
  const exists = existsFrom([
    'E:\\UnrealEngine\\Engine\\Build\\BatchFiles\\Build.bat',
    'E:\\Doodle\\script\\uePlug\\BuildDooodlePlug.bat',
    'E:\\Doodle\\DoodleLiveLink\\DoodleLiveLink.uproject',
  ]);
  const scripted = resolveTarget({}, context, exists);
  assert.equal(scripted.ok, true);
  assert.equal(scripted.resolved.mode, 'script');
  const explicit = resolveTarget({ projectFile: 'DoodleLiveLink/DoodleLiveLink.uproject' }, context, exists);
  assert.equal(explicit.resolved.mode, 'ubt');
});

test('引擎缺失或 projectFile 不存在时给出可读问题', () => {
  const missingEngine = resolveTarget({}, CONTEXT, existsFrom([]));
  assert.equal(missingEngine.ok, false);
  assert.match(missingEngine.problems[0], /引擎构建脚本不存在/);

  const missingProject = resolveTarget(
    {},
    CONTEXT,
    existsFrom(['E:\\UnrealEngine\\Engine\\Build\\BatchFiles\\Build.bat']),
  );
  assert.equal(missingProject.ok, false);
  assert.match(missingProject.problems[0], /默认工程不存在/);
});

test('非法参数被拒绝', () => {
  const outcome = resolveTarget(
    { target: 'bad name', configuration: 'Dev!', extraArgs: ['-project=x'] },
    CONTEXT,
    existsFrom(['E:\\UnrealEngine\\Engine\\Build\\BatchFiles\\Build.bat']),
  );
  assert.equal(outcome.ok, false);
  assert.equal(outcome.problems.length, 3);
});

console.log(`\n${passed} checks passed`);
