#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include "doodle_lib_fwd.h"
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/auto_task.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

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

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_projects_shots_run_ue_assembly, post) {
  person_.check_not_outsourcer();

  auto l_sql              = get_sqlite_database();

  auto l_ptr              = std::make_shared<server_task_info>();
  l_ptr->type_            = server_task_info_type::auto_light;
  l_ptr->submit_time_     = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();

  auto l_json             = in_handle->get_json();
  l_json.get_to(*l_ptr);
  l_ptr->status_      = server_task_info_status::submitted;
  l_ptr->submitter_   = person_.person_.uuid_id_;
  l_ptr->submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_ptr->type_        = server_task_info_type::auto_light;
  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  if (l_ptr->run_computer_id_.is_nil()) {
    auto l_all_computer = l_sql.get_all<computer>();
    // 过滤在线的计算机
    std::vector<computer> l_online_computer{};
    for (auto&& i : l_all_computer)
      if (i.status_ == computer_status::online) l_online_computer.push_back(i);
    if (l_online_computer.empty()) l_online_computer = l_all_computer;  // 如果没有在线的计算机，就使用所有计算机
    // 随机选择一台计算机
    static std::random_device rd{};
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, l_online_computer.size() - 1);
    l_ptr->run_computer_id_ = l_online_computer[dis(gen)].uuid_id_;
  }
#ifdef NDEBUG
  l_ptr->command_ = auto_task::shot_render_light(project_id_, id_);
#endif
  co_await get_sqlite_database().install(l_ptr);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 提交 UE 装配任务 project_id {} task_id {} job_id {} computer_id {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, id_, l_ptr->uuid_id_,
      l_ptr->run_computer_id_
  );
  socket_io::broadcast("doodle:task_info:create", nlohmann::json{} = *l_ptr);
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}
}  // namespace doodle::http