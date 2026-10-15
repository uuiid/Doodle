// dsh-sampling-params 纯逻辑测试：只覆盖 resolveSamplingParams 的合并语义，
// 不加载 cordis / 不启动 harness。
//
// 运行：node test/pure.mjs

import assert from 'node:assert/strict';

import { loadPlugin } from './load.mjs';

const { resolveSamplingParams } = await loadPlugin();

let passed = 0;
const check = (label, actual, expected) => {
  assert.deepEqual(actual, expected, label);
  passed += 1;
  console.log(`  ok  ${label}`);
};

console.log('resolveSamplingParams');

// 未配置任何层 → undefined（调用方据此原样返回，不制造空对象）
check(
  'no configuration yields undefined',
  resolveSamplingParams({ routes: {}, models: {} }, 'volcengine', 'm'),
  undefined,
);

// 仅路由级
check(
  'route-level value applies to every model on that route',
  resolveSamplingParams({ routes: { volcengine: { temperature: 0.6 } }, models: {} }, 'volcengine', 'm'),
  { temperature: 0.6 },
);

// 路由级不跨路由泄漏
check(
  'route-level value does not leak to another route',
  resolveSamplingParams({ routes: { volcengine: { temperature: 0.6 } }, models: {} }, 'deepseek', 'm'),
  undefined,
);

// "*" 兜底
check(
  'wildcard route is the fallback for every route',
  resolveSamplingParams({ routes: { '*': { temperature: 0.9 } }, models: {} }, 'anything', 'm'),
  { temperature: 0.9 },
);

// 具体路由覆盖 "*"
check(
  'concrete route overrides the wildcard route',
  resolveSamplingParams(
    { routes: { '*': { temperature: 0.9 }, volcengine: { temperature: 0.6 } }, models: {} },
    'volcengine',
    'm',
  ),
  { temperature: 0.6 },
);

// 模型级覆盖路由级，且逐键合并（未覆盖的键保留路由级）
check(
  'model entry overrides route per key and keeps other route keys',
  resolveSamplingParams(
    { routes: { volcengine: { temperature: 0.6, top_p: 0.9 } }, models: { 'volcengine/m': { temperature: 0.2 } } },
    'volcengine',
    'm',
  ),
  { temperature: 0.2, top_p: 0.9 },
);

// 模型级键不匹配时不生效
check(
  'model entry for another model does not apply',
  resolveSamplingParams({ routes: {}, models: { 'volcengine/other': { temperature: 0.2 } } }, 'volcengine', 'm'),
  undefined,
);

// "*/model" 匹配任意路由上的同名模型
check(
  'wildcard-provider model entry matches any route',
  resolveSamplingParams({ routes: {}, models: { '*/m': { temperature: 0.4 } } }, 'volcengine', 'm'),
  { temperature: 0.4 },
);

// "provider/*" 匹配该路由上的任意模型
check(
  'wildcard-model entry matches any model on the route',
  resolveSamplingParams({ routes: {}, models: { 'volcengine/*': { temperature: 0.4 } } }, 'volcengine', 'm'),
  { temperature: 0.4 },
);

// 最具体的键赢：provider/model > */model > provider/* > */*
check(
  'the most specific key wins',
  resolveSamplingParams(
    {
      routes: { volcengine: { temperature: 0.1 } },
      models: {
        '*/*': { temperature: 0.2 },
        'volcengine/*': { temperature: 0.3 },
        '*/m': { temperature: 0.4 },
        'volcengine/m': { temperature: 0.5 },
      },
    },
    'volcengine',
    'm',
  ),
  { temperature: 0.5 },
);

// 非 temperature 的键也照常合并（由调用方决定是否使用）
check(
  'non-temperature keys survive the merge',
  resolveSamplingParams({ routes: { volcengine: { top_p: 0.8 } }, models: {} }, 'volcengine', 'm'),
  { top_p: 0.8 },
);

// 空对象层不产生 undefined 之外的意外
check(
  'an empty layer object still yields an object',
  resolveSamplingParams({ routes: { volcengine: {} }, models: {} }, 'volcengine', 'm'),
  {},
);

console.log(`\n${passed} assertions passed`);
