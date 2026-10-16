#include "ai_light_orchestrator.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_stats.h>
#include <doodle_lib/http_method/kitsu/computers.h>
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/exception/diagnostic_information.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace doodle::ai_light {

using namespace orm;

namespace {

using namespace std::chrono_literals;

bool references_have_stats(const std::vector<ai_light_reference>& in_references) {
  return std::any_of(in_references.begin(), in_references.end(), [](const ai_light_reference& in_ref) {
    return !in_ref.stats_.is_null() && in_ref.stats_confidence_ > 0.0;
  });
}

orm::sql_modify_statement_t make_session_update(sqlite_database& in_sql, const ai_light_session& in_session) {
  using namespace orm;
  return update(in_sql)
      .from<ai_light_session>()
      .set(
          c(&ai_light_session::state_)             = in_session.state_,
          c(&ai_light_session::current_iteration_) = in_session.current_iteration_,
          c(&ai_light_session::judgement_due_at_)  = in_session.judgement_due_at_
      )
      .where(c(&ai_light_session::uuid_id_) == in_session.uuid_id_);
}

}  // namespace

namespace {

std::optional<ai_light_iteration> locate_iteration(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator
) {
  if (!in_server_task_info_id.is_nil()) return in_sql.get_ai_light_iteration_by_task(in_server_task_info_id);
  DOODLE_CHICK_HTTP(
      !in_locator.session_id_.is_nil(), bad_request, "回传没有任务号，也没有会话号，无法定位 AI 灯光轮次"
  );
  return in_sql.get_ai_light_iteration(in_locator.session_id_, in_locator.iteration_);
}

}  // namespace

boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator,
    const std::vector<ai_light_candidate>& in_candidates
) {
  auto l_it = locate_iteration(in_sql, in_server_task_info_id, in_locator);
  DOODLE_CHICK_HTTP(l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id);
  DOODLE_CHICK_HTTP(
      l_it->state_ != ai_light_iteration_state::judged, bad_request, "轮次 {} 已经判断过了，不接受渲染回填",
      l_it->iteration_
  );

  auto l_session = in_sql.get_by_uuid<ai_light_session>(l_it->session_id_);

  orm::sql_modify_statement_vector_t l_sqls{};
  for (const auto& l_cand : in_candidates) {
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == l_it->uuid_id_, bad_request, "候选 {} 不属于轮次 {}", l_cand.uuid_id_, l_it->iteration_
    );
    l_sqls.emplace_back(
        orm::update(in_sql)
            .from<ai_light_candidate>()
            .set(
                c(&ai_light_candidate::render_status_) = l_cand.render_status_,
                c(&ai_light_candidate::metrics_)       = l_cand.metrics_
            )
            .where(c(&ai_light_candidate::uuid_id_) == l_cand.uuid_id_)
    );
  }

  l_it->state_     = ai_light_iteration_state::awaiting_judgement;
  l_it->render_at_ = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_sqls.emplace_back(
      orm::update(in_sql)
          .from<ai_light_iteration>()
          .set(
              c(&ai_light_iteration::state_) = l_it->state_, c(&ai_light_iteration::render_at_) = l_it->render_at_
          )
          .where(c(&ai_light_iteration::uuid_id_) == l_it->uuid_id_)
  );

  l_session.mark_awaiting_judgement();
  l_sqls.emplace_back(make_session_update(in_sql, l_session));

  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮渲染完成，{} 个候选待判断", l_session.uuid_id_,
      l_it->iteration_, in_candidates.size()
  );
  co_return;
}

namespace {

}  // namespace

