//
// Created by TD on 24-8-26.
//

#include "check_files.h"
namespace doodle {
boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    std::shared_ptr<check_files_arg_t> in_args, logger_ptr in_logger
) {
  if (!FSys::exists(in_args->ue_project_path_)) {
    co_return std::make_tuple(boost::asio::error::make_error_code(boost::asio::error::not_found), "文件不存在");
  }
  // 复制UE文件到本机缓存文件夹
  try {
    static auto g_root{FSys::path{doodle_config::g_cache_path}};
    auto l_local_root = g_root / "check" / in_args->ue_project_path_.stem();
    in_logger->info("复制UE文件到本机缓存文件夹 {} {}", in_args->ue_project_path_.string(), l_local_root.string());
    if (!FSys::exists(l_local_root)) {
      FSys::create_directories(l_local_root);
    }

    FSys::copy(
        in_args->ue_project_path_.parent_path() / doodle_config::ue4_config, l_local_root / doodle_config::ue4_config,
        FSys::copy_options::recursive | FSys::copy_options::update_existing
    );
    FSys::copy(
        in_args->ue_project_path_.parent_path() / doodle_config::ue4_content, l_local_root / doodle_config::ue4_content,
        FSys::copy_options::recursive | FSys::copy_options::update_existing
    );
    FSys::copy(
        in_args->ue_project_path_, l_local_root / in_args->ue_project_path_.filename(),
        FSys::copy_options::update_existing
    );
  } catch (const FSys::filesystem_error& error) {
    co_return std::make_tuple(error.code(), error.what());
  }

  if (FSys::exists(in_args->maya_rig_file_)) {
    // 开始导出maya文件, 并进行检查

    // 导入UE中, 检查Ue文件
  }

}
}