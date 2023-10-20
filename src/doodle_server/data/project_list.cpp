//
// Created by td_main on 2023/10/19.
//

#include "project_list.h"

#include <utility>
namespace doodle::http {

project_storage_type::project_storage_type() : executor_(boost::asio::make_strand(g_io_context())){};
project_storage_type::project_storage_type(FSys::path in_project_path)
    : executor_(boost::asio::make_strand(g_io_context())), project_path_(std::move(in_project_path)) {}

void project_storage_type::load_project() {
  // 直接初始化
  if (!FSys::exists(project_path_)) {
    FSys::create_directory(project_path_.parent_path());
    registry_.ctx().emplace<project_config::base_config>(project_config::base_config::get_default());
    auto& l_prj = registry_.ctx().emplace<project>();
    l_prj.set_path(project_path_);
    l_prj.set_name(project_path_.stem().string());
  } else {
  }
}

void project_storage_type::save_project() {}

}  // namespace doodle::http