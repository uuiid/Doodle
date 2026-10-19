#include <doodle_core/configure/static_value.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/seedance2/task.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/http_method/kitsu/ai_light_stats.h>
#include <doodle_lib/http_method/kitsu/auto_task.h>
#include <doodle_lib/http_method/kitsu/preview.h>
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>
#include <boost/beast/http/status.hpp>

#include <algorithm>
#include <cpp-base64/base64.h>
#include <cstdint>
#include <exception>
#include <magic_enum/magic_enum.hpp>
#include <opencv2/opencv.hpp>
#include <optional>
#include <regex>
#include <string>
#include <utility>
#include <vector>


namespace doodle::http::ai_light {

namespace {

using namespace orm;
using namespace doodle::ai_light;

struct ai_light_judgement_t {
  uuid selected_candidate_id_{};
  std::string feedback_text_{};

  friend void from_json(const nlohmann::json& j, ai_light_judgement_t& p) {
    if (j.contains("selected_candidate_id")) j.at("selected_candidate_id").get_to(p.selected_candidate_id_);
    if (j.contains("feedback_text")) j.at("feedback_text").get_to(p.feedback_text_);
  }
};

struct submit_iteration_request_t {
  uuid session_id_{};
  uuid project_id_{};
  uuid operator_id_{};
  std::int32_t iteration_{0};
  std::vector<lighting_rig_t> rigs_{};
  uuid task_id_{};

  friend void to_json(nlohmann::json& j, const submit_iteration_request_t& p) {
    j["session_id"] = p.session_id_;
    j["project_id"] = p.project_id_;
    j["iteration"]  = p.iteration_;
    j["candidates"] = p.rigs_;
    j["task_id"]    = p.task_id_;
  }
  friend void from_json(const nlohmann::json& j, submit_iteration_request_t& p) {
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("project_id")) j.at("project_id").get_to(p.project_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("candidates")) j.at("candidates").get_to(p.rigs_);
    if (j.contains("task_id")) j.at("task_id").get_to(p.task_id_);
  }
};

ai_light_session load_session_checked(
    const http_jwt_fun::http_jwt_t& in_person, sqlite_database& in_sql, const uuid& in_session_id
) {
  auto l_session = in_sql.get_by_uuid<ai_light_session>(in_session_id);
  in_person.check_project_access(l_session.project_id_);
  return l_session;
}

uuid resolve_lighting_task_id(sqlite_database& in_sql, const uuid& in_entity_id) {
  auto l_task = in_sql.get_tasks_for_entity_and_task_type(in_entity_id, task_type::get_lighting_id());
  if (!l_task.has_value())
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request,
            fmt::format("镜头 {} 没有灯光任务，无法建立 AI 灯光会话", in_entity_id)
        }
    );
  return l_task->uuid_id_;
}

bool query_bool(const boost::url& in_url, std::string_view in_key, bool in_default) {
  for (auto&& [l_key, l_value, l_has] : in_url.params()) {
    if (l_key == in_key && l_has) return l_value == "true" || l_value == "1";
  }
  return in_default;
}

FSys::path session_dir(const ai_light_session& in_session) {
  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  return l_ctx.root_ / "ai_light" / FSys::split_uuid_path(fmt::to_string(in_session.uuid_id_));
}

void backup_session_dir(const ai_light_session& in_session) {
  auto l_src = session_dir(in_session);
  if (!FSys::exists(l_src)) return;
  try {
    auto l_dst = g_ctx().get<kitsu_ctx_t>().get_ai_light_backup_dir(
        in_session.uuid_id_, fmt::format("{:%Y_%m_%d_%H}", chrono::floor<chrono::hours>(chrono::system_clock::now()))
    );
    if (!FSys::exists(l_dst.parent_path())) FSys::create_directories(l_dst.parent_path());
    FSys::rename(l_src, l_dst);
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 产物已备份到 {}", in_session.uuid_id_, l_dst.generic_string()
    );
  } catch (const std::exception& in_err) {
    SPDLOG_LOGGER_ERROR(
        g_logger_ctrl().get_main_error(), "AI 灯光会话 {} 产物备份失败（会话仍会被删除）: {}", in_session.uuid_id_,
        in_err.what()
    );
  }
}

