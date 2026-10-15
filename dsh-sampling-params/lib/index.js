// dsh-sampling-params —— 让 DSH 的每次 LLM 调用带上可配置的采样参数。
//
// 为什么需要它：harness 的 `LlmCallControls`（dsh-llm/call-config）里
// `temperature` 是一等字段，但**没有任何生产路径给它赋值** ——
// `resolveCallWithInfo` 只补 `maxTokens` 与 `reasoningEffort` 的默认值，
// 而 `installModelSelection` 只覆盖 provider/model/reasoningEffort。
// 于是 `temperature` 一路是 undefined，适配器省略该字段，请求体里根本没有它，
// 服务端只能用它自己的默认值（火山方舟 Responses API 为 1.0）。
//
// 官方给这类需求留的扩展点是 `agent/request` 瀑布钩子：它返回的
// `LlmCallConfig` 就是本次请求的调用配置，`temperature` 是其中的合法字段。
// 本插件只做这一件事，因此不碰任何官方包，dsh 升级不会失效。
//
// 硬约束（与 dsh-cmake / dsh-tool-everything 同惯例）：
//   * 顶层不 import node: 内置模块。
//   * 所有注册走 ctx.effect。

export const name = 'sampling-params';

/**
 * 本插件只依赖 agent 作用域上的 `agent/request` 事件，不依赖任何服务。
 * 空数组表示"不需要注入任何服务"——声明 `inject: []` 而非省略，是为了让
 * loader 明确知道本插件不会因为服务缺失而挂起。
 */
export const inject = [];

/**
 * 采样参数值：JSON 标量。与 dsh-llm-pi-ai 的 samplingParams 域一致，
 * 只接受能安全往返文档与请求体的类型。
 */
const scalar = (z) => z.union([z.string(), z.number(), z.boolean(), z.const(null)]);

export const Config = (z) => z.object({
  /**
   * 路由级默认值，键为 provider 名。`"*"` 是所有路由的兜底。
   * 例：`{ volcengine: { temperature: 0.6 } }`
   */
  routes: z.dict(z.dict(scalar(z))).default({}),
  /**
   * 模型级覆盖，键为 `provider/model`，也支持通配的 `*` 形式（星号在前或在后）。
   * 例：`{ "volcengine/deepseek-v4-1-flash-260910": { temperature: 0.3 } }`
   */
  models: z.dict(z.dict(scalar(z))).default({}),
  /**
   * 是否把生效的采样参数写进日志（每次请求一行）。默认关闭：
   * 正常使用下它只是噪音，排查"到底发出去没有"时才打开。
   */
  log: z.boolean().default(false),
});

/**
 * 从配置里挑出适用于本次请求的采样参数。
 *
 * 合并顺序（后者覆盖前者）：`routes["*"]` → `routes[provider]` →
 * `models["*&#47;*"]` → `models["provider/*"]` → `models["*&#47;model"]` →
 * `models["provider/model"]`。这样最具体的键永远赢，且不需要调用方
 * 关心匹配优先级。
 *
 * @param {object} config 已解析的插件配置。
 * @param {string} provider 本次请求的路由名。
 * @param {string} model 本次请求的模型 id。
 * @returns {object|undefined} 合并后的参数；两层都没配时为 undefined。
 */
export function resolveSamplingParams(config, provider, model) {
  const routes = config.routes ?? {};
  const models = config.models ?? {};
  const layers = [
    routes['*'],
    routes[provider],
    models['*/*'],
    models[`${provider}/*`],
    models[`*/${model}`],
    models[`${provider}/${model}`],
  ];
  let merged;
  for (const layer of layers) {
    if (layer === undefined || layer === null) continue;
    merged = { ...merged, ...layer };
  }
  return merged;
}

/**
 * 插件入口。
 *
 * @param {any} ctx
 * @param {any} config
 */
export function apply(ctx, config) {
  const resolved = {
    routes: config.routes ?? {},
    models: config.models ?? {},
    log: config.log === true,
  };

  ctx.effect(() => ctx.on('agent/request', async (_payload, next) => {
    const callConfig = await next();
    const params = resolveSamplingParams(resolved, callConfig.provider, callConfig.model);
    if (params === undefined) return callConfig;

    // `temperature` 是 LlmCallConfig 的合法字段，直接写在顶层。
    // 其余键（top_p、top_k、penalty 等）不在 harness 的调用配置词汇里，
    // 本插件不臆造字段：它们需要适配器侧的 samplingParams 通道，
    // 见 README 的「已知限制」。
    const nextConfig = { ...callConfig };
    if (params.temperature !== undefined && params.temperature !== null) {
      nextConfig.temperature = params.temperature;
    }
    if (resolved.log) {
      ctx.logger?.info?.(
        `sampling-params: ${callConfig.provider}/${callConfig.model} temperature=${nextConfig.temperature ?? '(unset)'}`,
      );
    }
    return nextConfig;
  }));
}
