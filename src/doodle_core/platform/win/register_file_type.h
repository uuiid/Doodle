//
// Created by TD on 2022/2/22.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>

namespace doodle {

class register_file_type {
  static std::string get_key_str();

 public:
  register_file_type();

  void register_type();

  static FSys::path get_main_project();
  static FSys::path get_update_path();
  static const std::vector<project>& get_project_list();
  static FSys::path program_location();

  // 获取服务器地址
  static std::string get_server_address();

  // 获取服务器快照保存位置
  static FSys::path get_server_snapshot_path();
};

}  // namespace doodle