ai_light_session_detail_t load_detail_checked(
    const http_jwt_fun::http_jwt_t& in_person, sqlite_database& in_sql, const uuid& in_session_id
) {
  auto l_detail = build_session_detail(in_sql, in_session_id);
  if (!l_detail.has_value())
    throw_exception(
        http_request_error{boost::beast::http::status::not_found, fmt::format("AI 灯光会话 {} 不存在", in_session_id)}
    );
  in_person.check_project_access(l_detail->session_.project_id_);
  return std::move(*l_detail);
}

orm::sql_modify_statement_vector_t write_submitted_iteration(
    sqlite_database& in_sql, const submit_iteration_request_t& in_request
) {
  const auto l_iteration = in_request.iteration_;

  DOODLE_CHICK_HTTP(!in_request.rigs_.empty(), internal_server_error, "本轮候选数为 0，方案 §6.6 要求至少一个候选");

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
  l_it.uuid_id_             = core_set::get_set().get_uuid();
  l_it.session_id_          = in_request.session_id_;
  l_it.iteration_           = l_iteration;
  l_it.state_               = ai_light_iteration_state::generated;
  l_it.server_task_info_id_ = uuid{};

  lighting_rig_set_t l_rig_set{};
  l_rig_set.session_id_     = in_request.session_id_;
  l_rig_set.iteration_      = l_iteration;
  l_rig_set.shot_task_id_   = in_request.task_id_;
  l_rig_set.baseline_level_ = l_assembly_res.original_map_.generic_string();

  std::vector<ai_light_candidate> l_candidates{};
  l_candidates.reserve(in_request.rigs_.size());

  for (std::size_t l_i = 0; l_i < in_request.rigs_.size(); ++l_i) {
    ai_light_candidate l_cand{};
    l_cand.uuid_id_         = core_set::get_set().get_uuid();
    l_cand.iteration_id_    = l_it.uuid_id_;
    l_cand.session_id_      = in_request.session_id_;
    l_cand.render_status_   = candidate_render_status::pending;
    l_cand.is_selected_     = false;
    l_cand.preview_file_id_ = uuid{};

    auto l_rig              = in_request.rigs_.at(l_i);
    l_rig.candidate_id_     = l_cand.uuid_id_;
    l_rig_set.rigs_.emplace_back(std::move(l_rig));

    l_candidates.emplace_back(std::move(l_cand));
  }
  l_it.set_rigs(l_rig_set);

  orm::sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_iteration>().values(l_it));
  for (auto& l_cand : l_candidates) l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_candidate>().values(l_cand));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(),
      "AI 灯光会话 {} 第 {} 轮 {} 个候选的方案与候选行已落库（引擎内路线，等待发起方本机渲染）", in_request.session_id_,
      l_iteration, in_request.rigs_.size()
  );
  return l_sqls;
}

