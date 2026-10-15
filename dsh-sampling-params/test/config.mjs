// 验证 Config schema：确认 schemastery 能解析本插件的配置形状，
// 且默认值、类型校验都符合预期。不启动 harness。
//
// 运行：node test/config.mjs

import assert from 'node:assert/strict';

import { loadPlugin } from './load.mjs';

const { Config } = await loadPlugin();

let passed = 0;
const check = (label, fn) => {
  fn();
  passed += 1;
  console.log(`  ok  ${label}`);
};

console.log('Config schema');

check('empty config resolves to empty dicts and log=false', () => {
  const c = Config({});
  assert.deepEqual(c.routes, {});
  assert.deepEqual(c.models, {});
  assert.equal(c.log, false);
});

check('route-level temperature parses', () => {
  const c = Config({ routes: { volcengine: { temperature: 0.6 } } });
  assert.deepEqual(c.routes.volcengine, { temperature: 0.6 });
});

check('model-level override parses', () => {
  const c = Config({ models: { 'volcengine/deepseek-v4-1-flash-260910': { temperature: 0.3 } } });
  assert.deepEqual(c.models['volcengine/deepseek-v4-1-flash-260910'], { temperature: 0.3 });
});

check('wildcard keys parse', () => {
  const c = Config({ routes: { '*': { temperature: 0.9 } }, models: { '*/m': { temperature: 0.4 } } });
  assert.deepEqual(c.routes['*'], { temperature: 0.9 });
  assert.deepEqual(c.models['*/m'], { temperature: 0.4 });
});

check('non-temperature scalars parse', () => {
  const c = Config({ routes: { v: { top_p: 0.9, top_k: 40, seed: 7, stop: null } } });
  assert.deepEqual(c.routes.v, { top_p: 0.9, top_k: 40, seed: 7, stop: null });
});

check('log accepts true', () => {
  assert.equal(Config({ log: true }).log, true);
});

check('a nested object value is refused', () => {
  assert.throws(() => Config({ routes: { v: { temperature: { nested: 1 } } } }));
});

check('an array value is refused', () => {
  assert.throws(() => Config({ routes: { v: { temperature: [1, 2] } } }));
});

check('a non-numeric temperature string is still a scalar (schema is type-agnostic)', () => {
  const c = Config({ routes: { v: { temperature: 'warm' } } });
  assert.equal(c.routes.v.temperature, 'warm');
});

console.log(`\n${passed} assertions passed`);
