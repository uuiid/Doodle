//
// AI 灯光：两条接缝共用的**内部件**（私有头，不进公共 API）
//
// 这里放的都是实现细节：跨接缝共用的常量、字符串拼装、rig/预设的紧凑摘要。故意不做成
// 公共 API —— 一旦导出就会有人从外面依赖，而它们的形状会随提示词一起变。
//
// 本头**不含任何 HTTP 依赖**
// （不 include http_jwt_fun / session_data / 推理客户端），所以单测可以直接调用其中的纯函数：
// 单测宿主没有 core_set::ctx_ptr、没有 SSL context、没有 HTTP logger。
//
// 分工：本文件只有「纯函数 + 纯文本」。工具（`ai_light_rig_ops.h`）在另一侧，接缝的
// 请求/映射在自己的 `ai_light_seam_*.h` 里；三者都靠本头共享措辞与口径。
//

#pragma once

#include <doodle_core/metadata/ai_light/preset.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/ai_light/spec.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_inference_request.h>

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::ai_light {

// ---------------------------------------------------------------------------
// 字符串与颜色格式化
// ---------------------------------------------------------------------------

/// 用 in_sep 连接字符串（空 vector → 空串）。
std::string join_strings(const std::vector<std::string>& in_values, std::string_view in_sep);

/// 数字列表的紧凑写法（", " 分隔）。只用于提示词，不参与任何判定。
std::string join_numbers(const std::vector<double>& in_values);

/// 色值 → "#rrggbb"（先 clamp 到 [0,1] 再量化 8 bit）。提示词里出现颜色统一走它 ——
/// 直接打印三个浮点会让模型去猜哪个分量是 R。
std::string rgb_to_hex(const vec3_t& in_color);

// ---------------------------------------------------------------------------
// 提示词共用的文本段
// ---------------------------------------------------------------------------

/// 抽取给 spec 与 candidates 共用的系统指令段：职责边界（§6.1）、性格 → 灯光语言映射表
/// （§5.5）、单位口径。**只允许有一处定义** —— 两份提示词对「对比度是什么」说法不一致
/// 会让模型在两条接缝上给出互不相容的数值。
std::string build_common_lighting_section();

/// 轴值语义（§15.8 的表）。接缝 2 与接缝 3 共用同一份口径：模型按它给档位，服务端按同一套
/// 公式应用 —— 两边不一致时美术会看到「轴变了但图没变」。
///
/// **只放语义，不放约束**（§15.9 决定 D12）：接缝 2 的「恰好 5 个档位 / 档位不要取 0」是排轴
/// 环节的硬约束，接缝 3 只给**一个**方向值，两者约束不同。混进这一段会让接缝 3 的提示词
/// 连带继承一套不适用的档位规则。
inline constexpr std::string_view g_axis_value_semantics_text = R"AXIS(每个轴的 value 都是**增量**（不是绝对值），中性值是 0；只有 key_fill_ratio 例外，它本身就是目标比值。
- key_direction     方位角增量（度）。正负决定主光往哪边偏；常见档位 -60 / -30 / +30 / +60。
- key_fill_ratio    目标 key:fill 比值（绝对，如 3 / 6 / 10）。服务端按「当前比值 ÷ 目标比值」缩放补光强度。
- kelvin            色温增量（K）。同时作用于灯光色温与全局白平衡；常见档位 -800 / -400 / +400 / +800。
- hardness          硬软增量（-1..+1，正 = 更硬）。同时收紧影边缘与缩小光源半径；常见档位 -0.5 / +0.5。
- ambient_level     环境光相对增减（-1..+1）。作用于天光强度与 role=ambient 的灯；常见档位 -0.4 / +0.4。
- grading_strength  调色强度增量（-1..+1），只放大/缩小「偏离中性的量」（对比、饱和、提黑、暗角、光晕）；
                    常见档位 -0.35 / +0.35。
)AXIS";

// ---------------------------------------------------------------------------
// 摘要（写进提示词，给模型与美术读）
// ---------------------------------------------------------------------------

/// 目标区间的紧凑投影（对比度 / 白平衡 / 亮度分位 / 削波 / 调色倾向）。
std::string describe_targets(const lighting_targets_t& in_targets);

/// 一版 rig 的紧凑投影（globals 概览 + 每盏灯的关键参数）。
std::string describe_rig_brief(const lighting_rig_t& in_rig);

