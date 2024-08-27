//
// Created by TD on 24-8-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {

struct check_files_arg_t {
  FSys::path maya_rig_file_;
  FSys::path ue_project_path_;
  std::string ue_main_file_;
  FSys::path out_files_dir_;

  bool maya_has_export_fbx_;

  /// 本地缓存UE项目路径
  FSys::path local_ue_project_path_;
};

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    std::shared_ptr<check_files_arg_t> in_args, logger_ptr in_logger
);

}  // namespace doodle