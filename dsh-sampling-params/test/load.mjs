// 供三个测试加载插件入口。
//
// 为什么不能直接 `import '../lib/index.js'`：入口顶层 import 了
// `@deepseek-ai/schemastery`，而插件目录自身没有 node_modules——ESM 的裸模块
// 说明符只从「导入方所在目录」逐级向上解析（与 cwd 无关），所以从
// E:\Doodle\dsh-sampling-params 下解析不到它。
//
// 做法：从 profile 的 package.json 解析出 schemastery 的绝对路径，把入口源码里的
// 那个说明符改写成对应的 file: URL，再经 data: URL 导入。data: 模块与调用方同属
// 一个 realm，返回值与断言里的字面量原型一致（`vm` 方案会产生跨 realm 对象，
// 让 deepStrictEqual 误报），同时保留完整的 ESM 语义。

import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { pathToFileURL } from 'node:url';

const PROFILE_PACKAGE = 'C:/Users/TD/.dsh/profiles/web/package.json';
const SCHEMASTERY = '@deepseek-ai/schemastery';

export async function loadPlugin() {
  const require = createRequire(PROFILE_PACKAGE);
  const url = pathToFileURL(require.resolve(SCHEMASTERY)).href;

  const source = readFileSync(new URL('../lib/index.js', import.meta.url), 'utf8').replace(
    new RegExp(`from '${SCHEMASTERY}'`, 'g'),
    `from '${url}'`,
  );

  return import(`data:text/javascript;base64,${Buffer.from(source, 'utf8').toString('base64')}`);
}
