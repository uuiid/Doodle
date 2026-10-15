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
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/http_method/kitsu/ai_light_stats.h>
#include <doodle_lib/http_method/kitsu/preview.h>
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/beast/http/status.hpp>

#include <algorithm>
#include <magic_enum/magic_enum.hpp>
#include <opencv2/opencv.hpp>
#include <string>
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
  return g_ctx().get<kitsu_ctx_t>().get_ai_light_session_dir(in_session.uuid_id_);
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

  l_session.state_             = ai_light_session_state::draft;
  l_session.current_iteration_ = 0;
  l_session.judgement_due_at_  = std::nullopt;
  l_session.failure_reason_    = "";
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

  co_return in_handle->make_msg(nlohmann::json{{"data", l_sessions}});
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
  co_return in_handle->make_msg(nlohmann::json{{"data", l_sql.get_ai_light_references(l_session.uuid_id_)}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_session = load_session_checked(person_, l_sql, session_id_);

  auto l_files   = in_handle->get_files();
  DOODLE_CHICK_HTTP(!l_files.empty(), bad_request, "请上传至少一张参考图");

  auto& l_ctx    = g_ctx().get<kitsu_ctx_t>();
  auto l_ref_dir = l_ctx.get_ai_light_reference_dir(l_session.uuid_id_);
  if (!FSys::exists(l_ref_dir)) FSys::create_directories(l_ref_dir);

  std::vector<ai_light_reference> l_added{};
  for (auto&& l_file : l_files) {
    auto l_info = preview::convert_to_png(l_file);
    DOODLE_CHICK_HTTP(
        !l_info.path_.empty(), bad_request, "参考图 {} 无法解析为图片", l_file.filename().generic_string()
    );

    ai_light_reference l_ref{};
    l_ref.session_id_ = l_session.uuid_id_;
    l_ref.uuid_id_    = core_set::get_set().get_uuid();
    l_ref.file_path_  = l_ctx.get_ai_light_reference_file(l_session.uuid_id_, l_ref.uuid_id_);
    FSys::rename(l_info.path_, l_ref.file_path_);
    l_ref.width_    = l_info.size_.width;
    l_ref.height_   = l_info.size_.height;

    auto l_analysis = ai_light::analyze_reference_file(l_ref.file_path_);
    DOODLE_CHICK_HTTP(
        l_analysis.has_value(), bad_request, "参考图 {} 落盘后无法解码，无法统计",
        l_ref.file_path_.filename().generic_string()
    );
    l_ref.set_stats(l_analysis->stats_);
    l_ref.stats_confidence_ = l_analysis->confidence_;
    l_ref.is_outlier_       = false;
    l_added.push_back(l_ref);
  }

  sql_modify_statement_vector_t l_sqls{};
  for (auto& l_ref : l_added) l_sqls.emplace_back(insert(l_sql).into<ai_light_reference>().values(l_ref));

  if (l_session.state_ == ai_light_session_state::draft) {
    l_sqls.emplace_back(update(l_sql)
                            .from<ai_light_session>()
                            .set(c(&ai_light_session::state_) = ai_light_session_state::generating)
                            .where(c(&ai_light_session::uuid_id_) == l_session.uuid_id_));
  }
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 为 AI 灯光会话 {} 上传了 {} 张参考图", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, l_added.size()
  );
  co_return in_handle->make_msg(nlohmann::json{{"data", l_added}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();

  auto l_ref = l_sql.get_by_uuid<ai_light_reference>(id_);
  load_session_checked(person_, l_sql, l_ref.session_id_);

  co_return in_handle->make_msg(nlohmann::json{{"data", l_ref}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, put) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_ref  = l_sql.get_by_uuid<ai_light_reference>(id_);
  load_session_checked(person_, l_sql, l_ref.session_id_);

  if (l_json.contains("is_outlier")) l_json.at("is_outlier").get_to(l_ref.is_outlier_);
  if (l_json.contains("excluded_reason")) l_json.at("excluded_reason").get_to(l_ref.excluded_reason_);

  if (!l_ref.is_outlier_) l_ref.excluded_reason_.clear();

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<ai_light_reference>()
                          .set(
                              c(&ai_light_reference::is_outlier_)      = l_ref.is_outlier_,
                              c(&ai_light_reference::excluded_reason_) = l_ref.excluded_reason_
                          )
                          .where(c(&ai_light_reference::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "用户 {}({}) 把 AI 灯光参考图 {} 的排除标记改为 {}（原因：{}）",
      person_.person_.email_, person_.person_.get_full_name(), id_, l_ref.is_outlier_,
      l_ref.excluded_reason_.empty() ? "无" : l_ref.excluded_reason_
  );
  co_return in_handle->make_msg(nlohmann::json{{"data", l_ref}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();

  auto l_ref     = l_sql.get_by_uuid<ai_light_reference>(id_);
  auto l_session = load_session_checked(person_, l_sql, l_ref.session_id_);

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<ai_light_reference>().where(c(&ai_light_reference::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  if (!l_ref.file_path_.empty()) {
    std::error_code l_ec{};
    FSys::remove(l_ref.file_path_, l_ec);
    if (l_ec)
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_http(), "参考图 {} 的文件 {} 删除失败: {}", id_, l_ref.file_path_.generic_string(),
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

  l_session.mark_judged(ai_light_session_state::generating);
  l_session.failure_reason_ = "";
  l_sqls.emplace_back(update(l_sql)
                          .from<ai_light_session>()
                          .set(
                              c(&ai_light_session::state_)             = l_session.state_,
                              c(&ai_light_session::current_iteration_) = l_session.current_iteration_,
                              c(&ai_light_session::judgement_due_at_)  = l_session.judgement_due_at_,
                              c(&ai_light_session::failure_reason_)    = l_session.failure_reason_
                          )
                          .where(c(&ai_light_session::uuid_id_) == l_session.uuid_id_));

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
// 工作机回传：一轮候选的渲染结果（方案 §15.12）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(render_report_route, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_task = l_sql.get_by_uuid<server_task_info>(task_id_);
  DOODLE_CHICK_HTTP(
      l_task.type_ == server_task_info_type::ai_light, bad_request, "任务 {} 的类型是 {}，不是 AI 灯光任务", task_id_,
      magic_enum::enum_name(l_task.type_)
  );

  auto l_report = l_json.get<round_render_report_t>();
  co_await apply_render_report(l_sql, g_ctx().get<kitsu_ctx_t>(), task_id_, ai_light_iteration_locator_t{}, l_report);

  nlohmann::json l_out{{"task_id", task_id_}};
  if (auto l_it = l_sql.get_ai_light_iteration_by_task(task_id_); l_it.has_value()) {
    auto l_session           = l_sql.get_by_uuid<ai_light_session>(l_it->session_id_);
    l_out["session_id"]      = l_it->session_id_;
    l_out["iteration"]       = l_it->iteration_;
    l_out["iteration_state"] = l_it->state_;
    l_out["session_state"]   = l_session.state_;
    l_out["failure_reason"]  = l_session.failure_reason_;
  }
  co_return in_handle->make_msg(l_out);
}

// ===========================================================================
// 引擎内路线回传：一轮候选的渲染结果（D54）
// ===========================================================================
//
// 与上面那条工作机回传的差别只有两处，其余全部复用：
//   1) 定位方式：轮次在路径上（session_id + iteration），由 ai_light_iteration_locator_t
//      交给 apply_render_report。引擎内路线**没有 server_task_info 行**（D53：面板不派发），
//      所以拿不出任务 uuid，这也是 locator 存在的唯一理由。
//   2) 权限：调用方是本机 Doodle 客户端，用发起人自己的 JWT，所以做 check_project_access。
//      工作机那条相反（它不是项目成员，只认任务 uuid）；这里不放开。
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(render_report_local_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  auto l_session = load_session_checked(person_, l_sql, session_id_);

  DOODLE_CHICK_HTTP(
      l_sql.get_ai_light_iteration(l_session.uuid_id_, iteration_).has_value(), not_found, "会话 {} 没有第 {} 轮",
      l_session.uuid_id_, iteration_
  );

  auto l_report = l_json.get<round_render_report_t>();
  co_await apply_render_report(
      l_sql, g_ctx().get<kitsu_ctx_t>(), uuid{}, ai_light_iteration_locator_t{l_session.uuid_id_, iteration_}, l_report
  );

  nlohmann::json l_out{};
  l_out["session_id"] = l_session.uuid_id_;
  l_out["iteration"]  = iteration_;
  std::string l_it_state{};
  std::string l_session_state{};
  if (auto l_it = l_sql.get_ai_light_iteration(l_session.uuid_id_, iteration_); l_it.has_value()) {
    auto l_session_after     = l_sql.get_by_uuid<ai_light_session>(l_it->session_id_);
    l_out["iteration_state"] = l_it->state_;
    l_out["session_state"]   = l_session_after.state_;
    l_out["failure_reason"]  = l_session_after.failure_reason_;
    l_it_state               = std::string{magic_enum::enum_name(l_it->state_)};
    l_session_state          = std::string{magic_enum::enum_name(l_session_after.state_)};
  }

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光（引擎内路线）：会话 {} 第 {} 轮回传已受理，状态 {} / {}", l_session.uuid_id_,
      iteration_, l_it_state, l_session_state
  );
  co_return in_handle->make_msg(l_out);
}

// ===========================================================================
// 工作机上传候选产物（首帧海报 / 候选视频 / 线性 EXR）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(candidate_file_route, post) {
  person_.check_not_outsourcer();

  DOODLE_CHICK_HTTP(
      in_handle->content_type_ == detail::content_type::application_nuknown, bad_request,
      "候选产物的上传体必须是 application/octet-stream，收到的是 {}", magic_enum::enum_name(in_handle->content_type_)
  );

  auto l_sql  = get_sqlite_database();
  auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(id_);

  auto l_it   = l_sql.get_by_uuid<ai_light_iteration>(l_cand.iteration_id_);
  DOODLE_CHICK_HTTP(
      l_it.state_ != ai_light_iteration_state::judged, bad_request, "AI 灯光轮次 {} 已判定，不再接受候选产物上传",
      l_it.iteration_
  );

  auto& l_ctx         = g_ctx().get<kitsu_ctx_t>();
  const bool l_linear = role_ == doodle::ai_light::ai_light_candidate_file_role::render_linear;
  const bool l_movie  = role_ == doodle::ai_light::ai_light_candidate_file_role::render_movie;
  const FSys::path l_dst =
      l_movie ? candidate_movie_path(l_ctx, l_cand) : candidate_render_path(l_ctx, l_cand, l_linear);

  const FSys::path l_tmp = in_handle->get_file();
  DOODLE_CHICK_HTTP(FSys::exists(l_tmp), bad_request, "候选 {} 的上传体为空", id_);

  std::error_code l_ec{};
  if (l_dst.has_parent_path()) {
    FSys::create_directories(l_dst.parent_path(), l_ec);
    DOODLE_CHICK_HTTP(
        !l_ec, internal_server_error, "候选 {} 产物目录创建失败 {}：{}", id_, l_dst.parent_path().generic_string(),
        l_ec.message()
    );
  }
  const FSys::path l_tmp_dst = l_dst.string() + ".uploading";
  FSys::copy_file(l_tmp, l_tmp_dst, FSys::copy_options::overwrite_existing, l_ec);
  DOODLE_CHICK_HTTP(
      !l_ec, internal_server_error, "候选 {} 产物写入失败 {}：{}", id_, l_tmp_dst.generic_string(), l_ec.message()
  );
  FSys::rename(l_tmp_dst, l_dst, l_ec);
  DOODLE_CHICK_HTTP(
      !l_ec, internal_server_error, "候选 {} 产物落位失败 {}：{}", id_, l_dst.generic_string(), l_ec.message()
  );
  std::error_code l_ignored{};
  FSys::remove(l_tmp, l_ignored);

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光：候选 {}（轮次 {}）上传{}到位 {} {} 字节", id_, l_it.iteration_,
      l_movie ? "候选视频" : (l_linear ? "线性 EXR" : "首帧海报"), l_dst.generic_string(),
      FSys::file_size(l_dst, l_ignored)
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
  co_return in_handle->make_msg(nlohmann::json{{"data", l_sql.get_ai_light_presets(l_only)}});
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
  auto l_sql    = get_sqlite_database();
  auto l_preset = l_sql.get_ai_light_preset(id_);
  DOODLE_CHICK_HTTP(l_preset.has_value(), not_found, "预设 {} 不存在", id_);

  auto l_json = in_handle->get_json();
  if (l_json.contains("name")) l_json.at("name").get_to(l_preset->name_);
  if (l_json.contains("tags")) l_json.at("tags").get_to(l_preset->tags_);
  if (l_json.contains("embedding")) l_json.at("embedding").get_to(l_preset->embedding_);
  if (l_json.contains("embedding_dim")) l_json.at("embedding_dim").get_to(l_preset->embedding_dim_);
  if (l_json.contains("rig_fragment")) l_json.at("rig_fragment").get_to(l_preset->rig_fragment_);
  if (l_json.contains("applicable")) l_json.at("applicable").get_to(l_preset->applicable_);
  if (l_json.contains("stats_target")) l_json.at("stats_target").get_to(l_preset->stats_target_);
  if (l_json.contains("enabled")) l_json.at("enabled").get_to(l_preset->enabled_);

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<ai_light_preset>()
                          .set(
                              c(&ai_light_preset::name_)          = l_preset->name_,
                              c(&ai_light_preset::tags_)          = l_preset->tags_,
                              c(&ai_light_preset::embedding_)     = l_preset->embedding_,
                              c(&ai_light_preset::embedding_dim_) = l_preset->embedding_dim_,
                              c(&ai_light_preset::rig_fragment_)  = l_preset->rig_fragment_,
                              c(&ai_light_preset::applicable_)    = l_preset->applicable_,
                              c(&ai_light_preset::stats_target_)  = l_preset->stats_target_,
                              c(&ai_light_preset::enabled_)       = l_preset->enabled_
                          )
                          .where(c(&ai_light_preset::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));
  co_return in_handle->make_msg(nlohmann::json{} = *l_preset);
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

  co_return in_handle->make_msg(
      nlohmann::json{
          {"result", l_result.result_json_},
          {"input_tokens", l_result.input_tokens_},
          {"output_tokens", l_result.output_tokens_},
          {"total_tokens", l_result.total_tokens_},
      }
  );
}

}  // namespace doodle::http::ai_light
