// 验证 apply() 的接线：用桩 ctx 捕获注册的监听器，直接驱动它，
// 确认返回的 LlmCallConfig 带上 temperature，且不破坏其它字段。
// 不启动 harness。
//
// 运行：node test/apply.mjs

import assert from 'node:assert/strict';

import { loadPlugin } from './load.mjs';

const { apply } = await loadPlugin();

let passed = 0;
const check = (label, fn) => {
  fn();
  passed += 1;
  console.log(`  ok  ${label}`);
};

/** 构造一个最小 ctx 桩：记录 effect 注册，捕获 agent/request 监听器。 */
function makeCtx() {
  const listeners = new Map();
  const logs = [];
  return {
    listeners,
    logs,
    ctx: {
      effect: (fn) => fn(),
      on: (event, handler) => {
        listeners.set(event, handler);
        return () => listeners.delete(event);
      },
      logger: { info: (m) => logs.push(m) },
    },
  };
}

/** 驱动捕获到的 agent/request 监听器。 */
async function drive(harness, config, callConfig) {
  apply(harness.ctx, config);
  const handler = harness.listeners.get('agent/request');
  assert.ok(handler, 'agent/request listener must be registered');
  return handler({}, async () => callConfig);
}

console.log('apply()');

await (async () => {
  const h = makeCtx();
  const out = await drive(h, { routes: {}, models: {}, log: false }, { provider: 'volcengine', model: 'm' });
  check('registers an agent/request listener', () => assert.ok(h.listeners.has('agent/request')));
  check('no configuration leaves temperature absent', () => {
    assert.equal(out.temperature, undefined);
    assert.deepEqual(out, { provider: 'volcengine', model: 'm' });
  });
})();

await (async () => {
  const h = makeCtx();
  const out = await drive(
    h,
    { routes: { volcengine: { temperature: 0.6 } }, models: {}, log: false },
    { provider: 'volcengine', model: 'm' },
  );
  check('route-level temperature is injected', () => assert.equal(out.temperature, 0.6));
  check('provider and model survive untouched', () => {
    assert.equal(out.provider, 'volcengine');
    assert.equal(out.model, 'm');
  });
})();

await (async () => {
  const h = makeCtx();
  const out = await drive(
    h,
    { routes: { volcengine: { temperature: 0.6 } }, models: { 'volcengine/m': { temperature: 0.2 } }, log: false },
    { provider: 'volcengine', model: 'm' },
  );
  check('model-level override beats the route default', () => assert.equal(out.temperature, 0.2));
})();

await (async () => {
  const h = makeCtx();
  const out = await drive(
    h,
    { routes: { volcengine: { temperature: 0.6 } }, models: {}, log: false },
    { provider: 'other', model: 'm' },
  );
  check('another route is not affected', () => assert.equal(out.temperature, undefined));
})();

await (async () => {
  const h = makeCtx();
  const out = await drive(
    h,
    { routes: { volcengine: { temperature: 0.6 } }, models: {}, log: false },
    { provider: 'volcengine', model: 'm', maxTokens: 100, reasoningEffort: 'high' },
  );
  check('existing call controls are preserved', () => {
    assert.equal(out.maxTokens, 100);
    assert.equal(out.reasoningEffort, 'high');
    assert.equal(out.temperature, 0.6);
  });
})();

await (async () => {
  const h = makeCtx();
  const input = { provider: 'volcengine', model: 'm' };
  const out = await drive(h, { routes: { volcengine: { temperature: 0.6 } }, models: {}, log: false }, input);
  check('the upstream config object is not mutated', () => {
    assert.equal(input.temperature, undefined);
    assert.notEqual(out, input);
  });
})();

await (async () => {
  const h = makeCtx();
  await drive(
    h,
    { routes: { volcengine: { temperature: 0.6 } }, models: {}, log: true },
    { provider: 'volcengine', model: 'm' },
  );
  check('log=true writes one line naming route and value', () => {
    assert.equal(h.logs.length, 1);
    assert.match(h.logs[0], /volcengine\/m/);
    assert.match(h.logs[0], /0\.6/);
  });
})();

await (async () => {
  const h = makeCtx();
  await drive(
    h,
    { routes: { volcengine: { temperature: 0.6 } }, models: {}, log: false },
    { provider: 'volcengine', model: 'm' },
  );
  check('log=false writes nothing', () => assert.equal(h.logs.length, 0));
})();

await (async () => {
  const h = makeCtx();
  const out = await drive(
    h,
    { routes: { volcengine: { temperature: null } }, models: {}, log: false },
    { provider: 'volcengine', model: 'm' },
  );
  check('a null temperature is treated as unset, not written', () => assert.equal(out.temperature, undefined));
})();

await (async () => {
  const h = makeCtx();
  const out = await drive(
    h,
    { routes: { volcengine: { temperature: 0 } }, models: {}, log: false },
    { provider: 'volcengine', model: 'm' },
  );
  check('temperature 0 is written (not mistaken for unset)', () => assert.equal(out.temperature, 0));
})();

console.log(`\n${passed} assertions passed`);
