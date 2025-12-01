//
// Created by TD on 24-12-13.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
// /api/data/user/context
DOODLE_HTTP_FUN(epiboly_user_context)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/export-anim-fbx
DOODLE_HTTP_FUN(epiboly_actions_projects_export_anim_fbx)

void init_ctx();
epiboly_actions_projects_export_anim_fbx() : base_type() { init_ctx(); }
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http