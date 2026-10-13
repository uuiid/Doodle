#include "ai_light_submit.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ai_light_rig_ops.h"
#include "ai_light_submit.h"

namespace doodle::http::ai_light {

// 命名空间遮蔽：接缝函数在 doodle::ai_light，本类在 doodle::http::ai_light。
// 同名内层命名空间会遮蔽外层，直接写 ai_light::xxx 会命中自己（见 ai_light.h:181-184）。
// 同 ai_light.cpp:50 的既有写法。
namespace {
using namespace doodle::ai_light;

// ===========================================================================
// IR 准入闸门（角色降级 · 第二梯队新增）
// ===========================================================================
//
// 降级前，这三条约束是**接缝的返回值**上的断言：模型给的东西不合法，服务端自己知道。
// 现在服务端已经没有那条接缝了（候选生成归前端），同样的三条约束就落在准入闸门上 ——
// 不合法的东西不许落库。措辞沿用已删除的 map_candidate_result() 的原句
// （原 ai_light_seam_candidates.cpp:481-523，该文件随接缝一起删除），
// 这样历史错误信息与前端日志能对上，排查时不必重新学一套措辞。
//
// 为什么闸门必须在服务端、不能只在前端：
//   前端可以因为 bug 或被绕过而提交畸形候选，而落库的候选会被 UE 拿去建灯。
//   6 张一样的图比一次失败更糟（美术会以为「这几个方向看起来一样」）。

/// 只比较「灯怎么打」的部分。与已删除的 ai_light_seam_candidates.cpp:95-97 的 rig_substance
/// 同义（那份随接缝一起删了，这里是唯一的一份）。
/// 身份字段（rig_id/iteration/candidate_index）、variant_axis_、provenance_、ops_
/// 都由服务端另填，拿它们判「候选是否与基线相同」会误判。
nlohmann::json rig_substance(const lighting_rig_t& in_rig) {
  return nlohmann::json{{"globals", in_rig.globals_}, {"lights", in_rig.lights_}};
}

/// 契约 3 的自洽检查（§6.6）：「同轮 6 个候选灯集合完全一致」是硬约束，
/// 降级后没有任何一环替前端保证它。
///
/// 这里自己遍历 const 版本，不复用别处的取值助手 —— 只读检查不该拿到可写引用。
bool has_key_light(const lighting_rig_t& in_rig) {
  return std::any_of(
      in_rig.lights_.begin(), in_rig.lights_.end(), [](const light_t& in_light) { return in_light.role_ == light_role::key; }
  );
}

/// 解析前端交上来的 `axes` 数组。**宽松**：识别不了的轴名直接丢弃并 WARN，
/// 不抛错 —— 轴计划是"建议怎么渲"，本轮真渲什么由 candidates 决定（它们是权威状态）。
/// 前端给的轴名不认识时，候选已经按它自己的理解展开了，作用是把轴写进轮次行做审计。
iteration_axes_t parse_submitted_axes(const nlohmann::json& in_json) {
  iteration_axes_t l_ret{};
  if (!in_json.contains("axes")) return l_ret;
  const auto& l_axes = in_json.at("axes");
  if (!l_axes.is_array()) {
    SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "提交的 axes 不是数组，按空轴计划处理");
    return l_ret;
  }
  for (const auto& l_item : l_axes) {
    if (!l_item.is_object()) continue;
    axis_step_t l_step{};
    const auto l_name = l_item.value("axis", std::string{});
    if (const auto l_kind = magic_enum::enum_cast<variant_axis_kind>(l_name); l_kind.has_value()) {
      l_step.axis_ = *l_kind;
    } else {
      SPDLOG_LOGGER_WARN(g_logger_ctrl().get_http(), "提交的轴名 {} 不认识，已忽略", l_name);
      continue;
    }
    if (l_item.contains("steps") && l_item.at("steps").is_array())
      l_step.steps_ = l_item.at("steps").get<std::vector<double>>();
    l_step.label_ = l_item.value("label", std::string{});
    l_ret.axes_.push_back(std::move(l_step));
  }
  l_ret.strategy_ = in_json.value("strategy", std::string{});
  return l_ret;
}

/// 查会话并做项目级权限校验。与 ai_light.cpp:59-63 的 load_session_checked 逐字同义 ——
/// 那个函数在 ai_light.cpp 的匿名 namespace 里，别的翻译单元取不到。
/// 会话不存在时 get_by_uuid → to_single 抛 sqlite_orm_exception{404}，
/// 由 http_session_data 统一翻成 404，这里不自己 try/catch。
ai_light_session submit_load_session_checked(
    const http_jwt_fun::http_jwt_t& in_person, sqlite_database& in_sql, const uuid& in_session_id
) {
  auto l_session = in_sql.get_by_uuid<ai_light_session>(in_session_id);
  in_person.check_project_access(l_session.project_id_);
  return l_session;
}

/// 解析一个候选的 rig。身份字段（session/spec/iteration/candidate_index/rig_id/provenance）
/// **不从前端取**，由调用方在落库前补齐 —— 沿用已删除的 map_candidate_result() 的
/// 「身份字段由服务端钉死」那条纪律（原 ai_light_seam_candidates.cpp:504-513）。
lighting_rig_t parse_submitted_rig(const nlohmann::json& in_json, std::int32_t in_index) {
  DOODLE_CHICK_HTTP(
      in_json.is_object(), bad_request, "候选 {} 不是 JSON 对象", in_index
  );
  DOODLE_CHICK_HTTP(
      in_json.contains("globals") && in_json.at("globals").is_object(), bad_request,
      "候选 {} 缺少 globals 对象", in_index
  );
  DOODLE_CHICK_HTTP(
      in_json.contains("lights") && in_json.at("lights").is_array() && !in_json.at("lights").empty(), bad_request,
      "候选 {} 缺少非空的 lights 数组", in_index
  );

  lighting_rig_t l_rig{};
  // globals / lights 走 NLohmann 的 from_json（rig.h:565-597 已定义）。
  // 这里直接 get_to 硬解析，不走「接缝读取器」那套（曾经有过 ai_light_json_validation.h）：
  // 那套是给模型输出用的（抛 bad_gateway 记在接缝上），前端提交的形状错误是 400。
  // 两者的错误归属不同，措辞也不同。
  in_json.at("globals").get_to(l_rig.globals_);
  in_json.at("lights").get_to(l_rig.lights_);
  // ops 是审计影子（决定 D-58 路线 A）：收下但不参与任何判定，也不用于建灯。
  if (in_json.contains("ops") && in_json.at("ops").is_array()) in_json.at("ops").get_to(l_rig.ops_);
  // baseline_level_ 是只读字段，永不接受前端给的值 —— 它记录的是这一轮渲染的基线关卡，
  // 由服务端从装配推导填（下方调用方负责）。前端给了也覆盖掉。

  // variant_axis_：可选。基线轮（index 0）**必须**是 none，其余候选应当有轴。
  if (in_json.contains("variant_axis")) {
    variant_axis_t l_axis{};
    const auto l_name = in_json.at("variant_axis").is_string()
                            ? in_json.at("variant_axis").get<std::string>()
                            : std::string{};
    if (const auto l_kind = magic_enum::enum_cast<variant_axis_kind>(l_name); l_kind.has_value())
      l_axis.axis_ = *l_kind;
    l_axis.value_ = in_json.value("variant_value", 0.0);
    l_axis.label_ = in_json.value("variant_label", std::string{});
    l_rig.variant_axis_ = std::move(l_axis);
  }
  return l_rig;
}

}  // namespace

