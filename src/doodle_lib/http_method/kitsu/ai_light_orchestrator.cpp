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
          c(&ai_light_session::judgement_due_at_)  = in_session.judgement_due_at_,
          c(&ai_light_session::failure_reason_)    = in_session.failure_reason_
      )
      .where(c(&ai_light_session::uuid_id_) == in_session.uuid_id_);
}

}  // namespace

boost::asio::awaitable<void> apply_judgement(
    sqlite_database& in_sql, ai_light_session& in_session, ai_light_iteration& in_iteration,
    const ai_light_judgement_t& in_judgement, std::optional<preview_file> in_accepted_preview_file
) {
  DOODLE_CHICK_HTTP(
      in_iteration.session_id_ == in_session.uuid_id_, bad_request, "轮次 {} 不属于会话 {}", in_iteration.uuid_id_,
      in_session.uuid_id_
  );
  DOODLE_CHICK_HTTP(
      in_session.state_ == ai_light_session_state::awaiting_judgement, bad_request,
      "会话 {} 处于状态 {}，不在等人工判断", in_session.uuid_id_, magic_enum::enum_name(in_session.state_)
  );
  DOODLE_CHICK_HTTP(
      in_iteration.state_ != ai_light_iteration_state::judged, bad_request, "轮次 {} 已经判断过了",
      in_iteration.iteration_
  );

  const bool l_needs_candidate = in_judgement.decision_ == judgement_decision::accept ||
                                 in_judgement.decision_ == judgement_decision::continue_iteration;
  DOODLE_CHICK_HTTP(
      l_needs_candidate == !in_judgement.selected_candidate_id_.is_nil(), bad_request, "判断 {} 与选中的候选 {} 不匹配",
      magic_enum::enum_name(in_judgement.decision_), in_judgement.selected_candidate_id_
  );
  DOODLE_CHICK_HTTP(
      (in_judgement.decision_ == judgement_decision::accept) == in_accepted_preview_file.has_value(),
      internal_server_error, "定稿必须携带已构造好的 preview_file，其余判断不得携带"
  );

  const ai_light_session_state l_next_state = ai_light_session_state::generating;

  orm::sql_modify_statement_vector_t l_sqls{};

  if (in_accepted_preview_file.has_value())
    l_sqls.emplace_back(orm::insert(in_sql).into<preview_file>().values(*in_accepted_preview_file));

  if (in_judgement.decision_ == judgement_decision::accept) {
    if (auto l_prev = in_sql.get_ai_light_accepted_candidate(in_session.uuid_id_);
        l_prev.has_value() && l_prev->uuid_id_ != in_judgement.selected_candidate_id_) {
      l_prev->clear_accepted();
      l_prev->preview_file_id_ = uuid{};
      l_sqls.emplace_back(
          orm::update(in_sql)
              .from<ai_light_candidate>()
              .set(
                  c(&ai_light_candidate::is_accepted_)         = l_prev->is_accepted_,
                  c(&ai_light_candidate::accepted_session_id_) = l_prev->accepted_session_id_,
                  c(&ai_light_candidate::preview_file_id_)     = l_prev->preview_file_id_
              )
              .where(c(&ai_light_candidate::uuid_id_) == l_prev->uuid_id_)
      );
    }
  }

  if (!in_judgement.selected_candidate_id_.is_nil()) {
    auto l_cand = in_sql.get_by_uuid<ai_light_candidate>(in_judgement.selected_candidate_id_);
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == in_iteration.uuid_id_, bad_request, "候选 {} 不属于第 {} 轮", l_cand.uuid_id_,
        in_iteration.iteration_
    );

    if (in_judgement.decision_ == judgement_decision::accept) {
      l_cand.mark_accepted();
      l_cand.preview_file_id_ = in_accepted_preview_file->uuid_id_;
    }
    l_cand.is_selected_ = true;

    l_sqls.emplace_back(
        orm::update(in_sql)
            .from<ai_light_candidate>()
            .set(
                c(&ai_light_candidate::is_selected_)         = l_cand.is_selected_,
                c(&ai_light_candidate::is_accepted_)         = l_cand.is_accepted_,
                c(&ai_light_candidate::accepted_session_id_) = l_cand.accepted_session_id_,
                c(&ai_light_candidate::preview_file_id_)     = l_cand.preview_file_id_
            )
            .where(c(&ai_light_candidate::uuid_id_) == l_cand.uuid_id_)
    );
  }

  in_iteration.apply_judgement(in_judgement);
  l_sqls.emplace_back(
      orm::update(in_sql)
          .from<ai_light_iteration>()
          .set(
              c(&ai_light_iteration::state_)     = in_iteration.state_,
              c(&ai_light_iteration::judgement_) = in_iteration.judgement_,
              c(&ai_light_iteration::judged_by_) = in_iteration.judged_by_,
              c(&ai_light_iteration::judged_at_) = in_iteration.judged_at_
          )
          .where(c(&ai_light_iteration::uuid_id_) == in_iteration.uuid_id_)
  );

  in_session.mark_judged(l_next_state);
  in_session.failure_reason_ = "";
  l_sqls.emplace_back(make_session_update(in_sql, in_session));

  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮判断 {}（操作人 {}）", in_session.uuid_id_,
      in_iteration.iteration_, magic_enum::enum_name(in_judgement.decision_), in_judgement.by_
  );
  co_return;
}

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
  l_session.failure_reason_ = "";
  l_sqls.emplace_back(make_session_update(in_sql, l_session));

  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮渲染完成，{} 个候选待判断", l_session.uuid_id_,
      l_it->iteration_, in_candidates.size()
  );
  co_return;
}