/// 一版 Spec 的紧凑投影（intent + targets + palette），接缝 2 与接缝 3 都用。
std::string describe_spec_brief(const lighting_spec_t& in_spec);

/// 一条预设的紧凑投影，接缝 2 列候选预设时用。
std::string describe_preset_brief(const ai_light_preset& in_preset);

// ---------------------------------------------------------------------------
// 推理信封与推理管道（曾经由「两条接缝」共用；三条接缝现已全部删除，
// 唯一调用方是**推理网关** ai_light.cpp）
// ---------------------------------------------------------------------------

/// 请求体共有的身份字段。
///
/// 提到公共基类是为了让 `run_seam_inference` 只收一个信封：管道要的三样东西
/// （计费主体、任务归属项目、会话）语义完全相同，其余字段各自不同。
///
/// 不给 `DOODLELIB_API`：这是库内部件，派生请求体自己带着 DLL 边界（基类只贡献
/// 对象布局里的成员，消费侧读写成员不需要跨 DLL 的符号）。派生类作为 dllexport 类的
/// 非导出基类，MSVC 只会给 C4275，而本工程已 `/wd4275`。
struct seam_request_base_t {
  uuid session_id_{};
  /// 写入 sd2::task.project_uuid_id_（可空 FK）。只为归属可查，内部不依赖它。
  uuid project_id_{};
  /// 触发人（person uuid）= 计费主体 + 签名 URL 里的 uid。
  ///
  /// 必须显式传入、**不给默认值**：`run_inference_task` 会按 `user_id_` 扣
  /// `person_token`，一旦有默认值，将来新增调用点忘记传就会把费用静默记到别人头上。
  /// 为 nil 时由网关回落到 `ai_light_session.submitter_`；两者都 nil 抛
  /// bad_request（判定在 `make_seam_client` 里）。
  uuid operator_id_{};
};

/// 一次推理的产出：**只有**模型输出的 JSON 与三项 token 计数。
///
/// 刻意不把 `run_task_result_t` 原样传出去：status / error_code / is_timeout 这些「为什么失败」
/// 的信息已经被 `chick_inference_ok` 消费成了 `doodle_error`（400 或 502），调用方拿到它们
/// 只会写出第二套互不相容的判定。字段名与 `run_task_result_t` 的同名成员保持一致。
struct seam_inference_result_t {
  nlohmann::json json_{};
  std::int64_t input_tokens_{0};
  std::int64_t output_tokens_{0};
  std::int64_t total_tokens_{0};
};

/// 推理管道：解析触发人 → 建客户端 → 组装任务行 → 跑推理 → 判定成败。
///
/// `in_data_request` 是已建造好的请求体 json（前端来的 body 原样）；`in_iteration_id`
/// 为 nil 表示这次推理不属于任何轮次。
///
/// 参数是 json 而不是 `inference_request_t`：落库的与发出去的必须是**同一份**东西，中间再
/// 夹一次结构体往返就等于给前端透传的键加了一道静默白名单（不认识的键无声消失）。
///
/// 不再有 `in_text_prompt`：它曾是任务行 `text_prompt_` 的来源（会话列表上显示的那句话），
/// 而那句话是**从请求体里再抄一份**——同一份提示词存两处，两处迟早不一致，且抄的那份还是
/// 截断过的。任务行要显示什么由 `collect_request_info()` 在落库时从请求体自己读。
///
/// 不再有 `in_seam` 参数：接缝名曾是记账与措辞的键，现在「这条推理跑的是哪条接缝」既不进
/// 任务行、也不影响任何判定 —— 它只存在于调用方的函数名里，让管道去收一个只有措辞用途的
/// 参数是把两者假耦合起来。
///
/// 副作用**只有** `run_inference_task` 自己的那个事务（任务行 + 扣费）。抛 `doodle_error`
/// 表示这次推理不成立（400 内容违规/参数不全，502 推理失败或输出不是合法 JSON 对象）。
/// 注意：服务端已不再有「回滚会话状态」的调用方 —— 网关只把错误原样报给前端。
boost::asio::awaitable<seam_inference_result_t> run_seam_inference(
    sqlite_database& in_sql, const seam_request_base_t& in_request, const nlohmann::json& in_data_request,
    const uuid& in_iteration_id = {}
);

}  // namespace doodle::ai_light
