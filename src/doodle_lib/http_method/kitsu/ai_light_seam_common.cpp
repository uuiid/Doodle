#include "ai_light_seam_common.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/seedance2/task.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <string>
#include <string_view>


namespace doodle::ai_light {

// `select` / `object` / `c` 这些 ORM 助手都在 doodle::orm 里，本文件通篇在用
// （编排器同样在这一层用了这条 using —— 从它那里搬过来的管道不能少这一行）。
using namespace orm;

// ---------------------------------------------------------------------------
// 推理管道（曾经由三条接缝共用；接缝现已全部删除，唯一调用方是推理网关 ai_light.cpp）
// ---------------------------------------------------------------------------
//
// 三条接缝的 `generate_spec`／`generate_candidates`／`structure_feedback` 原本各写一遍同样的 14 行
// （解析触发人 → 建客户端 → 组装任务信封 → 跑推理 → 判定成败）。任何一处改了 token 计费或任务字段，
// 另一处就悄悄落后 —— 收口到这一处后，「一次接缝推理长什么样」只有一个答案。
//
// 三条接缝现已全部删除（角色降级 · 第二梯队），但这条管道**仍然活着**：
// 唯一的调用方是**推理网关**（`ai_light.cpp`）。网关把「前端点名的模型 + 提示词」
// 原样转发出去，而凭据解析、任务信封、token 计费、成败判定全部沿用这里 ——
// 推理确实发生了，扣费凭证就必须照旧落库。
// 换句话说：本文件的职责从「接缝的实现细节」变成了「推理网关的计费与准入层」。

namespace {

struct resolved_inference_t {
  std::shared_ptr<http::seedance2::ai_client_base> client_{};
  uuid studio_id_{};
};

/// 触发人 → person → ai_studio → 推理客户端。
///
/// 用 select + to_optional 而不是 get_by_uuid<T>：后者内部是 to_single()，「查不到」会变成
/// 一条 sqlite 异常（HTTP 500），而这里应当是 400 语义。
/// 每次推理都**新建**客户端：http_client_ssl 内部只有一个 socket 且无锁（§15.6 第 6 条）。
/// `in_what` 只用于措辞（出错信息要指明哪次推理），不参与任何判定。
resolved_inference_t make_seam_client(sqlite_database& in_sql, const uuid& in_operator_id, std::string_view in_what) {
  DOODLE_CHICK_HTTP(
      !in_operator_id.is_nil(), bad_request, "{} 缺少触发人（operator_id），无法解析 AI 工作室与计费主体", in_what
  );
  const auto l_person = select(in_sql)
                            .columns(object<person>())
                            .from<person>()
                            .where(c(&person::uuid_id_) == in_operator_id)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_person.has_value(), bad_request, "{} 找不到触发人 {}（person 不存在），无法解析 AI 工作室", in_what,
      in_operator_id
  );
  DOODLE_CHICK_HTTP(
      !l_person->ai_studio_id_.is_nil(), bad_request, "{} 的触发人 {} 未关联 AI 工作室，无法调用推理接口", in_what,
      in_operator_id
  );
  const auto l_studio = select(in_sql)
                            .columns(object<ai_studio>())
                            .from<ai_studio>()
                            .where(c(&ai_studio::uuid_id_) == l_person->ai_studio_id_)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_studio.has_value(), bad_request, "{} 找不到 AI 工作室 {}（配置可能已被删除）", in_what, l_person->ai_studio_id_
  );

  resolved_inference_t l_out{};
  l_out.studio_id_ = l_studio->uuid_id_;
  l_out.client_    = http::seedance2::make_inference_client(*l_studio);
  return l_out;
}

}  // namespace

boost::asio::awaitable<seam_inference_result_t> run_seam_inference(
    sqlite_database& in_sql, const seam_request_base_t& in_request, const nlohmann::json& in_data_request,
    const uuid& in_iteration_id
) {
  // ---- 1) 触发人 → AI 工作室 → 客户端 ----
  auto l_resolved = make_seam_client(in_sql, in_request.operator_id_, "推理");

  // ---- 2) 组装任务与请求体 ----
  seedance2::task l_task{};
  l_task.type_                  = seedance2::task_type::inference;     // not_null，必须显式赋值
  l_task.backend_               = seedance2::task_backend::seedance2;  // 同上
  l_task.user_id_               = in_request.operator_id_;             // 计费主体 + 签名 URL 的 uid
  l_task.ai_studio_id_          = l_resolved.studio_id_;
  l_task.project_uuid_id_       = in_request.project_id_;
  // 与蒸馏阶段的区别：候选生成与反馈结构化的调用方在调用**之前**就把 iteration 的 uuid
  // 铸造好了，所以这条推理任务能反查到「哪一次推理生成了第 N 轮候选 / 结构化过哪一轮反馈」。
  // 为 nil（蒸馏阶段）表示这次推理不属于任何轮次。
  l_task.ai_light_iteration_id_ = in_iteration_id;
  // text_prompt_ 不在**这里**填：任务行要显示的那句话由 collect_request_info() 在
  // run_inference_task 落库时从请求体自己读。调用方再抄一份带进来的话，同一份提示词
  // 就有了第二个真相，而且抄进来的那版往往是截断过的摘要。
  l_task.completion_tokens_     = doodle_config::g_ark_inference_default_consumed_tokens;
  // 落库的就是**要发出去的那份** json, 不再经 inference_request_t 往返一趟: 那次往返是一份
  // 手写白名单, 网关透传进来的键(方舟的 input / tools / text.format…)会被静默吃掉。
  // 接缝路径本来就自组扁平信封(调用方传 to_json 的结果进来), 形状不变。
  l_task.data_request_          = in_data_request;

  // run_inference_task 自己一个事务里落终态任务行 + 扣费（§15.6 的方案 A）。
  // 这是接缝唯一的副作用，且**有意保留**：推理确实发生了，必须留痕计费 ——
  // 否则调用方回滚会话状态时会连带把 person_token 的扣费抹掉（契约 2 的受控例外）。
  auto l_result                 = co_await http::seedance2::run_inference_task(l_task, in_sql, l_resolved.client_);

  seam_inference_result_t l_out{};
  l_out.json_          = l_result.result_json_;
  l_out.input_tokens_  = l_result.input_tokens_;
  l_out.output_tokens_ = l_result.output_tokens_;
  l_out.total_tokens_  = l_result.total_tokens_;
  co_return l_out;
}

// 曾经这里还有一整块**提示词格式化工具**（本文件后 99 行）：
//   * join_strings / join_numbers / rgb_to_hex —— 字符串与颜色格式化
//   * build_common_lighting_section / g_axis_value_semantics_text —— 共用提示词文本段
//   * describe_targets / describe_rig_brief / describe_preset_brief —— 摘要投影
//   * describe_spec_brief —— 更早一步随 Spec 闸门删除
// **已全部删除** —— 它们的唯一消费者是接缝 2（候选生成）与接缝 3（反馈结构化）的
// 提示词组装。那两条接缝归前端后服务端不再自己拼提示词：推理网关（ai_light.cpp）
// 只做原样透传（前端给的键一律原样发往方舟 /responses）。保留会诱使后来人在服务端
// 重新长出一套提示词口径，与前端的设计分叉。

}  // namespace doodle::ai_light
