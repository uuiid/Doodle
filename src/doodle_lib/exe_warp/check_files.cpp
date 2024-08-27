//
// Created by TD on 24-8-26.
//

#include "check_files.h"

#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
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

  FSys::path render_map_;             // 渲染的关卡
  std::string create_map_;            // 创建的关卡(放置骨骼网格体)
  std::string original_map_;          // 地编提供的主场景路径, 我们需要抓取子场景
  FSys::path out_file_dir_;           // 输出文件夹
  FSys::path level_sequence_import_;  // 渲染关卡序列(包的路径), 包括下面的子关卡
  FSys::path movie_pipeline_config_;  // 渲染配置(包的路径)
  friend void to_json(nlohmann::json& j, const run_ue_check_arg_t& p) {
    j["import_files"]          = p.import_files_;
    j["render_map"]            = p.render_map_;
    j["create_map"]            = p.create_map_;
    j["original_map"]          = p.original_map_;
    j["out_file_dir"]          = p.out_file_dir_;
    j["level_sequence_import"] = p.level_sequence_import_;
    j["movie_pipeline_config"] = p.movie_pipeline_config_;
  }
};

run_ue_check_arg_t create_check_arg(
    const check_files_arg_t& in_args, const maya_exe_ns::maya_out_arg& in_maya_out_arg
) {
  run_ue_check_arg_t l_arg;
  auto l_out_path = in_maya_out_arg.out_file_list[0].out_file;
  if (FSys::exists(l_out_path))
    l_arg.import_files_ = {.path_ = l_out_path, .type_ = l_out_path.extension() == ".fbx" ? "char" : "geo"};
  l_arg.original_map_          = in_args.ue_main_file_;
  l_arg.render_map_            = fmt::format("/{}/check/main_map", doodle_config::ue4_game);
  l_arg.create_map_            = fmt::format("/{}/check/sub_import_map", doodle_config::ue4_game);
  l_arg.level_sequence_import_ = fmt::format("/{}/check/main_level_sequence", doodle_config::ue4_game);
  l_arg.movie_pipeline_config_ = fmt::format("/{}/check/main_level_sequence_config", doodle_config::ue4_game);

  l_arg.movie_pipeline_config_.replace_extension(l_arg.movie_pipeline_config_.stem());
  l_arg.level_sequence_import_.replace_extension(l_arg.level_sequence_import_.stem());
  return l_arg;
}

}  // namespace

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    std::shared_ptr<check_files_arg_t> in_args, logger_ptr in_logger
) {
  if (!FSys::exists(in_args->ue_project_path_) || !FSys::exists(in_args->maya_rig_file_)) {
    co_return std::make_tuple(boost::asio::error::make_error_code(boost::asio::error::not_found), "文件不存在");
  }
  // 复制UE文件到本机缓存文件夹
  try {
    in_args->local_ue_project_path_ = down_copy_file(in_args->ue_project_path_, in_logger);
  } catch (const FSys::filesystem_error& error) {
    co_return std::make_tuple(error.code(), error.what());
  }
  boost::system::error_code l_ec{};

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

  auto l_check_arg                = create_check_arg(*in_args, l_out);
  nlohmann::json l_run_import_arg = l_check_arg;
  auto l_tmp_path                 = FSys::write_tmp_file("ue_check", l_run_import_arg.dump(), ".json");
  in_logger->warn("排队导入文件 {} ", in_args->local_ue_project_path_);
  in_logger->log(level::off, magic_enum::enum_name(process_message::state::pause));
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    in_logger->error(" 用户取消操作");
    co_return std::tuple(boost::system::error_code{boost::asio::error::operation_aborted}, FSys::path{});
  }
  // 添加三次重试
  for (int i = 0; i < 3; ++i) {
    l_ec = co_await async_run_ue(
        {in_args->local_ue_project_path_.generic_string(), "-windowed", "-log", "-stdout", "-AllowStdOutLogVerbosity",
         "-ForceLogFlush", "-Unattended", "-run=DoodleAutoAnimation", fmt::format("-Params={}", l_tmp_path)},
        in_logger
    );
    if (!l_ec) break;
    in_logger->warn("导入文件失败 开始第 {} 重试", i + 1);
    // 这个错误可以忽略, 有错误的情况下, 将状态设置为运行
    in_logger->log(level::off, magic_enum::enum_name(process_message::state::pause));
  }
  if (l_ec) co_return std::tuple(l_ec, FSys::path{});
  in_logger->warn("导入文件完成");
  in_logger->warn("排队渲染, 输出目录 {}", l_check_arg.out_file_dir_);
  if (exists(l_check_arg.out_file_dir_)) {
    try {
      FSys::remove_all(l_check_arg.out_file_dir_);
    } catch (FSys::filesystem_error& err) {
      in_logger->error("渲染删除上次输出错误 error:{}", err.what());
    }
  }
  in_logger->log(level::off, magic_enum::enum_name(process_message::state::pause));
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    in_logger->error(" 用户取消操作");
    co_return std::tuple(boost::system::error_code{boost::asio::error::operation_aborted}, FSys::path{});
  }
  for (int i = 0; i < 3; ++i) {
    l_ec = co_await async_run_ue(
        {in_args->local_ue_project_path_.generic_string(), l_check_arg.render_map_.generic_string(), "-game",
         fmt::format(R"(-LevelSequence="{}")", l_check_arg.level_sequence_import_),
         fmt::format(R"(-MoviePipelineConfig="{}")", l_check_arg.movie_pipeline_config_), "-windowed", "-log",
         "-stdout", "-AllowStdOutLogVerbosity", "-ForceLogFlush", "-Unattended"},
        in_logger
    );
    if (!l_ec) break;
    in_logger->warn("渲染失败 开始第 {} 重试", i + 1);
    // 这个错误可以忽略, 有错误的情况下, 将状态设置为运行
    in_logger->log(level::off, magic_enum::enum_name(process_message::state::pause));
  }
  if (l_ec) co_return std::tuple(l_ec, FSys::path{});
}
}