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

#include "ai_light_submit.h"
#include <algorithm>
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace doodle::http::ai_light {

// 命名空间遮蔽：接缝函数在 doodle::ai_light，本类在 doodle::http::ai_light。
// 同名内层命名空间会遮蔽外层，直接写 ai_light::xxx 会命中自己（见 ai_light.h:181-184）。
// 同 ai_light.cpp:50 的既有写法。
using namespace doodle::ai_light;

// ===========================================================================
// GET 不支持
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(iteration_submit_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_body    = in_handle->get_json();

  // 会话在路径上，操作者只从 JWT 取 —— 与网关同一条纪律（结构上不可伪造）。
  // 会话不存在时 get_by_uuid → to_single 抛 sqlite_orm_exception{404}，
  // 由 http_session_data 统一翻成 404，这里不自己 try/catch。
  auto l_session = l_sql.get_by_uuid<ai_light_session>(session_id_);
  person_.check_project_access(l_session.project_id_);

  // 会话必须可以干活：draft（刚建会话就提交）或 generating（正常路径 / 再出一轮）。
  // 取消人工闸门 1 之后这里放行 draft —— 服务端不再要求「先确认输出版本」。
  // awaiting_judgement 明确不放行：上一轮候选还没被判断，此时提交会让它们变成孤儿。
  DOODLE_CHICK_HTTP(
      can_submit_candidates(l_session.state_), bad_request,
      "会话 {} 处于状态 {}，不能提交候选（需要 draft 或 generating）", l_session.uuid_id_,
      magic_enum::enum_name(l_session.state_)
  );

  // ---- 直接反序列化请求体 ----
  // 形状见 ai_light_submit.h 的请求体段：`candidates` 就是 lighting_rig_t 的数组
  // （variant_axis 是嵌套对象），`axes` 是 iteration_axes_t。
  // 形状不合法（缺 candidates / 类型对不上）在这里抛 type_error —— 由 http_session_data
  // 统一翻成 400，与下面各闸门的错误归属一致。
  submit_iteration_request_t l_req{};
  l_body.get_to(l_req);

  // 候选数由前端决定，服务端只要求非空 —— 后面两道闸门都要读 front()，
  // 而且「一个候选都没提交」不可能是一轮有效的渲染请求。
  DOODLE_CHICK_HTTP(
      !l_req.rigs_.empty(), bad_request, "请求体缺少 candidates 数组（或为空）"
  );

  // ===========================================================================
  // IR 准入闸门
  // ===========================================================================
  // 候选生成归前端，所以「模型给的东西合不合法」这件事从接缝的返回值断言
  // 移到了这道准入闸门：不合法的东西不许落库。
  //
  // 为什么闸门必须在服务端、不能只在前端：
  //   前端可以因为 bug 或被绕过而提交畸形候选，而落库的候选会被 UE 拿去建灯。
  //   一批一样的图比一次失败更糟（美术会以为「这几个方向看起来一样」）。

  // ---- 闸门 1：每个候选要有非空的 lights ----
  // from_json 是宽松的（缺键不抛），所以形状校验在这里补上。
  // 按**实际提交的数量**遍历，数量由前端决定。
  for (std::size_t l_i = 0; l_i < l_req.rigs_.size(); ++l_i) {
    DOODLE_CHICK_HTTP(
        !l_req.rigs_.at(l_i).lights_.empty(), bad_request, "候选 {} 缺少非空的 lights 数组", l_i
    );
  }

  // ---- 闸门 2：至少一个 role=key 主光 ----
  // 缺主光时 key_direction / key_fill_ratio 两个轴都是空操作（§9.1）。
  // 基线是 index 0（前端展开时以它为基准）。
  // 这里自己遍历 const 版本，不复用别处的取值助手 —— 只读检查不该拿到可写引用。
  DOODLE_CHICK_HTTP(
      std::any_of(
          l_req.rigs_.front().lights_.begin(), l_req.rigs_.front().lights_.end(),
          [](const light_t& in_light) { return in_light.role_ == light_role::key; }
      ),
      bad_request,
      "候选基线 rig 里没有 role=key 的主光：key_direction / key_fill_ratio 两个轴都会变成空操作，"
      "§9.1 要求的「候选之间真的在目标上有差异」无法成立"
  );

  // ---- 闸门 3：相邻候选 substance 必须不同 ----
  // 返回一批一样的图比一次失败更糟 —— 那会让美术以为「这几个方向看起来一样」。
  //
  // substance 只比较「灯怎么打」的部分（globals + lights）：身份字段
  // （rig_id/iteration/candidate_index）、variant_axis_、provenance_、ops_ 都由服务端另填，
  // 拿它们判「候选是否与基线相同」会误判。
  {
    const auto l_to_substance = [](const lighting_rig_t& in_rig) {
      return nlohmann::json{{"globals", in_rig.globals_}, {"lights", in_rig.lights_}};
    };
    const auto l_base_substance = l_to_substance(l_req.rigs_.front());
    for (std::size_t l_i = 1; l_i < l_req.rigs_.size(); ++l_i) {
      const auto& l_rig = l_req.rigs_.at(l_i);
      DOODLE_CHICK_HTTP(
          !(l_to_substance(l_rig) == l_base_substance), bad_request,
          "候选 {} 沿轴 {} = {} 展开后与基线完全相同（这条轴在当前 rig 上不生效）", l_i,
          magic_enum::enum_name(l_rig.variant_axis_.axis_), l_rig.variant_axis_.value_
      );
    }
  }

  // ---- 闸门 4：iteration 号必须是正整数，且等于 current_iteration_ + 1 ----
  DOODLE_CHICK_HTTP(l_req.iteration_ > 0, bad_request, "iteration 必须是正整数，收到 {}", l_req.iteration_);
  DOODLE_CHICK_HTTP(
      l_req.iteration_ == l_session.current_iteration_ + 1, bad_request,
      "提交的轮次号 {} 与会话期望的不一致（当前 {}，期望 {}）", l_req.iteration_, l_session.current_iteration_,
      l_session.current_iteration_ + 1
  );

  // ---- 身份字段一律由服务端钉死，前端给了也不采信 ----
  // 从路径取会话、从 JWT 取操作者（结构上不可伪造）；project / task 从会话行取。
  l_req.session_id_  = l_session.uuid_id_;
  l_req.project_id_  = l_session.project_id_;
  l_req.operator_id_ = person_.person_.uuid_id_;
  l_req.task_id_     = l_session.task_id_;

  // 引擎内路线：这条路线不建任务行、不广播，所以任务号出参在这里必然保持 nil
  // （出参是给「服务端自己生成候选」那条队列路线用的）。
  uuid l_unused_task_id{};
  auto l_plan = co_await write_submitted_iteration(l_sql, l_req, true, l_unused_task_id);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 向 AI 灯光会话 {} 提交了第 {} 轮 {} 个候选（前端展开）",
      person_.person_.email_, person_.person_.get_full_name(), l_session.uuid_id_, l_req.iteration_,
      l_req.rigs_.size()
  );

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  nlohmann::json l_ret{};
  l_ret = *l_detail;  // 赋值而非 {} 构造：见 nlohmann::json{x} 的花括号陷阱
  if (l_plan.has_value()) l_ret["launch_plan"] = *l_plan;
  co_return in_handle->make_msg(l_ret);
}

}  // namespace doodle::http::ai_light
