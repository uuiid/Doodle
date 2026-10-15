
#pragma once

#include <doodle_core/metadata/ai_light/task_payload.h>

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <core/http/http_function.h>

namespace doodle::http::ai_light {

DOODLE_HTTP_JWT_FUN(shot_session_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid entity_id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(session_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(reference_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(reference_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_SIGNED_FUN(reference_image_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_SIGNED_FUN(candidate_image_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(judge_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(render_report_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid task_id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(render_report_local_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
std::int32_t iteration_{0};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(candidate_file_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
doodle::ai_light::ai_light_candidate_file_role role_{doodle::ai_light::ai_light_candidate_file_role::render};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(candidate_movie_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(preset_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(preset_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(inference_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_JWT_FUN(iteration_submit_route)
DOODLE_HTTP_FUN_OVERRIDE(post)

uuid session_id_{};

DOODLE_HTTP_FUN_END()
}  // namespace doodle::http::ai_light
