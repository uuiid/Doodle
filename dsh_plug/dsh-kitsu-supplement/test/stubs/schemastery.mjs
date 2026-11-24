// @deepseek-ai/schemastery 的最小 stub：只保留本插件用到的链式 API。

const node = () => ({
  default: (value) => value,
  optional: () => node(),
  description: () => node(),
  value: () => node(),
});

const z = {
  object: (shape) => ({ ...node(), shape }),
  string: () => node(),
  number: () => node(),
  boolean: () => node(),
  union: () => node(),
  array: () => node(),
  any: () => node(),
  const: () => node(),
  dict: () => node(),
};

export default z;
export { z };