namespace {

constexpr std::string_view g_render_report_seam{"round_render_report"};

}  // namespace

round_render_report_t parse_round_render_report(const nlohmann::json& in_json) {
  round_render_report_t l_ret{};

  auto l_opt_string = [](const nlohmann::json& in_parent, std::string_view in_key) -> std::string {
    if (!in_parent.is_object() || !in_parent.contains(in_key)) return {};
    const auto& l_v = in_parent.at(in_key);
    return l_v.is_string() ? l_v.get<std::string>() : std::string{};
  };
  auto l_opt_number = [](const nlohmann::json& in_parent, std::string_view in_key) -> std::optional<double> {
    if (!in_parent.is_object() || !in_parent.contains(in_key)) return std::nullopt;
    const auto& l_v = in_parent.at(in_key);
    return l_v.is_number() ? std::optional<double>{l_v.get<double>()} : std::nullopt;
  };

  l_ret.error_ = l_opt_string(in_json, "error");

  DOODLE_CHICK_HTTP(
      in_json.is_object() && in_json.contains("candidates") && in_json.at("candidates").is_array(), bad_gateway,
      "{}：字段 candidates 缺失或类型不符合（期望数组）", g_render_report_seam
  );
  const auto& l_candidates = in_json.at("candidates");
  l_ret.candidates_.reserve(l_candidates.size());

  for (std::size_t i = 0; i < l_candidates.size(); ++i) {
    const auto l_base  = fmt::format("candidates[{}]", i);

    const auto& l_item = l_candidates.at(i);
    DOODLE_CHICK_HTTP(
        l_item.is_object(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望对象）", g_render_report_seam, l_base
    );

    candidate_render_report_t l_cand{};

    const auto l_id_path = l_base + ".candidate_id";
    DOODLE_CHICK_HTTP(
        l_item.contains("candidate_id") && l_item.at("candidate_id").is_string(), bad_gateway,
        "{}：字段 {} 缺失或类型不符合（期望 uuid 字符串）", g_render_report_seam, l_id_path
    );
    try {
      l_cand.candidate_id_ = l_item.at("candidate_id").get<uuid>();
    } catch (const std::exception& in_err) {
      DOODLE_CHICK_HTTP(
          false, bad_gateway, "{}：字段 {} 不是合法的 uuid（{}）", g_render_report_seam, l_id_path, in_err.what()
      );
    }

    DOODLE_CHICK_HTTP(
        l_item.contains("status") && l_item.at("status").is_string(), bad_gateway,
        "{}：字段 {}.status 缺失或类型不符合（期望字符串）", g_render_report_seam, l_base
    );
    {
      const auto l_status_name = l_item.at("status").get<std::string>();
      const auto l_status_v    = magic_enum::enum_cast<candidate_render_status>(l_status_name);
      DOODLE_CHICK_HTTP(
          l_status_v.has_value(), bad_gateway, "{}：字段 {}.status 的值 \"{}\" 不是合法的 {}", g_render_report_seam,
          l_base, l_status_name, magic_enum::enum_type_name<candidate_render_status>()
      );
      l_cand.status_ = *l_status_v;
    }

    l_cand.candidate_index_ =
        static_cast<std::int32_t>(l_opt_number(l_item, "candidate_index").value_or(static_cast<double>(i)));

    if (l_item.contains("metrics")) {
      try {
        l_item.at("metrics").get_to(l_cand.metrics_);
      } catch (const std::exception& in_err) {
        SPDLOG_LOGGER_WARN(
            g_logger_ctrl().get_http(), "AI 灯光回传：候选 {} 的 metrics 不合规，已丢弃（{}）", l_cand.candidate_id_,
            in_err.what()
        );
      }
    }

    l_ret.candidates_.emplace_back(std::move(l_cand));
  }

  return l_ret;
}

boost::asio::awaitable<void> mark_iteration_failed(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator,
    const std::string& in_reason
) {
  auto l_it = locate_iteration(in_sql, in_server_task_info_id, in_locator);
  DOODLE_CHICK_HTTP(l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id);

  auto l_session            = in_sql.get_by_uuid<ai_light_session>(l_it->session_id_);
  l_session.failure_reason_ = in_reason;

  orm::sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(make_session_update(in_sql, l_session));
  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮渲染失败（会话停留在 {}）：{}", l_session.uuid_id_,
      l_it->iteration_, magic_enum::enum_name(l_session.state_), l_session.failure_reason_
  );
  co_return;
}

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
    co_await mark_iteration_failed(
        in_sql, in_server_task_info_id, in_locator,
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
    co_await mark_iteration_failed(
        in_sql, in_server_task_info_id, in_locator,
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
  l_detail.accepted_candidate_   = in_sql.get_ai_light_accepted_candidate(in_session_id);

  l_detail.is_waiting_human_     = is_waiting_human(l_detail.session_.state_);
  l_detail.is_judgement_overdue_ = l_detail.session_.is_judgement_overdue(
      chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()}
  );
  return l_ret;
}

}  // namespace doodle::ai_light
