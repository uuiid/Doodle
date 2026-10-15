# dsh-sampling-params

给 DeepSeek Harness 的每次 LLM 调用注入可配置的采样参数（`temperature` 等），
通过官方的 `agent/request` 瀑布钩子实现，**不修改任何官方包**。

## 为什么需要它

harness 的调用配置类型 `LlmCallControls`（`@deepseek-ai/dsh-llm/call-config`）
里 `temperature` 是一等字段，但**没有任何生产路径给它赋值**：

| 层 | 对 `temperature` 的行为 |
| --- | --- |
| `LlmCallControls.temperature` | 可选字段，无人赋值 |
| `resolveCallWithInfo`（`dsh-llm`） | 只补 `maxTokens` 与 `reasoningEffort` 的默认值，**不碰 temperature** |
| `adapterDefaults` | 只跟踪 `reasoningEffort` / `maxTokens`，无 temperature |
| `llm-pi-ai` 适配器 | `undefined` 就省略该字段 |
| pi-ai（`simple-options.js`） | 原样透传 |
| pi-ai responses（`openai-responses.js`） | `undefined` 则**不加进请求体** |
| 服务端 | 用它自己的默认值（火山方舟 Responses API 为 `1.0`） |

结论：**默认情况下请求体里根本没有 `temperature`**，你无法通过配置文件设置它。

`agent/request` 是官方为这类需求预留的扩展点，它返回的 `LlmCallConfig`
就是本次请求的调用配置。本插件只做这一件事。

## 安装

与 `dsh-cmake` / `dsh-tool-everything` 同惯例，作为 profile 的 `file:` 依赖：

1. 在 `C:\Users\TD\.dsh\profiles\web\package.json` 的 `dependencies` 加：

   ```json
   "dsh-sampling-params": "file:E:/Doodle/dsh-sampling-params"
   ```

2. 在同一个文件的 `dsh.profile.bundles` 数组里加 `"dsh-sampling-params"`。

3. 在 profile 目录执行 `pnpm install`。

4. 重启 dsh。

## 配置

配置写在 `C:\Users\TD\.dsh\profiles\web\cordis.patch.yml` 里本插件的 `config` 下：

```yaml
- insert:
    - id: sampling-params
      name: dsh-sampling-params
      config:
        routes:
          volcengine:
            temperature: 0.6
        models:
          volcengine/deepseek-v4-1-flash-260910:
            temperature: 0.3
        log: false
```

### 字段

| 字段 | 说明 |
| --- | --- |
| `routes` | 路由级默认值，键为 provider 名。`"*"` 是所有路由的兜底 |
| `models` | 模型级覆盖，键为 `provider/model`；支持 `*/model` 与 `provider/*` 通配 |
| `log` | 是否把每次请求生效的采样参数写进日志（默认 `false`） |

### 合并顺序

后者覆盖前者，**最具体的键永远赢**：

```
routes["*"] → routes[provider] → models["*/*"] → models["provider/*"]
  → models["*/model"] → models["provider/model"]
```

逐键合并：模型级只覆盖它写了的键，路由级的其它键保留。

### 值域

值必须是 JSON 标量（字符串 / 数字 / 布尔 / null）。嵌套对象与数组会被
schema 拒绝——这保证配置能安全往返文档与请求体。

`temperature: 0` 会被正常写出（不会被误判为"未设置"）；`null` 视为未设置。

## 已知限制

**只有 `temperature` 会真正生效。** 其余键（`top_p`、`top_k`、penalty 等）
不在 harness 的调用配置词汇里（见 `dsh-llm` README：「`GenerateOptions`
sampling is `temperature`/`maxTokens`/`stop` only」），本插件不臆造字段，
因此它们会被合并但不会被使用。

要发这些字段需要适配器侧的通道——`@earendil-works/pi-ai` 本身支持
`options.samplingParams`（它会最后 `Object.assign` 进请求体），但
`dsh-llm-pi-ai` 的 profile schema 没有暴露它。若确实需要，只能改官方包。

## 与 `compat.supportsTemperature` 的区别

`compat.supportsTemperature` 是**死路**：`dsh-llm-pi-ai` 的 `catalog.ts`
与 `docs/config-catalog.md` 都注明该开关属于 `anthropic-messages` 协议，
`openai-responses` 不声明它，在 profile 上设置会让模型在解析阶段直接失败。

## 测试

```powershell
cd E:\Doodle\dsh-sampling-params
node --check lib/index.js
node test/pure.mjs     # 合并语义
node test/apply.mjs    # apply() 接线
node test/config.mjs   # Config schema（需从 profile 目录解析 schemastery）
```

`test/config.mjs` 从 `C:/Users/TD/.dsh/profiles/web/package.json` 解析
`@deepseek-ai/schemastery`，因为本地插件自身没有 `node_modules`。

## 注意

- 若模型在开启 thinking 时忽略温度（DeepSeek 官方 API 有此行为，
  见 `dsh-llm-deepseek` README），设置本插件不会有可见效果，
  此时应改用 `reasoningEfforts` 档位调节。
- 火山方舟文档不建议同时调整 `temperature` 与 `top_p`。