// ===========================================================================
// GET 不支持
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(iteration_submit_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_body    = in_handle->get_json();

  // 会话在路径上，操作者只从 JWT 取 —— 与网关同一条纪律（结构上不可伪造）。
  auto l_session = submit_load_session_checked(person_, l_sql, session_id_);

  // 会话必须可以干活：draft（刚建会话就提交）或 generating（正常路径 / 再出一轮）。
  // 取消人工闸门 1 之后这里放行 draft —— 服务端不再要求「先确认输出版本」。
  // awaiting_judgement 明确不放行：上一轮候选还没被判断，此时提交会让它们变成孤儿。
  DOODLE_CHICK_HTTP(
      can_submit_candidates(l_session.state_), bad_request,
      "会话 {} 处于状态 {}，不能提交候选（需要 draft 或 generating）", l_session.uuid_id_,
      magic_enum::enum_name(l_session.state_)
  );

  // ---- 闸门 1：候选数必须恰好 g_candidate_count ----
  DOODLE_CHICK_HTTP(
      l_body.contains("candidates") && l_body.at("candidates").is_array(), bad_request,
      "请求体缺少 candidates 数组"
  );
  const auto& l_candidates_json = l_body.at("candidates");
  DOODLE_CHICK_HTTP(
      l_candidates_json.size() == static_cast<std::size_t>(g_candidate_count), bad_request,
      "提交了 {} 个候选，方案 §6.6 要求恰好 {} 个", l_candidates_json.size(), g_candidate_count
  );

  // ---- 解析 rigs ----
  std::vector<lighting_rig_t> l_rigs{};
  l_rigs.reserve(static_cast<std::size_t>(g_candidate_count));
  for (std::int32_t l_i = 0; l_i < g_candidate_count; ++l_i)
    l_rigs.push_back(parse_submitted_rig(l_candidates_json.at(static_cast<std::size_t>(l_i)), l_i));

  // ---- 闸门 2：至少一个 role=key 主光（逐字沿用接缝措辞）----
  // 缺主光时 key_direction / key_fill_ratio 两个轴都是空操作（§9.1）。
  // 基线是 index 0（前端展开时以它为基准）。
  DOODLE_CHICK_HTTP(
      has_key_light(l_rigs.front()), bad_request,
      "候选基线 rig 里没有 role=key 的主光：key_direction / key_fill_ratio 两个轴都会变成空操作，"
      "§9.1 要求的「候选之间真的在目标上有差异」无法成立"
  );

  // ---- 闸门 3：相邻候选 substance 必须不同（逐字沿用接缝措辞）----
  // 返回 6 张一样的图比一次失败更糟 —— 那会让美术以为「这几个方向看起来一样」。
  {
    const auto l_base_substance = rig_substance(l_rigs.front());
    for (std::int32_t l_i = 1; l_i < g_candidate_count; ++l_i) {
      DOODLE_CHICK_HTTP(
          !(rig_substance(l_rigs.at(static_cast<std::size_t>(l_i))) == l_base_substance), bad_request,
          "候选 {} 沿轴 {} = {} 展开后与基线完全相同（这条轴在当前 rig 上不生效）", l_i,
          magic_enum::enum_name(l_rigs.at(static_cast<std::size_t>(l_i)).variant_axis_.axis_),
          l_rigs.at(static_cast<std::size_t>(l_i)).variant_axis_.value_
      );
    }
  }

  // ---- 闸门 4：iteration 号必须是正整数，且等于 current_iteration_ + 1 ----
  DOODLE_CHICK_HTTP(l_body.contains("iteration"), bad_request, "请求体缺少 iteration");
  const auto l_iteration = l_body.at("iteration").get<std::int32_t>();
  DOODLE_CHICK_HTTP(
      l_iteration > 0, bad_request, "iteration 必须是正整数，收到 {}", l_iteration
  );
  DOODLE_CHICK_HTTP(
      l_iteration == l_session.current_iteration_ + 1, bad_request,
      "提交的轮次号 {} 与会话期望的不一致（当前 {}，期望 {}）", l_iteration, l_session.current_iteration_,
      l_session.current_iteration_ + 1
  );

  // ---- 调用共用落库件 ----
  // 说明：候选的落库、iteration 行、session 推进、launch_plan 装配全部复用
  // write_submitted_iteration()（ai_light_orchestrator.h）。那一段原本在
  // advance_session_generating_branch() 里（已随候选生成降级一起删除），
  // 现在只剩这一条提交路线 —— 落库逻辑不再有第二份实现可漂移。
  submit_iteration_request_t l_req{};
  l_req.session_id_    = l_session.uuid_id_;
  l_req.project_id_    = l_session.project_id_;
  l_req.operator_id_   = person_.person_.uuid_id_;
  l_req.iteration_     = l_iteration;
  l_req.axes_          = parse_submitted_axes(l_body);
  l_req.rigs_          = std::move(l_rigs);
  l_req.task_id_       = l_session.task_id_;  // 装配推导要它定位镜头

  // 引擎内路线：这条路线不建任务行、不广播，所以任务号出参在这里必然保持 nil
  // （出参是给「服务端自己生成候选」那条队列路线用的）。
  uuid l_unused_task_id{};
  auto l_plan = co_await write_submitted_iteration(l_sql, l_req, true, l_unused_task_id);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 向 AI 灯光会话 {} 提交了第 {} 轮 {} 个候选（前端展开）",
      person_.person_.email_, person_.person_.get_full_name(), l_session.uuid_id_, l_iteration, g_candidate_count
  );

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  nlohmann::json l_ret{};
  l_ret = *l_detail;  // 赋值而非 {} 构造：见 nlohmann::json{x} 的花括号陷阱
  if (l_plan.has_value()) l_ret["launch_plan"] = *l_plan;
  co_return in_handle->make_msg(l_ret);
}

}  // namespace doodle::http::ai_light
