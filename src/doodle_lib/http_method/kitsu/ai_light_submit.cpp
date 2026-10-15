
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/http_method/kitsu/auto_task.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace doodle::http::ai_light {

using namespace doodle::ai_light;
using namespace doodle::orm;

namespace {

orm::sql_modify_statement_vector_t write_submitted_iteration(
    sqlite_database& in_sql, const submit_iteration_request_t& in_request
) {
  const auto l_iteration = in_request.iteration_;

  DOODLE_CHICK_HTTP(
      !in_request.rigs_.empty(), internal_server_error, "本轮候选数为 0，方案 §6.6 要求至少一个候选"
  );

  http::auto_task::shot_render_light_sync_result l_assembly_res{};
  std::string l_assembly_err{};
  if (in_request.task_id_.is_nil()) {
    l_assembly_err = "会话没有关联镜头任务（task_id 为空）";
  } else {
    try {
      l_assembly_res = http::auto_task::shot_render_light_sync_only(in_request.project_id_, in_request.task_id_);
    } catch (const std::exception& in_err) {
      l_assembly_err = in_err.what();
    }
  }
  DOODLE_CHICK_HTTP(
      l_assembly_err.empty(), internal_server_error,
      "AI 灯光会话 {} 第 {} 轮：无法推导本镜头的渲染装配，本轮无法渲染（{}）", in_request.session_id_, l_iteration,
      l_assembly_err
  );

  ai_light_iteration l_it{};
  l_it.uuid_id_    = core_set::get_set().get_uuid();
  l_it.session_id_ = in_request.session_id_;
  l_it.iteration_  = l_iteration;
  l_it.state_      = ai_light_iteration_state::generated;
  l_it.server_task_info_id_ = uuid{};

  orm::sql_modify_statement_vector_t l_sqls{};
  for (std::size_t l_i = 0; l_i < in_request.rigs_.size(); ++l_i) {
    auto l_rig             = in_request.rigs_.at(l_i);
    l_rig.session_id_      = in_request.session_id_;
    l_rig.iteration_       = l_iteration;
    l_rig.candidate_index_ = static_cast<std::int32_t>(l_i);
    l_rig.rig_id_          = uuid{};
    l_rig.baseline_level_  = l_assembly_res.original_map_.generic_string();

    ai_light_candidate l_cand{};
    l_cand.uuid_id_         = core_set::get_set().get_uuid();
    l_cand.iteration_id_    = l_it.uuid_id_;
    l_cand.session_id_      = in_request.session_id_;
    l_cand.candidate_index_ = l_i;
    l_cand.set_rig(l_rig);
 
    l_cand.render_status_ = candidate_render_status::pending;

    l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_candidate>().values(l_cand));
  }

  auto l_session_next               = in_sql.get_by_uuid<ai_light_session>(in_request.session_id_);
  l_session_next.current_iteration_ = l_iteration;
  l_session_next.failure_reason_    = "";

  l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_iteration>().values(l_it));
  l_sqls.emplace_back(
      orm::update(in_sql)
          .from<ai_light_session>()
          .set(
              c(&ai_light_session::state_)             = l_session_next.state_,
              c(&ai_light_session::current_iteration_) = l_session_next.current_iteration_,
              c(&ai_light_session::judgement_due_at_)  = l_session_next.judgement_due_at_,
              c(&ai_light_session::failure_reason_)    = l_session_next.failure_reason_
          )
          .where(c(&ai_light_session::uuid_id_) == l_session_next.uuid_id_)
  );

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(),
      "AI 灯光会话 {} 第 {} 轮 {} 个候选已落库（引擎内路线，等待发起方本机渲染）", in_request.session_id_, l_iteration,
      in_request.rigs_.size()
  );
  return l_sqls;
}

