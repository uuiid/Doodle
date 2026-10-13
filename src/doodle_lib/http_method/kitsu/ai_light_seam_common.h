//
// AI 灯光：推理网关的**计费与准入层**（私有头，不进公共 API）
//
// 三条接缝（generate_spec / generate_candidates / structure_feedback）已随角色降级
// 全部删除，归前端。本文件因此不再是「接缝共用件」—— 它现在只装一样东西：
// 推理网关（ai_light.cpp）走的那条推理管道（凭据解析 → 任务信封 → 计费 → 成败判定）。
//
// 曾经这里还有一整块**提示词格式化工具**：join_strings / join_numbers / rgb_to_hex、
// build_common_lighting_section、g_axis_value_semantics_text，以及
// describe_targets / describe_rig_brief / describe_preset_brief 三个摘要投影。
// **已全部删除** —— 它们的唯一消费者是接缝 2 / 接缝 3 的提示词组装，而服务端已不再
// 自己拼提示词（网关只做原样透传，见 ai_light.cpp 的推理网关段）。
//
// 本头**不含任何 HTTP 依赖**
// （不 include http_jwt_fun / session_data / 推理客户端），单测可直接调用。
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>

namespace doodle::ai_light {


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
