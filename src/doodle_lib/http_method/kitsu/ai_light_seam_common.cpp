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

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::ai_light {

// `select` / `object` / `c` 这些 ORM 助手都在 doodle::orm 里，本文件通篇在用
// （编排器同样在这一层用了这条 using —— 从它那里搬过来的管道不能少这一行）。
using namespace orm;

// ---------------------------------------------------------------------------
// 接缝共用的推理管道
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
resolved_inference_t make_seam_client(
    sqlite_database& in_sql, const uuid& in_operator_id, std::string_view in_what
) {
  DOODLE_CHICK_HTTP(
      !in_operator_id.is_nil(), bad_request, "{} 缺少触发人（operator_id），无法解析 AI 工作室与计费主体",
      in_what
  );
  const auto l_person =
      select(in_sql).columns(object<person>()).from<person>().where(c(&person::uuid_id_) == in_operator_id)()
          .to_optional();
  DOODLE_CHICK_HTTP(
      l_person.has_value(), bad_request, "{} 找不到触发人 {}（person 不存在），无法解析 AI 工作室", in_what,
      in_operator_id
  );
  DOODLE_CHICK_HTTP(
      !l_person->ai_studio_id_.is_nil(), bad_request, "{} 的触发人 {} 未关联 AI 工作室，无法调用推理接口",
      in_what, in_operator_id
  );
  const auto l_studio = select(in_sql)
                            .columns(object<ai_studio>())
                            .from<ai_studio>()
                            .where(c(&ai_studio::uuid_id_) == l_person->ai_studio_id_)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_studio.has_value(), bad_request, "{} 找不到 AI 工作室 {}（配置可能已被删除）", in_what,
      l_person->ai_studio_id_
  );

  resolved_inference_t l_out{};
  l_out.studio_id_ = l_studio->uuid_id_;
  l_out.client_    = http::seedance2::make_inference_client(*l_studio);
  return l_out;
}

/// 一次推理的成败判定。重试全在客户端内做完了（g_ark_inference_max_retry），这里不重试。
/// 内容违规 → 400（可解释：改提示词/换图能解决），其余失败 → 502；
/// result_json_ 非 object（含 null）也是 502 —— 输出不合解析不是调用方的错。
void chick_inference_ok(const http::seedance2::ai_client_base::run_task_result_t& in_result, std::string_view in_what) {
  const auto l_brief_of = [&in_result]() {
    return in_result.result_text_.substr(0, std::min<std::size_t>(200, in_result.result_text_.size()));
  };
  if (in_result.status_ != seedance2::task_status::succeeded) {
    const auto l_brief = l_brief_of();
    if (http::seedance2::seedance2_client::is_sensitive_content_error(in_result.data_response_)) {
      throw_exception(doodle_error{
          boost::beast::http::status::bad_request, "{} 被内容安全策略拒绝：{}", in_what,
          l_brief.empty() ? in_result.error_code_ : l_brief
      });
    }
    throw_exception(doodle_error{
        boost::beast::http::status::bad_gateway, "{} 推理失败（status={}, error_code={}, 超时={}）：{}", in_what,
        magic_enum::enum_name(in_result.status_), in_result.error_code_, in_result.is_timeout_, l_brief
    });
  }
  // result_json_ 由 parse_inference_response 填好（内部已做过 strip_code_fence + parse），
  // 调用方**不要**二次解析 result_text_。
  if (!in_result.result_json_.is_object()) {
    throw_exception(doodle_error{
        boost::beast::http::status::bad_gateway, "{} 的模型输出不是合法 JSON 对象：{}", in_what, l_brief_of()
    });
  }
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
  l_task.type_                  = seedance2::task_type::inference;  // not_null，必须显式赋值
  l_task.backend_               = seedance2::task_backend::seedance2;  // 同上
  l_task.user_id_               = in_request.operator_id_;  // 计费主体 + 签名 URL 的 uid
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
  auto l_result = co_await http::seedance2::run_inference_task(l_task, in_sql, l_resolved.client_);

  // ---- 3) 成败判定（内容违规 400 / 其它失败 502 / 输出非 object 502）----
  chick_inference_ok(l_result, "推理");

  seam_inference_result_t l_out{};
  l_out.json_          = l_result.result_json_;
  l_out.input_tokens_  = l_result.input_tokens_;
  l_out.output_tokens_ = l_result.output_tokens_;
  l_out.total_tokens_  = l_result.total_tokens_;
  co_return l_out;
}

std::string join_strings(const std::vector<std::string>& in_values, std::string_view in_sep) {
  std::string l_ret{};
  for (std::size_t l_i = 0; l_i < in_values.size(); ++l_i) {
    if (l_i != 0) l_ret += in_sep;
    l_ret += in_values[l_i];
  }
  return l_ret;
}

std::string rgb_to_hex(const vec3_t& in_color) {
  const auto l_quantize = [](double in_v) {
    return static_cast<int>(std::lround(std::clamp(in_v, 0.0, 1.0) * 255.0));
  };
  return fmt::format("#{:02x}{:02x}{:02x}", l_quantize(in_color.x_), l_quantize(in_color.y_), l_quantize(in_color.z_));
}