orm::sql_modify_statement_vector_t write_dispatched_iteration(
    sqlite_database& in_sql, const submit_iteration_request_t& in_request
) {
  const auto l_iteration = in_request.iteration_;

  DOODLE_CHICK_HTTP(!in_request.rigs_.empty(), internal_server_error, "本轮候选数为 0，方案 §6.6 要求至少一个候选");

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
  l_it.uuid_id_        = core_set::get_set().get_uuid();
  l_it.session_id_     = in_request.session_id_;
  l_it.iteration_      = l_iteration;
  l_it.state_          = ai_light_iteration_state::generated;

  const auto l_out_dir = make_iteration_relative_dir(in_request.session_id_, l_it.uuid_id_);

  orm::sql_modify_statement_vector_t l_sqls{};
  std::vector<ai_light_task_candidate_t> l_payload_candidates{};
  l_payload_candidates.reserve(in_request.rigs_.size());

  lighting_rig_set_t l_rig_set{};
  l_rig_set.session_id_     = in_request.session_id_;
  l_rig_set.iteration_      = l_iteration;
  l_rig_set.shot_task_id_   = in_request.task_id_;
  l_rig_set.baseline_level_ = l_assembly_res.original_map_.generic_string();

  for (std::size_t l_i = 0; l_i < in_request.rigs_.size(); ++l_i) {
    auto l_rig = in_request.rigs_.at(l_i);

    ai_light_candidate l_cand{};
    l_cand.uuid_id_ = core_set::get_set().get_uuid();

    l_rig_set.rigs_.emplace_back(l_rig);

    l_payload_candidates.push_back(
        ai_light_task_candidate_t{
            l_cand.uuid_id_, make_candidate_frames_dir(l_cand.uuid_id_),
            make_candidate_file_name(l_cand.uuid_id_, false), make_candidate_file_name(l_cand.uuid_id_, true),
            make_candidate_movie_file(l_cand.uuid_id_), l_rig
        }
    );
  }
  l_it.set_rigs(l_rig_set);

  auto l_session_next = in_sql.get_by_uuid<ai_light_session>(in_request.session_id_);

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

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮 {} 个候选已派发（任务 {}）", in_request.session_id_,
      l_iteration, in_request.rigs_.size(), l_task.uuid_id_
  );
  socket_io::broadcast(socket_io::server_task_info_new_broadcast_t{.server_task_info_id_ = l_task.uuid_id_});
  return l_sqls;
}

}  // namespace

