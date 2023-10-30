//
// Created by td_main on 2023/10/19.
//

#include "project_list.h"

#include <doodle_core/database_task/details/load_save_impl.h>

#include <utility>
namespace doodle::http {

project_storage_type::project_storage_type()
    : executor_(boost::asio::make_strand(g_io_context())),
      loader_{doodle::database_n::obs_all{}},
      registry_(std::make_shared<entt::registry>()){};
project_storage_type::project_storage_type(FSys::path in_project_path)
    : executor_(boost::asio::make_strand(g_io_context())),
      loader_{doodle::database_n::obs_all{}},
      project_path_(std::move(in_project_path)),
      registry_(std::make_shared<entt::registry>()) {}

void project_storage_type::load_project() {
  // 直接初始化
  if (!FSys::exists(project_path_)) {
    FSys::create_directory(project_path_.parent_path());
    registry_->ctx().emplace<project_config::base_config>(project_config::base_config::get_default());
    auto& l_prj = registry_->ctx().emplace<project>();
    l_prj.set_path(project_path_);
    l_prj.set_name(project_path_.stem().string());
    l_prj.set_path(project_path_.parent_path());
  } else {
    auto& l_loader = std::any_cast<doodle::database_n::obs_all&>(loader_);
  }
}

void project_storage_type::save_project() {}

}  // namespace doodle::http