/// 「提示词未提及 + 参考冲突 → 不设目标」这一支由聚合器完成：冲突维度不写进硬目标。
std::string describe_targets(const lighting_targets_t& in_targets) {
  std::string l_out{};
  l_out += fmt::format(
      "  contrast=[{}, {}]  white_balance_kelvin=[{}, {}]\n", in_targets.contrast_.low_, in_targets.contrast_.high_,
      in_targets.white_balance_kelvin_.low_, in_targets.white_balance_kelvin_.high_
  );
  l_out += fmt::format(
      "  luma p10=[{}, {}]  p50=[{}, {}]  p90=[{}, {}]\n", in_targets.luma_percentiles_.p10_.low_,
      in_targets.luma_percentiles_.p10_.high_, in_targets.luma_percentiles_.p50_.low_,
      in_targets.luma_percentiles_.p50_.high_, in_targets.luma_percentiles_.p90_.low_,
      in_targets.luma_percentiles_.p90_.high_
  );
  l_out += fmt::format(
      "  clip_rate high=[{}, {}]  low=[{}, {}]\n", in_targets.clip_rate_.high_.low_, in_targets.clip_rate_.high_.high_,
      in_targets.clip_rate_.low_.low_, in_targets.clip_rate_.low_.high_
  );
  l_out += fmt::format(
      "  grading_hints: vignette={} lifted_blacks={} bloom={}\n", in_targets.grading_hints_.vignette_,
      in_targets.grading_hints_.lifted_blacks_ ? "true" : "false", in_targets.grading_hints_.bloom_
  );
  return l_out;
}

std::string build_common_lighting_section() {
  return std::string{R"LIGHT(【职责边界：你决定什么、不决定什么】
你决定：灯的数量、角色、类型、相对方位；环境光 / 雾 / 天空 / 曝光 / 调色的整体倾向；增删改哪些灯（结构决策）。
你不决定：最终精确强度 / 色温 / 距离的数值；直接写 UE API 调用或代码；逐帧关键帧的精确数值；预测渲染结果。
绝对禁止：改变场景几何或材质。你的可改范围只有「灯光 + 环境光/雾/天空 + 后处理/调色」。

【单位与格式口径】
- 对比度 = 显示空间（sRGB）亮度分位差 p90 − p10，不是标准差。
- 色温用 Kelvin。
- 所有区间都是长度为 2 的数组 [low, high]，且 low <= high。
- 亮度分位 p10 / p50 / p90 是显示空间的归一化值（0 = 黑，1 = 白）。
- 削波率 clip_rate 是 [0, 1] 的比例。
- grading_hints.vignette / bloom 只能是 none / subtle / moderate / strong。
)LIGHT"};
}

std::string join_numbers(const std::vector<double>& in_values) {
  std::string l_ret{};
  for (std::size_t l_i = 0; l_i < in_values.size(); ++l_i) {
    if (l_i != 0) l_ret += ", ";
    l_ret += fmt::format("{}", in_values[l_i]);
  }
  return l_ret;
}

std::string describe_rig_brief(const lighting_rig_t& in_rig) {
  std::string l_out = fmt::format(
      "  globals: exposure_bias={:.2f} white_balance_kelvin={:.0f} sky_light_intensity={:.2f} fog_density={:.2f}\n"
      "           grading(contrast={:.2f} saturation={:.2f} lift={:.2f} vignette={:.2f} bloom={:.2f})\n",
      in_rig.globals_.exposure_.ev100_bias_, in_rig.globals_.white_balance_.kelvin_,
      in_rig.globals_.environment_.sky_light_intensity_, in_rig.globals_.environment_.fog_density_,
      in_rig.globals_.grading_.contrast_, in_rig.globals_.grading_.saturation_, in_rig.globals_.grading_.lift_,
      in_rig.globals_.grading_.vignette_, in_rig.globals_.grading_.bloom_
  );
  l_out += fmt::format("  灯 {} 个：\n", in_rig.lights_.size());
  for (const auto& l_light : in_rig.lights_) {
    l_out += fmt::format(
        "    role={} type={} azimuth={:.1f} elevation={:.1f} distance={:.2f} intensity={:.1f}({}) "
        "kelvin={:.0f} softness={:.2f} radius_cm={:.1f} cast_shadows={}\n",
        magic_enum::enum_name(l_light.role_), magic_enum::enum_name(l_light.type_), l_light.offset_.azimuth_deg_,
        l_light.offset_.elevation_deg_, l_light.offset_.distance_m_, l_light.intensity_.value_,
        magic_enum::enum_name(l_light.intensity_.unit_), l_light.color_.kelvin_, l_light.shadows_.softness_,
        l_light.shape_.source_radius_cm_, l_light.shadows_.cast_ ? "true" : "false"
    );
  }
  return l_out;
}

// 曾经这里还有 describe_spec_brief(const lighting_spec_t&) —— Spec 的紧凑投影。
// **已删除**：它的唯一调用方是接缝 3 的 build_feedback_input_text，而那条接缝已整条移除；
// 它的入参类型 lighting_spec_t 本身也随 Spec 闸门删除。参考图目标若还要投影给人看，
// 直接用下面的 describe_targets（它收的是 lighting_targets_t，仍然活着）。

std::string describe_preset_brief(const ai_light_preset& in_preset) {
  const auto l_target = in_preset.get_stats_target();
  return fmt::format(
      "  - {}  tags=[{}]  stats_target(contrast=[{}, {}] kelvin=[{}, {}])\n", in_preset.name_,
      join_strings(in_preset.tags_, ", "), l_target.contrast_.low_, l_target.contrast_.high_,
      l_target.white_balance_kelvin_.low_, l_target.white_balance_kelvin_.high_
  );
}

}  // namespace doodle::ai_light
