#pragma once

#include <doodle_core/metadata/ai_light/preset.h>
#include <doodle_core/metadata/ai_light/task_payload.h>
#include <doodle_core/metadata/ai_light/view.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/preview_file.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

inline FSys::path candidate_render_path(const http::kitsu_ctx_t& in_ctx, const ai_light_candidate& in_candidate) {
  return in_ctx.get_ai_light_candidate_thumbnail_file(in_candidate.uuid_id_);
}

inline FSys::path candidate_movie_path(const http::kitsu_ctx_t& in_ctx, const ai_light_candidate& in_candidate) {
  return in_ctx.get_ai_light_candidate_movie_file(in_candidate.uuid_id_);
}

DOODLELIB_API std::optional<ai_light_session_detail_t> build_session_detail(
    sqlite_database& in_sql, const uuid& in_session_id
);

}  // namespace doodle::ai_light