orm::sql_modify_statement_vector_t write_dispatched_iteration(
    sqlite_database& in_sql, const submit_iteration_request_t& in_request
) {
  const auto l_iteration = in_request.iteration_;

  DOODLE_CHICK_HTTP(
      !in_request.rigs_.empty(), internal_server_error, "本轮候选数为 0，方案 §6.6 要求至少一个候选"
  );

  http::auto_task::shot_render_light_sync_result l_assembly_res{};
  std::string l_assembly_err{};
  if (in_request.task_id_.is_nil()) {
    l_assembly_err = "会话没有关联镜头任务（task_id 为空）";
  } else {
    try {
      l_assembly_res = http::auto_task::shot_render_light_sync_only(in_request.project_id_, in_request.task_id_);
    } catch (const std::exception& in_err) {
      l_assembly_err = in_err.what();
    }
  }
  DOODLE_CHICK_HTTP(
      l_assembly_err.empty(), internal_server_error,
      "AI 灯光会话 {} 第 {} 轮：无法推导本镜头的渲染装配，本轮无法渲染（{}）", in_request.session_id_, l_iteration,
      l_assembly_err
  );

  ai_light_iteration l_it{};
  l_it.uuid_id_    = core_set::get_set().get_uuid();
  l_it.session_id_ = in_request.session_id_;
  l_it.iteration_  = l_iteration;
  l_it.state_      = ai_light_iteration_state::generated;

  const auto l_out_dir = make_iteration_relative_dir(in_request.session_id_, l_it.uuid_id_);

  orm::sql_modify_statement_vector_t l_sqls{};
  std::vector<ai_light_task_candidate_t> l_payload_candidates{};
  l_payload_candidates.reserve(in_request.rigs_.size());
  for (std::size_t l_i = 0; l_i < in_request.rigs_.size(); ++l_i) {
    auto l_rig             = in_request.rigs_.at(l_i);
    l_rig.session_id_      = in_request.session_id_;
    l_rig.iteration_       = l_iteration;
    l_rig.candidate_index_ = static_cast<std::int32_t>(l_i);
    l_rig.rig_id_          = uuid{};
    l_rig.baseline_level_  = l_assembly_res.original_map_.generic_string();

    ai_light_candidate l_cand{};
    l_cand.uuid_id_         = core_set::get_set().get_uuid();
    l_cand.iteration_id_    = l_it.uuid_id_;
    l_cand.session_id_      = in_request.session_id_;
    l_cand.candidate_index_ = l_i;
    l_cand.set_rig(l_rig);
    l_cand.sync_from_rig();
    l_cand.render_status_ = candidate_render_status::pending;

    l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_candidate>().values(l_cand));

    l_payload_candidates.push_back(
        ai_light_task_candidate_t{
            static_cast<std::int32_t>(l_i), l_cand.uuid_id_, make_candidate_frames_dir(l_cand.uuid_id_),
            make_candidate_file_name(l_cand.uuid_id_, false), make_candidate_file_name(l_cand.uuid_id_, true),
            make_candidate_movie_file(l_cand.uuid_id_), l_rig
        }
    );
  }

  auto l_session_next               = in_sql.get_by_uuid<ai_light_session>(in_request.session_id_);
  l_session_next.current_iteration_ = l_iteration;
  l_session_next.failure_reason_    = "";

  ai_light_task_payload_t l_payload{};
  l_payload.session_id_            = in_request.session_id_;
  l_payload.iteration_             = l_iteration;
  l_payload.out_dir_               = l_out_dir;
  l_payload.baseline_level_        = l_assembly_res.original_map_.generic_string();
  l_payload.render_map_            = l_assembly_res.render_map_.generic_string();
  l_payload.level_sequence_        = l_assembly_res.level_sequence_import_.generic_string();
  l_payload.movie_pipeline_config_ = l_assembly_res.movie_pipeline_config_.generic_string();
  l_payload.frame_begin_           = l_assembly_res.begin_time_;
  l_payload.frame_end_             = l_assembly_res.end_time_;
  l_payload.ue_project_path_       = l_assembly_res.uproject_file_name_.generic_string();
  l_payload.camera_file_path_      = l_assembly_res.camera_file_path_.generic_string();
  l_payload.project_dir_           = l_assembly_res.project_dir_;
  l_payload.assembly_files_        = std::move(l_assembly_res.assembly_files_);
  l_payload.candidates_            = std::move(l_payload_candidates);

  server_task_info l_task{};
  l_task.name_ =
      fmt::format("AI 灯光 {} 轮 {}", l_session_next.title_.empty() ? "会话" : l_session_next.title_, l_iteration);
  l_task.status_            = server_task_info_status::submitted;
  l_task.submitter_         = l_session_next.submitter_;
  l_task.submit_time_       = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_task.task_id_           = in_request.task_id_;
  l_task.type_              = server_task_info_type::ai_light;
  l_task.command_           = l_payload;
  l_task.uuid_id_           = core_set::get_set().get_uuid();
  l_it.server_task_info_id_ = l_task.uuid_id_;
  l_it.dispatch_at_         = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  l_sqls.emplace_back(orm::insert(in_sql).into<server_task_info>().values(l_task));
  l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_iteration>().values(l_it));
  l_sqls.emplace_back(
      orm::update(in_sql)
          .from<ai_light_session>()
          .set(
              c(&ai_light_session::state_)             = l_session_next.state_,
              c(&ai_light_session::current_iteration_) = l_session_next.current_iteration_,
              c(&ai_light_session::judgement_due_at_)  = l_session_next.judgement_due_at_,
              c(&ai_light_session::failure_reason_)    = l_session_next.failure_reason_
          )
          .where(c(&ai_light_session::uuid_id_) == l_session_next.uuid_id_)
  );

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮 {} 个候选已派发（任务 {}）", in_request.session_id_,
      l_iteration, in_request.rigs_.size(), l_task.uuid_id_
  );
  socket_io::broadcast(socket_io::server_task_info_new_broadcast_t{.server_task_info_id_ = l_task.uuid_id_});
  return l_sqls;
}

}  // namespace

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

  auto l_sqls = write_submitted_iteration(l_sql, l_req);
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 向 AI 灯光会话 {} 提交了第 {} 轮 {} 个候选（前端展开）",
      person_.person_.email_, person_.person_.get_full_name(), l_session.uuid_id_, l_req.iteration_, l_req.rigs_.size()
  );

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  nlohmann::json l_ret{};
  l_ret = *l_detail;
  co_return in_handle->make_msg(l_ret);
}

}  // namespace doodle::http::ai_light