boost::asio::awaitable<void> apply_render_report(
    sqlite_database& in_sql, const http::kitsu_ctx_t& in_ctx, const uuid& in_server_task_info_id,
    const ai_light_iteration_locator_t& in_locator, const round_render_report_t& in_report
) {
  auto l_it = locate_iteration(in_sql, in_server_task_info_id, in_locator);
  DOODLE_CHICK_HTTP(l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id);

  if (!in_report.error_.empty() ||
      !std::any_of(
          in_report.candidates_.begin(), in_report.candidates_.end(),
          [](const candidate_render_report_t& in_cand) { return in_cand.status_ == candidate_render_status::rendered; }
      )) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮渲染失败：{}", l_it->session_id_, l_it->iteration_,
        !in_report.error_.empty()
            ? in_report.error_
            : fmt::format("轮次 {} 的 {} 个候选全部渲染失败", l_it->iteration_, in_report.candidates_.size())
    );
    co_return;
  }

  auto l_round = in_sql.get_ai_light_candidates(l_it->uuid_id_);

  std::vector<ai_light_candidate> l_candidates{};
  l_candidates.reserve(in_report.candidates_.size());
  std::size_t l_missing_artifact = 0;

  for (const auto& l_rep : in_report.candidates_) {
    const auto l_found = std::find_if(l_round.begin(), l_round.end(), [&](const ai_light_candidate& in_cand) {
      return in_cand.uuid_id_ == l_rep.candidate_id_;
    });
    DOODLE_CHICK_HTTP(
        l_found != l_round.end(), bad_request, "候选 {} 不属于 AI 灯光轮次 {}", l_rep.candidate_id_, l_it->iteration_
    );

    ai_light_candidate l_cand = *l_found;
    l_cand.render_status_     = l_rep.status_;
    l_cand.metrics_           = l_rep.metrics_;

    if (l_cand.render_status_ == candidate_render_status::rendered) {
      const FSys::path l_png    = candidate_render_path(in_ctx, l_cand, false);
      const FSys::path l_movie  = candidate_movie_path(in_ctx, l_cand);
      const bool l_png_exists   = FSys::exists(l_png);
      const bool l_movie_exists = FSys::exists(l_movie);
      if (!l_png_exists || !l_movie_exists) {
        ++l_missing_artifact;
        l_cand.render_status_ = candidate_render_status::failed;
        SPDLOG_LOGGER_WARN(
            g_logger_ctrl().get_http(),
            "AI 灯光回传：候选 {}（轮次 {}）声称渲染成功，但产物取不到（海报 {}：{}；视频 {}：{}），已降级为失败",
            l_cand.uuid_id_, l_it->iteration_, l_png.generic_string(), l_png_exists ? "在" : "不在",
            l_movie.generic_string(), l_movie_exists ? "在" : "不在"
        );
      }
    }

    const FSys::path l_exr = candidate_render_path(in_ctx, l_cand, true);
    if (!FSys::exists(l_exr)) {
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_http(), "AI 灯光回传：候选 {}（轮次 {}）的线性 EXR 不在 {}（可选产物，不影响本轮判断）",
          l_cand.uuid_id_, l_it->iteration_, l_exr.generic_string()
      );
    }

    l_candidates.emplace_back(std::move(l_cand));
  }

  const bool l_any_rendered_after =
      std::any_of(l_candidates.begin(), l_candidates.end(), [](const ai_light_candidate& in_cand) {
        return in_cand.render_status_ == candidate_render_status::rendered;
      });
  if (!l_any_rendered_after) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮渲染失败：{}", l_it->session_id_, l_it->iteration_,
        fmt::format(
            "轮次 {} 的 {} 个候选渲染成功但产物文件缺失（海报或视频），本轮无可判断结果", l_it->iteration_,
            l_missing_artifact
        )
    );
    co_return;
  }

  co_await mark_iteration_rendered(in_sql, in_server_task_info_id, in_locator, l_candidates);
  co_return;
}

std::optional<ai_light_session_detail_t> build_session_detail(sqlite_database& in_sql, const uuid& in_session_id) {
  std::optional<ai_light_session_detail_t> l_ret{};
  try {
    l_ret.emplace();
    l_ret->session_ = in_sql.get_by_uuid<ai_light_session>(in_session_id);
  } catch (const orm::sqlite_orm_exception& in_err) {
    if (in_err.code_status_() != boost::beast::http::status::not_found) throw;
    SPDLOG_LOGGER_DEBUG(g_logger_ctrl().get_http(), "AI 灯光会话 {} 不存在: {}", in_session_id, in_err.what());
    return std::nullopt;
  }

  auto& l_detail                 = *l_ret;
  l_detail.references_           = in_sql.get_ai_light_references(in_session_id);
  l_detail.iterations_           = in_sql.get_ai_light_iterations(in_session_id);
  l_detail.candidates_           = in_sql.get_ai_light_candidates_by_session(in_session_id);

  l_detail.is_waiting_human_     = is_waiting_human(l_detail.session_.state_);
  l_detail.is_judgement_overdue_ = l_detail.session_.is_judgement_overdue(
      chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()}
  );
  return l_ret;
}

}  // namespace doodle::ai_light
