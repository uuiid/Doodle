#pragma once
#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>


namespace doodle::http::auto_task {

run_ue_assembly_local::run_ue_assembly_arg shot_render_light(const uuid& in_project_id, const uuid& in_shot_id);
}