
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

#include <algorithm>
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace doodle::http::ai_light {

using namespace doodle::ai_light;

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(iteration_submit_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_body    = in_handle->get_json();

  auto l_session = l_sql.get_by_uuid<ai_light_session>(session_id_);
  person_.check_project_access(l_session.project_id_);

  DOODLE_CHICK_HTTP(
      can_submit_candidates(l_session.state_), bad_request,
      "会话 {} 处于状态 {}，不能提交候选（需要 draft 或 generating）", l_session.uuid_id_,
      magic_enum::enum_name(l_session.state_)
  );

  submit_iteration_request_t l_req{};
  l_body.get_to(l_req);

  DOODLE_CHICK_HTTP(!l_req.rigs_.empty(), bad_request, "请求体缺少 candidates 数组（或为空）");

  DOODLE_CHICK_HTTP(l_req.iteration_ > 0, bad_request, "iteration 必须是正整数，收到 {}", l_req.iteration_);
  DOODLE_CHICK_HTTP(
      l_req.iteration_ == l_session.current_iteration_ + 1, bad_request,
      "提交的轮次号 {} 与会话期望的不一致（当前 {}，期望 {}）", l_req.iteration_, l_session.current_iteration_,
      l_session.current_iteration_ + 1
  );

  l_req.session_id_  = l_session.uuid_id_;
  l_req.project_id_  = l_session.project_id_;
  l_req.operator_id_ = person_.person_.uuid_id_;
  l_req.task_id_     = l_session.task_id_;
  DOODLE_CHICK_HTTP(!l_req.task_id_.is_nil(), bad_request, "会话 {} 的任务 ID 为空", l_session.uuid_id_);

  auto l_plan = co_await write_submitted_iteration(l_sql, l_req, true);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 向 AI 灯光会话 {} 提交了第 {} 轮 {} 个候选（前端展开）",
      person_.person_.email_, person_.person_.get_full_name(), l_session.uuid_id_, l_req.iteration_, l_req.rigs_.size()
  );

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  nlohmann::json l_ret{};
  l_ret = *l_detail;
  if (l_plan.has_value()) l_ret["launch_plan"] = *l_plan;
  co_return in_handle->make_msg(l_ret);
}

}  // namespace doodle::http::ai_light