// ===========================================================================
// 会话
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(shot_session_route, post) {
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_json   = in_handle->get_json();

  auto l_entity = l_sql.get_by_uuid<entity>(entity_id_);

  ai_light_session l_session{};
  l_json.get_to(l_session);
  l_session.submitter_  = person_.person_.uuid_id_;
  l_session.entity_id_  = entity_id_;
  l_session.project_id_ = l_entity.project_id_;
  l_session.task_id_    = resolve_lighting_task_id(l_sql, entity_id_);
  person_.check_project_access(l_session.project_id_);

  DOODLE_CHICK_HTTP(
      !l_session.prompt_text_.empty(), bad_request, "缺少 prompt_text —— 提示词是权威意图，不允许空着建会话"
  );

  if (l_session.title_.empty()) l_session.title_ = l_entity.name_;

  {
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(insert(l_sql).into<ai_light_session>().values(l_session));
    co_await l_sql.run_sql(std::move(l_sqls));
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 为镜头 {} 创建 AI 灯光会话 {}（{} 字提示词）", person_.person_.email_,
      person_.person_.get_full_name(), l_session.entity_id_, l_session.uuid_id_, l_session.prompt_text_.size()
  );

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_detail);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(shot_session_route, get) {
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();
  auto l_sessions = l_sql.get_ai_light_sessions_by_entity(entity_id_);

  for (const auto& l_s : l_sessions) person_.check_project_access(l_s.project_id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_sessions);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_instance_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  co_return in_handle->make_msg(nlohmann::json{} = load_detail_checked(person_, l_sql, id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_instance_route, put) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  auto l_session = load_session_checked(person_, l_sql, id_);

  if (l_json.contains("prompt_text")) l_json.at("prompt_text").get_to(l_session.prompt_text_);

  DOODLE_CHICK_HTTP(!l_session.prompt_text_.empty(), bad_request, "提示词不能为空");

  {
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(update(l_sql)
                            .from<ai_light_session>()
                            .set(c(&ai_light_session::prompt_text_) = l_session.prompt_text_)
                            .where(c(&ai_light_session::uuid_id_) == id_));
    co_await l_sql.run_sql(std::move(l_sqls));
  }

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "用户 {}({}) 修改了 AI 灯光会话 {} 的提示词（{} 字符）", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_session.prompt_text_.size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = load_detail_checked(person_, l_sql, id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_session = load_session_checked(person_, l_sql, id_);

  backup_session_dir(l_session);

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<ai_light_session>().where(c(&ai_light_session::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 AI 灯光会话 {}", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );
  co_return in_handle->make_msg_204();
}

// ===========================================================================
// 参考图
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_route, get) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_session = load_session_checked(person_, l_sql, session_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_ai_light_references(l_session.uuid_id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_session = load_session_checked(person_, l_sql, session_id_);

  auto l_files   = in_handle->get_files();
  DOODLE_CHICK_HTTP(!l_files.empty(), bad_request, "请上传至少一张参考图");

  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();

  std::vector<ai_light_reference> l_added{};
  for (auto&& l_file : l_files) {
    auto l_info = preview::convert_to_png(l_file);
    DOODLE_CHICK_HTTP(
        !l_info.path_.empty(), bad_request, "参考图 {} 无法解析为图片", l_file.filename().generic_string()
    );

    ai_light_reference l_ref{};
    l_ref.session_id_ = l_session.uuid_id_;
    l_ref.uuid_id_    = core_set::get_set().get_uuid();
    auto l_file_path  = l_ctx.get_sd2_pictures_file(l_ref.uuid_id_, "png");
    auto l_thumbnail  = l_ctx.get_sd2_thumbnail_file(l_ref.uuid_id_);

    if (auto l_p = l_file_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    if (auto l_p = l_thumbnail.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

    {
      auto l_image = cv::imread(l_info.path_.generic_string());
      DOODLE_CHICK_HTTP(!l_image.empty(), bad_request, "参考图 {} 无法解码", l_file.filename().generic_string());
      auto l_resize = std::min(500.0 / l_image.cols, 500.0 / l_image.rows);
      cv::resize(l_image, l_image, cv::Size(l_image.cols * l_resize, l_image.rows * l_resize));
      cv::imwrite(l_thumbnail.generic_string(), l_image);
    }
    FSys::rename(l_info.path_, l_file_path);

    auto l_analysis = ai_light::analyze_reference_file(l_file_path);
    DOODLE_CHICK_HTTP(
        l_analysis.has_value(), bad_request, "参考图 {} 落盘后无法解码，无法统计",
        l_file_path.filename().generic_string()
    );
    l_ref.set_stats(l_analysis->stats_);
    l_ref.stats_confidence_ = l_analysis->confidence_;
    l_ref.is_outlier_       = false;
    l_added.push_back(l_ref);
  }

  sql_modify_statement_vector_t l_sqls{};
  for (auto& l_ref : l_added) l_sqls.emplace_back(insert(l_sql).into<ai_light_reference>().values(l_ref));

  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 为 AI 灯光会话 {} 上传了 {} 张参考图", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, l_added.size()
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_added);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();

  auto l_ref = l_sql.get_by_uuid<ai_light_reference>(id_);
  load_session_checked(person_, l_sql, l_ref.session_id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_ref);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, put) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_ref  = l_sql.get_by_uuid<ai_light_reference>(id_);
  load_session_checked(person_, l_sql, l_ref.session_id_);

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<ai_light_reference>()
                          .set_from_ref(l_json, ai_light_reference::put_property_list())
                          .where(c(&ai_light_reference::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  auto l_after = l_sql.get_by_uuid<ai_light_reference>(id_);
  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "用户 {}({}) 把 AI 灯光参考图 {} 的排除标记改为 {}（原因：{}）",
      person_.person_.email_, person_.person_.get_full_name(), id_, l_after.is_outlier_,
      l_after.excluded_reason_.empty() ? "无" : l_after.excluded_reason_
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_after);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();

  auto l_ref     = l_sql.get_by_uuid<ai_light_reference>(id_);
  auto l_session = load_session_checked(person_, l_sql, l_ref.session_id_);

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<ai_light_reference>().where(c(&ai_light_reference::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  if (auto l_file_path = g_ctx().get<kitsu_ctx_t>().get_sd2_pictures_file(id_, "png"); !l_file_path.empty()) {
    std::error_code l_ec{};
    FSys::remove(l_file_path, l_ec);
    if (l_ec)
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_http(), "参考图 {} 的文件 {} 删除失败: {}", id_, l_file_path.generic_string(),
          l_ec.message()
      );
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 AI 灯光参考图 {}", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );
  co_return in_handle->make_msg_204();
}

// ===========================================================================
// 候选渲染图（中间候选专用接口；只有定稿才进 preview_file）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(candidate_movie_route, get) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();

  auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(id_);
  person_.check_project_access(l_sql.get_by_uuid<ai_light_session>(l_cand.session_id_).project_id_);

  DOODLE_CHICK_HTTP(
      l_cand.render_status_ != candidate_render_status::pending, not_found, "候选 {} 尚未渲染完成", l_cand.uuid_id_
  );
  DOODLE_CHICK_HTTP(
      l_cand.render_status_ != candidate_render_status::failed, not_found, "候选 {} 渲染失败", l_cand.uuid_id_
  );
  const FSys::path l_movie_path = candidate_movie_path(g_ctx().get<kitsu_ctx_t>(), l_cand);
  DOODLE_CHICK(FSys::exists(l_movie_path), "候选视频不存在 文件 {}", l_movie_path.generic_string());

  co_return in_handle->make_msg(l_movie_path, kitsu::mime_type(l_movie_path.extension()));
}

// ===========================================================================
// 人工闸门 2：判断
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(judge_route, post) {
  person_.check_not_outsourcer();
  auto l_sql       = get_sqlite_database();
  auto l_json      = in_handle->get_json();

  auto l_session   = load_session_checked(person_, l_sql, session_id_);

  auto l_iteration = l_sql.get_by_uuid<ai_light_iteration>(iteration_id_);
  DOODLE_CHICK_HTTP(
      l_iteration.session_id_ == l_session.uuid_id_, bad_request, "轮次 {} 不属于会话 {}", iteration_id_,
      l_session.uuid_id_
  );

  ai_light_judgement_t l_judgement{};
  l_json.get_to(l_judgement);

  DOODLE_CHICK_HTTP(
      l_iteration.state_ != ai_light_iteration_state::judged, bad_request, "轮次 {} 已经判断过了",
      l_iteration.iteration_
  );

  sql_modify_statement_vector_t l_sqls{};

  if (!l_judgement.selected_candidate_id_.is_nil()) {
    auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(l_judgement.selected_candidate_id_);
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == l_iteration.uuid_id_, bad_request, "候选 {} 不属于第 {} 轮", l_cand.uuid_id_,
        l_iteration.iteration_
    );

    l_cand.is_selected_ = true;

    l_sqls.emplace_back(update(l_sql)
                            .from<ai_light_candidate>()
                            .set(c(&ai_light_candidate::is_selected_) = l_cand.is_selected_)
                            .where(c(&ai_light_candidate::uuid_id_) == l_cand.uuid_id_));
  }

  l_iteration.state_         = ai_light_iteration_state::judged;
  l_iteration.judged_by_     = person_.person_.uuid_id_;
  l_iteration.judged_at_     = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
  l_iteration.feedback_text_ = l_judgement.feedback_text_;
  l_sqls.emplace_back(update(l_sql)
                          .from<ai_light_iteration>()
                          .set(
                              c(&ai_light_iteration::state_)         = l_iteration.state_,
                              c(&ai_light_iteration::judged_by_)     = l_iteration.judged_by_,
                              c(&ai_light_iteration::judged_at_)     = l_iteration.judged_at_,
                              c(&ai_light_iteration::feedback_text_) = l_iteration.feedback_text_
                          )
                          .where(c(&ai_light_iteration::uuid_id_) == l_iteration.uuid_id_));

  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 判断 AI 灯光会话 {} 第 {} 轮: {}", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, l_iteration.iteration_,
      l_judgement.selected_candidate_id_.is_nil() ? "全否" : "选定候选"
  );

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_detail);
}

// ===========================================================================
// 工作机上传候选的渲染帧序列（begin → 逐帧 → finish）
// ===========================================================================

namespace {

struct candidate_upload_target_t {
  ai_light_candidate candidate_;
  ai_light_iteration iteration_;
  FSys::path frames_dir_;
};

candidate_upload_target_t locate_upload_target(
    sqlite_database& in_sql, const http::kitsu_ctx_t& in_ctx, const uuid& in_candidate_id
) {
  candidate_upload_target_t l_ret{};
  l_ret.candidate_  = in_sql.get_by_uuid<ai_light_candidate>(in_candidate_id);
  l_ret.iteration_  = in_sql.get_by_uuid<ai_light_iteration>(l_ret.candidate_.iteration_id_);
  l_ret.frames_dir_ = in_ctx.get_ai_light_candidate_frames_dir(
      l_ret.candidate_.session_id_, l_ret.candidate_.iteration_id_, l_ret.candidate_.uuid_id_
  );
  return l_ret;
}

std::int32_t count_sequence_frames(const FSys::path& in_frames_dir) {
  std::error_code l_ec{};
  if (!FSys::is_directory(in_frames_dir, l_ec)) return 0;

  std::int32_t l_count{};
  for (const auto& l_entry : FSys::directory_iterator{in_frames_dir, l_ec}) {
    if (l_ec) return l_count;
    if (!l_entry.is_regular_file(l_ec)) continue;
    if (l_entry.path().extension() != ".png") continue;
    ++l_count;
  }
  return l_count;
}

FSys::path frame_file_name_from_header(const session_data_ptr& in_handle) {
  const auto& l_header = in_handle->req_header_;
  DOODLE_CHICK_HTTP(
      l_header.count(boost::beast::http::field::content_disposition) != 0, bad_request, "缺失必要的请求头信息"
  );

  std::string l_raw{l_header[boost::beast::http::field::content_disposition]};
  static const std::regex g_regex_base64{R"(^([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)?$)"};
  std::string l_name = std::regex_match(l_raw, g_regex_base64) ? base64_decode(std::string_view{l_raw}) : l_raw;

  DOODLE_CHICK_HTTP(!l_name.empty(), bad_request, "缺少帧文件名");
  const FSys::path l_path{l_name};
  DOODLE_CHICK_HTTP(
      l_path.filename() == l_path && l_name != "." && l_name != "..", bad_request, "帧文件名非法：{}", l_name
  );
  return l_path;
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(candidate_file_route, post) {
  person_.check_not_outsourcer();

  auto l_sql    = get_sqlite_database();
  auto& l_ctx   = g_ctx().get<kitsu_ctx_t>();
  auto l_target = locate_upload_target(l_sql, l_ctx, candidate_id_);

  if (stage_ == doodle::ai_light::ai_light_candidate_upload_stage::begin_sequence) {
    std::error_code l_ec{};
    FSys::remove_all(l_target.frames_dir_, l_ec);
    DOODLE_CHICK_HTTP(
        !l_ec, internal_server_error, "候选 {} 旧帧目录清理失败 {}：{}", candidate_id_,
        l_target.frames_dir_.generic_string(), l_ec.message()
    );
    FSys::create_directories(l_target.frames_dir_, l_ec);
    DOODLE_CHICK_HTTP(
        !l_ec, internal_server_error, "候选 {} 帧目录创建失败 {}：{}", candidate_id_,
        l_target.frames_dir_.generic_string(), l_ec.message()
    );

    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(update(l_sql)
                            .from<ai_light_candidate>()
                            .set(
                                c(&ai_light_candidate::render_status_) = ai_light::candidate_render_status::pending,
                                c(&ai_light_candidate::metrics_)       = nlohmann::json{}
                            )
                            .where(c(&ai_light_candidate::uuid_id_) == candidate_id_));
    co_await l_sql.run_sql(std::move(l_sqls));

    SPDLOG_LOGGER_INFO(
        g_logger_ctrl().get_http(), "AI 灯光：候选 {}（轮次 {}）开始上传帧序列，帧目录 {}", candidate_id_,
        l_target.iteration_.iteration_, l_target.frames_dir_.generic_string()
    );
    co_return in_handle->make_msg_204();
  }

  if (stage_ == doodle::ai_light::ai_light_candidate_upload_stage::upload_frame) {
    DOODLE_CHICK_HTTP(
        in_handle->content_type_ == detail::content_type::application_nuknown, bad_request,
        "候选帧的上传体必须是 application/octet-stream，收到的是 {}", magic_enum::enum_name(in_handle->content_type_)
    );

    const FSys::path l_dst = l_target.frames_dir_ / frame_file_name_from_header(in_handle);
    const FSys::path l_tmp = in_handle->get_file();
    DOODLE_CHICK_HTTP(FSys::exists(l_tmp), bad_request, "候选 {} 的上传体为空", candidate_id_);

    std::error_code l_ec{};
    FSys::create_directories(l_dst.parent_path(), l_ec);
    DOODLE_CHICK_HTTP(
        !l_ec, internal_server_error, "候选 {} 帧目录创建失败 {}：{}", candidate_id_,
        l_dst.parent_path().generic_string(), l_ec.message()
    );

    const FSys::path l_tmp_dst = l_dst.string() + ".uploading";
    FSys::copy_file(l_tmp, l_tmp_dst, FSys::copy_options::overwrite_existing, l_ec);
    DOODLE_CHICK_HTTP(
        !l_ec, internal_server_error, "候选 {} 帧写入失败 {}：{}", candidate_id_, l_tmp_dst.generic_string(),
        l_ec.message()
    );
    FSys::rename(l_tmp_dst, l_dst, l_ec);
    DOODLE_CHICK_HTTP(
        !l_ec, internal_server_error, "候选 {} 帧落位失败 {}：{}", candidate_id_, l_dst.generic_string(), l_ec.message()
    );
    std::error_code l_ignored{};
    FSys::remove(l_tmp, l_ignored);

    SPDLOG_LOGGER_INFO(
        g_logger_ctrl().get_http(), "AI 灯光：候选 {}（轮次 {}）上传帧 {}，{} 字节", candidate_id_,
        l_target.iteration_.iteration_, l_dst.filename().generic_string(), FSys::file_size(l_dst, l_ignored)
    );
    co_return in_handle->make_msg_204();
  }

  const std::int32_t l_frame_count = count_sequence_frames(l_target.frames_dir_);
  DOODLE_CHICK_HTTP(l_frame_count > 0, bad_request, "候选 {} 没有收到任何帧", candidate_id_);
  DOODLE_CHICK_HTTP(
      l_frame_count >= 2, bad_request, "候选 {} 只收到 {} 帧，合成不出视频", candidate_id_, l_frame_count
  );

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<ai_light_candidate>()
                          .set(c(&ai_light_candidate::render_status_) = ai_light::candidate_render_status::rendered)
                          .where(c(&ai_light_candidate::uuid_id_) == candidate_id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光：候选 {}（轮次 {}）帧序列上传完成，共 {} 帧", candidate_id_,
      l_target.iteration_.iteration_, l_frame_count
  );
  co_return in_handle->make_msg_204();
}

// ===========================================================================
// 预设库
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_route, get) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_only = query_bool(in_handle->url_, "enabled_only", true);
  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_ai_light_presets(l_only));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_route, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  ai_light_preset l_preset{};
  l_json.get_to(l_preset);
  DOODLE_CHICK_HTTP(!l_preset.name_.empty(), bad_request, "缺少预设名称");
  l_preset.created_by_ = person_.person_.uuid_id_;

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<ai_light_preset>().values(l_preset));
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 新建 AI 灯光预设 {}（{}）", person_.person_.email_,
      person_.person_.get_full_name(), l_preset.uuid_id_, l_preset.name_
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_preset);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_instance_route, get) {
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_preset = l_sql.get_ai_light_preset(id_);
  DOODLE_CHICK_HTTP(l_preset.has_value(), not_found, "预设 {} 不存在", id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_preset);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_instance_route, put) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  DOODLE_CHICK_HTTP(l_sql.get_ai_light_preset(id_).has_value(), not_found, "预设 {} 不存在", id_);

  auto l_json = in_handle->get_json();

  sql_modify_statement_vector_t l_sqls{};
  auto l_update = update(l_sql).from<ai_light_preset>().set_from_ref<ai_light_preset>(l_json).where(
      c(&ai_light_preset::uuid_id_) == id_
  );
  l_sqls.emplace_back(std::move(l_update));
  co_await l_sql.run_sql(std::move(l_sqls));

  co_return in_handle->make_msg(nlohmann::json{} = *l_sql.get_ai_light_preset(id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_preset = l_sql.get_ai_light_preset(id_);
  DOODLE_CHICK_HTTP(l_preset.has_value(), not_found, "预设 {} 不存在", id_);

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<ai_light_preset>().where(c(&ai_light_preset::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));
  co_return in_handle->make_msg_204();
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(inference_route, post) {
  person_.check_not_outsourcer();

  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  auto l_session = l_sql.get_by_uuid<ai_light_session>(session_id_);
  person_.check_project_access(l_session.project_id_);

  const uuid& l_operator_id = person_.person_.uuid_id_;

  DOODLE_CHICK_HTTP(
      !l_operator_id.is_nil(), bad_request, "推理缺少触发人（operator_id），无法解析 AI 工作室与计费主体"
  );
  const auto l_person = select(l_sql)
                            .columns(object<person>())
                            .from<person>()
                            .where(c(&person::uuid_id_) == l_operator_id)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_person.has_value(), bad_request, "推理找不到触发人 {}（person 不存在），无法解析 AI 工作室", l_operator_id
  );
  DOODLE_CHICK_HTTP(
      !l_person->ai_studio_id_.is_nil(), bad_request, "推理的触发人 {} 未关联 AI 工作室，无法调用推理接口",
      l_operator_id
  );
  const auto l_studio = select(l_sql)
                            .columns(object<ai_studio>())
                            .from<ai_studio>()
                            .where(c(&ai_studio::uuid_id_) == l_person->ai_studio_id_)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_studio.has_value(), bad_request, "推理找不到 AI 工作室 {}（配置可能已被删除）", l_person->ai_studio_id_
  );

  doodle::seedance2::task l_task{};
  l_task.type_                = doodle::seedance2::task_type::inference;
  l_task.backend_             = doodle::seedance2::task_backend::seedance2;
  l_task.user_id_             = l_operator_id;
  l_task.ai_studio_id_        = l_studio->uuid_id_;
  l_task.project_uuid_id_     = l_session.project_id_;
  l_task.ai_light_session_id_ = l_session.uuid_id_;
  l_task.completion_tokens_   = doodle_config::g_ark_inference_default_consumed_tokens;
  l_task.data_request_        = l_json;

  auto l_result =
      co_await http::seedance2::run_inference_task(l_task, l_sql, http::seedance2::make_inference_client(*l_studio));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光推理网关：会话 {} 完成一次推理（{} → {} token）", l_session.uuid_id_,
      l_result.input_tokens_, l_result.output_tokens_
  );

  co_return in_handle->make_msg(l_task.data_request_);
}

