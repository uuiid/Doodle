
#pragma once

#include <doodle_core/metadata/ai_light/task_payload.h>

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <core/http/http_function.h>

namespace doodle::http::ai_light {

// /api/doodle/ai/light/shot/{}/session
DOODLE_HTTP_JWT_FUN(shot_session_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/shot/{entity_id_}/session", entity_id_)
uuid entity_id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/session/{}
DOODLE_HTTP_JWT_FUN(session_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/session/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/session/{}/reference
DOODLE_HTTP_JWT_FUN(reference_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/session/{session_id_}/reference", session_id_)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/reference/{}
DOODLE_HTTP_JWT_FUN(reference_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/reference/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/session/{}/iteration/{}
DOODLE_HTTP_JWT_FUN(judge_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/doodle/ai/light/session/{session_id_}/iteration/{iteration_id_}", session_id_, iteration_id_
)
uuid session_id_{};
uuid iteration_id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/candidate/{}/begin_sequence
DOODLE_HTTP_JWT_FUN(candidate_begin_sequence_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/candidate/{candidate_id_}/begin_sequence", candidate_id_)
uuid candidate_id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/candidate/{}/upload_frame
DOODLE_HTTP_JWT_FUN(candidate_upload_frame_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/candidate/{candidate_id_}/upload_frame", candidate_id_)
uuid candidate_id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/candidate/{}/finish_sequence
DOODLE_HTTP_JWT_FUN(candidate_finish_sequence_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/candidate/{candidate_id_}/finish_sequence", candidate_id_)
uuid candidate_id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/candidate/{}/movie
DOODLE_HTTP_JWT_FUN(candidate_movie_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/candidate/{id_}/movie", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/preset
DOODLE_HTTP_JWT_FUN(preset_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/preset")
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/preset/{}
DOODLE_HTTP_JWT_FUN(preset_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/preset/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/session/{}/inference
DOODLE_HTTP_JWT_FUN(inference_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/ai/light/session/{session_id_}/inference", session_id_)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/ai/light/session/{}/iterate
DOODLE_HTTP_JWT_FUN(iteration_submit_route)
DOODLE_HTTP_FUN_OVERRIDE(post)

DOODLE_HTTP_ROUTE("/api/doodle/ai/light/session/{session_id_}/iteration", session_id_)
uuid session_id_{};

DOODLE_HTTP_FUN_END()
}  // namespace doodle::http::ai_light
