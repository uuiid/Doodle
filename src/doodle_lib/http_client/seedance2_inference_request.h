//
// Created by TD on 2024/2/29.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace doodle::http::seedance2 {

// 这里曾经有一个 `inference_request_t`（归一化推理信封：input_text / instructions /
// images / model / temperature / json_schema… 十三个扁平键）以及它的 to_json / from_json。
// **整个结构体已删除**，因为它的往返是**一份手写白名单**：只认识的键会被留下、其余全被
// 静默吃掉，而现在的发射路径要求「前端请求原样透传」（见下面 finalize_inference_body 的说明）。
//
// 删除后本文件只剩一件事：**在原始 json 上原地定稿**。落库的 `data_request_` 与发出去的
// body 是同一份 json，中间不再经过任何结构体 —— 少一次往返就少一次静默丢键的机会。
// 键名与语义（方舟 Responses API）见 finalize_inference_body 的注释。

/**
 * 定稿一份**要发出去**的 body（原地改）: 覆盖服务端的强制字段。
 *
 * 这是 `/responses` 请求体与「本系统自己的请求」之间的边界: 过了这里的内容就只有方舟的
 * schema 在管, 服务端不再往里加任何字段 —— 所以它**只做覆盖**, 不替前端补默认值。
 *
 * 参数是 `nlohmann::json`（而不是曾经那个 `inference_request_t`）: 发出去的东西就是任务行里
 * 那份 json 本身, 中间不再经过一次结构体往返 —— 那次往返是一份手写白名单, 不认识的前端
 * 字段会被静默吃掉, 与「原样透传」直接冲突。
 *
 * `in_default_model` 由调用方从请求体**之外**取（服务端策略）, 于是前端传什么 model 都无效;
 * 空 model（会话没配, 或前端根本没传）也走这条回落, 不会带着 model:"" 打过去。
 */
inline void finalize_inference_body(nlohmann::json& io_body, const std::string& in_default_model) {
  // 传了空串（会话没配模型）时也要回落到缺省值, 否则会带着 model:"" 打过去白跑一趟。
  const auto l_model = io_body.contains("model") && io_body.at("model").is_string()
                           ? io_body.at("model").get<std::string>()
                           : std::string{};
  io_body["model"]   = l_model.empty() ? in_default_model : l_model;
  // 不把提示词与素材留在方舟侧 —— 两条路径同一条隐私口径, 由这里统一保证。
  // store 是**唯一**一个服务端强行写入的键; 除此之外这里不补任何默认值（包括 thinking:
  // 以前这里发 {"type":"disabled"}, 那是服务端替前端做决定, 与「原样透传」冲突）。
  io_body["store"] = false;
  // max_output_tokens 同理**不补**：结构化输出要求全字段 required, 输出长度不可预估, 而截断
  // 之后 result_json_ 只剩半个 JSON —— 那个失败面跟「schema 不合规被拒」完全分不出来。
  // 交给模型/账号侧决定上限; 只有调用方/前端显式给了才发（它在体里, 原样留着, 这里不动）。
}

}  // namespace doodle::http::seedance2
