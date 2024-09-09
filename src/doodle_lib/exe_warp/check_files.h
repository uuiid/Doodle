//
// Created by TD on 24-8-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/project.h>
namespace doodle {

struct check_files_arg_t {
  FSys::path maya_rig_file_;
  FSys::path ue_project_path_;
  std::string ue_main_file_;
  FSys::path out_files_dir_;

  bool maya_has_export_fbx_;

  /// 本地缓存UE项目路径
  FSys::path local_ue_project_path_;

  // 检查类型
  enum check_type { char_, scene };

  check_type check_type_;

  project project_;
};

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    std::shared_ptr<check_files_arg_t> in_args, logger_ptr in_logger
);

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    boost::uuids::uuid in_check_path, logger_ptr in_logger
);

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    const FSys::path& in_check_path, logger_ptr in_logger
);

}  // namespace doodle