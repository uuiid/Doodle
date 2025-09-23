//
// Created by TD on 25-5-13.
//
#pragma once
#include <doodle_lib/core/http/http_function.h>
namespace doodle::http::local {
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

// /api/doodle/task/{id}/restart
DOODLE_HTTP_FUN(task_instance_restart)
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
}  // namespace doodle::http::local
