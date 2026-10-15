// 验证 Config schema：确认 schemastery 能解析本插件的配置形状，
// 且默认值、类型校验都符合预期。不启动 harness。
//
// 依赖解析说明：本地插件自身没有 node_modules，`@deepseek-ai/schemastery`
// 由 profile 的 pnpm workspace 提供。因此本测试必须从 profile 目录运行：
//   cd C:\Users\TD\.dsh\profiles\web
//   node E:\Doodle\dsh-sampling-params\test\config.mjs
//
// 运行：node test/config.mjs（在 profile 目录下）

import assert from 'node:assert/strict';
import { createRequire } from 'node:module';

// 从 profile 的 node_modules 解析 schemastery，而不是插件自己的目录。
const require = createRequire('C:/Users/TD/.dsh/profiles/web/package.json');
const schemasteryPath = require.resolve('@deepseek-ai/schemastery');
const z = (await import(`file:///${schemasteryPath.replace(/\\/g, '/')}`)).default;

const { Config } = await import('../lib/index.js');

let passed = 0;
const check = (label, fn) => {
  fn();
  passed += 1;
  console.log(`  ok  ${label}`);
};

console.log('Config schema');

check('empty config resolves to empty dicts and log=false', () => {
  const c = Config(z)({});
  assert.deepEqual(c.routes, {});
  assert.deepEqual(c.models, {});
  assert.equal(c.log, false);
});

check('route-level temperature parses', () => {
  const c = Config(z)({ routes: { volcengine: { temperature: 0.6 } } });
  assert.deepEqual(c.routes.volcengine, { temperature: 0.6 });
});

check('model-level override parses', () => {
  const c = Config(z)({ models: { 'volcengine/deepseek-v4-1-flash-260910': { temperature: 0.3 } } });
  assert.deepEqual(c.models['volcengine/deepseek-v4-1-flash-260910'], { temperature: 0.3 });
});

check('wildcard keys parse', () => {
  const c = Config(z)({ routes: { '*': { temperature: 0.9 } }, models: { '*/m': { temperature: 0.4 } } });
  assert.deepEqual(c.routes['*'], { temperature: 0.9 });
  assert.deepEqual(c.models['*/m'], { temperature: 0.4 });
});

check('non-temperature scalars parse', () => {
  const c = Config(z)({ routes: { v: { top_p: 0.9, top_k: 40, seed: 7, stop: null } } });
  assert.deepEqual(c.routes.v, { top_p: 0.9, top_k: 40, seed: 7, stop: null });
});

check('log accepts true', () => {
  assert.equal(Config(z)({ log: true }).log, true);
});

check('a nested object value is refused', () => {
  assert.throws(() => Config(z)({ routes: { v: { temperature: { nested: 1 } } } }));
});

check('an array value is refused', () => {
  assert.throws(() => Config(z)({ routes: { v: { temperature: [1, 2] } } }));
});

check('a non-numeric temperature string is still a scalar (schema is type-agnostic)', () => {
  const c = Config(z)({ routes: { v: { temperature: 'warm' } } });
  assert.equal(c.routes.v.temperature, 'warm');
});

console.log(`\n${passed} assertions passed`);
