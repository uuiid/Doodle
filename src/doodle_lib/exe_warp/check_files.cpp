//
// Created by TD on 24-8-26.
//

#include "check_files.h"

#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
namespace doodle {
namespace {
FSys::path down_copy_file(FSys::path in_ue_prject_path, logger_ptr in_logger) {
  static auto g_root{FSys::path{doodle_config::g_cache_path}};
  auto l_local_root = g_root / "check" / in_ue_prject_path.stem();
  in_logger->info("复制UE文件到本机缓存文件夹 {} {}", in_ue_prject_path.string(), l_local_root.string());
  if (!FSys::exists(l_local_root)) {
    FSys::create_directories(l_local_root);
  }

  FSys::copy(
      in_ue_prject_path.parent_path() / doodle_config::ue4_config, l_local_root / doodle_config::ue4_config,
      FSys::copy_options::recursive | FSys::copy_options::update_existing
  );
  FSys::copy(
      in_ue_prject_path.parent_path() / doodle_config::ue4_content, l_local_root / doodle_config::ue4_content,
      FSys::copy_options::recursive | FSys::copy_options::update_existing
  );
  FSys::copy(in_ue_prject_path, l_local_root / in_ue_prject_path.filename(), FSys::copy_options::update_existing);
  return l_local_root / in_ue_prject_path.filename();
}

struct run_ue_check_arg_t {
  import_and_render_ue_ns::import_files_t import_files_;  // 需要导入的文件(可空)

  FSys::path render_map_;  // 渲染地编提供的关卡
  std::string create_map;  // 创建的关卡(放置骨骼网格体)

  FSys::path level_sequence_import_;  // 渲染关卡序列(包的路径), 包括下面的子关卡
  FSys::path movie_pipeline_config_;  // 渲染配置(包的路径)
  friend void to_json(nlohmann::json& j, const run_ue_check_arg_t& p) {
    j["import_files"]          = p.import_files_;
    j["render_map"]            = p.render_map_;
    j["create_map"]            = p.create_map;
    j["level_sequence_import"] = p.level_sequence_import_;
    j["movie_pipeline_config"] = p.movie_pipeline_config_;
  }
};
}  // namespace

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    std::shared_ptr<check_files_arg_t> in_args, logger_ptr in_logger
) {
  if (!FSys::exists(in_args->ue_project_path_)) {
    co_return std::make_tuple(boost::asio::error::make_error_code(boost::asio::error::not_found), "文件不存在");
  }
  // 复制UE文件到本机缓存文件夹

  try {
    in_args->local_ue_project_path_ = down_copy_file(in_args->ue_project_path_, in_logger);
  } catch (const FSys::filesystem_error& error) {
    co_return std::make_tuple(error.code(), error.what());
  }
  boost::system::error_code l_ec{};

  if (FSys::exists(in_args->maya_rig_file_)) {
    // 开始导出maya文件, 并进行检查
    auto l_arg = std::make_shared<maya_exe_ns::export_fbx_arg>();
    maya_exe_ns::maya_out_arg l_out{};
    l_arg->rig_file_export = true;
    l_arg->file_path       = in_args->maya_rig_file_;
    l_arg->bitset_ |= maya_exe_ns::flags::k_export_fbx_type;
    for (int i = 0; i < 3; ++i) {
      std::tie(l_ec, l_out) = co_await async_run_maya(l_arg, in_logger);
      if (!l_ec) {
        break;
      }
      in_logger->warn("运行maya错误, 开始第{}次重试", i + 1);
      in_logger->log(level::off, magic_enum::enum_name(process_message::state::pause));
    }
    if (l_ec) co_return std::tuple(l_ec, "maya绑定文件错误");

    // 导入UE中, 检查Ue文件

    if (l_out.out_file_list.empty() && in_args->maya_has_export_fbx_) {
      in_logger->error("运行maya时, 没有导出任何物体, 错误的文件");
      l_ec = boost::asio::error::make_error_code(boost::asio::error::not_found);
      co_return std::tuple(l_ec, "maya绑定文件错误");
    }
    import_and_render_ue_ns::fix_project(in_args->local_ue_project_path_);
    import_and_render_ue_ns::fix_config(in_args->local_ue_project_path_);


  }

}
}