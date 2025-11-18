//
// Created by TD on 25-5-13.
//
#pragma once
#include <doodle_lib/core/http/http_function.h>
namespace doodle::http::local {

class local_http_fun : public http_function {
  using base_type = http_function;

 protected:
  void parse_header(const session_data_ptr& in_handle) override;
  std::string token_;

 public:
  local_http_fun() : base_type() {}
  virtual ~local_http_fun() = default;
};

// /api/doodle/local_setting
DOODLE_HTTP_FUN(local_setting)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/doodle/local_setting/tmp_dir/server_task
DOODLE_HTTP_FUN(local_setting_tmp_dir_server_task)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/doodle/task
DOODLE_HTTP_FUN(task)
void init_ctx();
task() : base_type() { init_ctx(); }
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/doodle/task/{id}
DOODLE_HTTP_FUN(task_instance)
DOODLE_HTTP_FUN_OVERRIDE(patch)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/doodle/task/{id}/inspect
DOODLE_HTTP_FUN_C(task_inspect_instance, local_http_fun)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/doodle/task/{id}/generate_uesk_file
DOODLE_HTTP_FUN_C(task_instance_generate_uesk_file, local_http_fun)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()


// /api/doodle/task/{id}/log
DOODLE_HTTP_FUN(task_instance_log)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/doodle/video/thumbnail
DOODLE_HTTP_FUN(video_thumbnail)
void init_ctx();
video_thumbnail() : base_type() { init_ctx(); }
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/shots/{shot_id}/run-ue-assembly
DOODLE_HTTP_FUN_C(actions_projects_shots_run_ue_assembly_local, local_http_fun)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/shots/{shot_id}/export-anim-fbx
DOODLE_HTTP_FUN_C(actions_projects_shots_export_anim_fbx_local, local_http_fun)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/shots/{shot_id}/update-sim-abc
DOODLE_HTTP_FUN_C(actions_projects_shots_update_sim_abc_local, local_http_fun)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http::local
