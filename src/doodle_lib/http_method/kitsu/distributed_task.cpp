#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/task.h>

#include "doodle_lib/doodle_lib_fwd.h"
#include <doodle_lib/core/entity_path.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/exe_warp/export_fbx_arg.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_method/kitsu/auto_task.h>
#include <doodle_lib/http_method/kitsu/computers.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <memory>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_projects_shots_run_ue_assembly, get) {
  person_.check_not_outsourcer();
  auto l_ret = auto_task::shot_render_light(project_id_, id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成生成 UE 装配参数 project_id {} task_id {} asset_count {} camera_file {}", person_.person_.email_,
      person_.person_.get_full_name(), project_id_, id_, l_ret.asset_infos_.size(),
      l_ret.camera_file_path_.filename().generic_string()
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_ret);
}

namespace {
std::shared_ptr<server_task_info> make_server_task_info_from_json(
    const nlohmann::json& in_json, const uuid& person_id, const uuid& task_id
) {
  auto l_sql = get_sqlite_database();

  auto l_ptr = std::make_shared<server_task_info>();
  in_json.get_to(*l_ptr);
  l_ptr->status_      = server_task_info_status::submitted;
  l_ptr->submitter_   = person_id;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_ptr->type_        = server_task_info_type::auto_light;
  l_ptr->task_id_     = task_id;
  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  return l_ptr;
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_projects_shots_run_ue_assembly, post) {
  person_.check_not_outsourcer();
  auto l_ptr = make_server_task_info_from_json(in_handle->get_json(), person_.person_.uuid_id_, id_);

#ifdef NDEBUG
  l_ptr->command_ = auto_task::shot_render_light(project_id_, id_);
#endif
  co_await get_sqlite_database().install(l_ptr);
  if (!l_ptr->run_computer_id_.is_nil())
    co_await computers_assign_task::get_instance().run_next_task(l_ptr->run_computer_id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 提交 UE 装配任务 project_id {} task_id {} job_id {} computer_id {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, id_, l_ptr->uuid_id_,
      l_ptr->run_computer_id_
  );
  socket_io::broadcast(socket_io::server_task_info_new_broadcast_t{.server_task_info_id_ = l_ptr->uuid_id_});
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

namespace {
export_fbx_arg_distributed::args shot_export_anim_fbx(const uuid& project_id, const uuid& in_task_id) {
  auto l_sql            = get_sqlite_database();
  auto l_task           = l_sql.get_by_uuid<task>(in_task_id);
  auto l_proj           = l_sql.get_by_uuid<project>(l_task.project_id_);
  auto l_entity         = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_episode_entity = l_sql.get_by_uuid<entity>(l_entity.parent_id_);
  auto l_ext            = l_sql.get_entity_shot_extend(l_entity.uuid_id_);
  DOODLE_CHICK_HTTP(l_ext, bad_request, "镜头扩展信息不存在，无法导出动画 fbx");
  DOODLE_CHICK_HTTP(l_ext->frame_in_, bad_request, "镜头扩展信息开始帧不存在，无法导出动画 fbx");
  DOODLE_CHICK_HTTP(l_ext->frame_out_, bad_request, "镜头扩展信息结束帧不存在，无法导出动画 fbx");
  export_fbx_arg_distributed::args l_arg{};
  l_arg.create_play_blast_ = true;
  l_arg.maya_file_name_    = get_shots_animation_file_name(l_episode_entity, l_entity, l_proj);
  l_arg.maya_file_name_.replace_extension(".ma");
  l_arg.download_file_ =
      l_proj.path_ / get_shots_animation_maya_path(l_episode_entity) / l_arg.maya_file_name_.filename();
  l_arg.film_aperture_ = l_proj.get_film_aperture();
  l_arg.size_          = l_proj.get_resolution();
  l_arg.frame_in_      = *l_ext->frame_in_;
  l_arg.frame_out_     = *l_ext->frame_out_;
  DOODLE_CHICK_HTTP(!l_arg.download_file_.empty(), bad_request, "生成的 maya 文件路径无效，无法导出动画 fbx");
  DOODLE_CHICK_HTTP(FSys::exists(l_arg.download_file_), bad_request, "生成的 maya 文件路径不存在，无法导出动画 fbx");
  return l_arg;
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_projects_shots_run_export_anim_fbx, post) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  auto l_ptr = make_server_task_info_from_json(in_handle->get_json(), person_.person_.uuid_id_, id_);
#ifdef NDEBUG
  l_ptr->command_ = shot_export_anim_fbx(project_id_, id_);
#endif
  co_await get_sqlite_database().install(l_ptr);
  if (!l_ptr->run_computer_id_.is_nil())
    co_await computers_assign_task::get_instance().run_next_task(l_ptr->run_computer_id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 提交动画导出任务 project_id {} task_id {} job_id {} computer_id {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, id_, l_ptr->uuid_id_,
      l_ptr->run_computer_id_
  );
  socket_io::broadcast(socket_io::server_task_info_new_broadcast_t{.server_task_info_id_ = l_ptr->uuid_id_});
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}
}  // namespace doodle::http