// ===========================================================================
// 会话提交一轮候选
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(iteration_submit_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_body    = in_handle->get_json();

  auto l_session = l_sql.get_by_uuid<ai_light_session>(session_id_);
  person_.check_project_access(l_session.project_id_);

  // 上一轮还没被判断时不许提交 —— 否则那批候选会变成孤儿（没有人再能判断它们）。
  // 判据落在**最新一轮**上：没有轮次即放行（等价于原先的 draft）。
  const auto l_prev = l_sql.get_ai_light_iterations(l_session.uuid_id_);
  DOODLE_CHICK_HTTP(
      l_prev.empty() || !is_waiting_human(l_prev.back()), bad_request, "会话 {} 第 {} 轮尚未判断，不能提交新候选",
      l_session.uuid_id_, l_prev.empty() ? 0 : l_prev.back().iteration_
  );

  submit_iteration_request_t l_req{};
  l_body.get_to(l_req);

  DOODLE_CHICK_HTTP(!l_req.rigs_.empty(), bad_request, "请求体缺少 candidates 数组（或为空）");

  DOODLE_CHICK_HTTP(l_req.iteration_ > 0, bad_request, "iteration 必须是正整数，收到 {}", l_req.iteration_);
  const std::int32_t l_expected = l_prev.empty() ? 1 : l_prev.back().iteration_ + 1;
  DOODLE_CHICK_HTTP(
      l_req.iteration_ == l_expected, bad_request, "提交的轮次号 {} 与会话期望的不一致（当前 {}，期望 {}）",
      l_req.iteration_, l_prev.empty() ? 0 : l_prev.back().iteration_, l_expected
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
