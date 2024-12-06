//
// Created by TD on 24-8-26.
//

#include "check_files.h"

#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/long_task/image_to_move.h>

namespace doodle {
namespace {
FSys::path down_copy_file(
    FSys::path in_ue_prject_path, logger_ptr in_logger, const std::string& in_project_short_string
) {
  static auto g_root{FSys::path{doodle_config::g_cache_path}};
  auto l_local_root = g_root / "check" / in_project_short_string / in_ue_prject_path.stem();
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
  std::optional<import_and_render_ue_ns::import_files_t> import_files_;  // 需要导入的文件(可空)

  FSys::path render_map_;             // 渲染的关卡
  std::string create_map_;            // 创建的关卡(放置骨骼网格体)
  std::string original_map_;          // 地编提供的主场景路径, 我们需要抓取子场景
  FSys::path out_file_dir_;           // 输出文件夹
  FSys::path level_sequence_import_;  // 渲染关卡序列(包的路径), 包括下面的子关卡
  FSys::path movie_pipeline_config_;  // 渲染配置(包的路径)
  std::string check_type_;            // 检查类型

  FSys::path import_dir_;  // 导入文件的位置
  friend void to_json(nlohmann::json& j, const run_ue_check_arg_t& p) {
    if (p.import_files_) j["import_files"] = *p.import_files_;
    j["render_map"]            = p.render_map_;
    j["create_map"]            = p.create_map_;
    j["original_map"]          = p.original_map_;
    j["out_file_dir"]          = p.out_file_dir_;
    j["import_dir"]            = p.import_dir_;
    auto l_level_sequence_path = p.level_sequence_import_;
    l_level_sequence_path.replace_extension();
    j["level_sequence_import"] = l_level_sequence_path;
    auto l_movie_pipeline_path = p.movie_pipeline_config_;
    l_movie_pipeline_path.replace_extension();
    j["movie_pipeline_config"] = l_movie_pipeline_path;
    j["check_type"]            = p.check_type_;
  }
};

run_ue_check_arg_t create_check_arg(
    const check_files_arg_t& in_args, const maya_exe_ns::maya_out_arg& in_maya_out_arg
) {
  run_ue_check_arg_t l_arg;

  if (!in_maya_out_arg.out_file_list.empty() && FSys::exists(in_maya_out_arg.out_file_list.front().out_file)) {
    auto l_out_path     = in_maya_out_arg.out_file_list.front().out_file;
    l_arg.import_files_ = {.type_ = l_out_path.extension() == ".fbx" ? "char" : "geo", .path_ = l_out_path};
  }
  l_arg.original_map_ = in_args.ue_main_file_;
  l_arg.render_map_   = fmt::format("{}/check/main_map", doodle_config::ue4_game);
  l_arg.create_map_   = fmt::format("{}/check/sub_import_map", doodle_config::ue4_game);
  // l_arg.import_dir_ =
  //     fmt::format("{}/check/import_{4:%m_%d_%H_%M}", std::string{doodle_config::ue4_game},
  //     time_point_wrap{}.get_local_time());
  l_arg.import_dir_   = fmt::format(
      "{}/check/import_{:%m_%d_%H_%M}", std::string{doodle_config::ue4_game}, time_point_wrap{}.get_local_time()
  );
  l_arg.out_file_dir_          = in_args.out_files_dir_;
  l_arg.level_sequence_import_ = fmt::format("{}/check/{}", doodle_config::ue4_game, in_args.ue_project_path_.stem());
  l_arg.movie_pipeline_config_ = fmt::format("{}/check/main_level_sequence_config", doodle_config::ue4_game);

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
    in_args->local_ue_project_path_ = down_copy_file(in_args->ue_project_path_, in_logger, in_args->project_.shor_str_);
  } catch (const FSys::filesystem_error& error) {
    co_return std::make_tuple(error.code(), error.what());
  }
  boost::system::error_code l_ec{};

  // 开始导出maya文件, 并进行检查
  auto l_arg = std::make_shared<maya_exe_ns::export_fbx_arg>();
  maya_exe_ns::maya_out_arg l_out{};
  l_arg->rig_file_export_ = true;
  l_arg->file_path        = in_args->maya_rig_file_;
  for (int i = 0; i < 3; ++i) {
    std::tie(l_ec, l_out) = co_await async_run_maya(l_arg, in_logger);
    if (!l_ec) {
      break;
    }
    in_logger->warn("运行maya错误, 开始第{}次重试", i + 1);
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
  // 调整输出目录
  in_args->out_files_dir_ = in_args->local_ue_project_path_.parent_path() / doodle_config::ue4_saved /
                            doodle_config::ue4_movie_renders / in_args->ue_project_path_.stem();
  if (FSys::exists(in_args->out_files_dir_)) {
    try {
      FSys::remove_all(in_args->out_files_dir_);
    } catch (FSys::filesystem_error& err) {
      in_logger->warn("渲染删除上次输出错误 error:{}", err.what());
    }
  }

  auto l_check_arg                = create_check_arg(*in_args, l_out);
  l_check_arg.check_type_         = magic_enum::enum_name(in_args->check_type_);
  nlohmann::json l_run_import_arg = l_check_arg;
  auto l_tmp_path                 = FSys::write_tmp_file("ue_check", l_run_import_arg.dump(), ".json");
  in_logger->warn("排队导入文件 {} ", in_args->local_ue_project_path_);
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    in_logger->error("用户取消操作");
    co_return std::tuple(boost::system::error_code{boost::asio::error::operation_aborted}, std::string{"用户取消操作"});
  }
  // 添加三次重试
  for (int i = 0; i < 3; ++i) {
    l_ec = co_await async_run_ue(
        {in_args->local_ue_project_path_.generic_string(), "-windowed", "-log", "-stdout", "-AllowStdOutLogVerbosity",
         "-ForceLogFlush", "-Unattended", "-run=DoodleAutoAnimation", fmt::format("-Check={}", l_tmp_path)},
        in_logger
    );
    if (!l_ec) break;
    in_logger->warn("导入文件失败 开始第 {} 重试", i + 1);
    // 这个错误可以忽略, 有错误的情况下, 将状态设置为运行
  }
  if (l_ec) co_return std::tuple(l_ec, std::string{});
  in_logger->warn("导入文件完成");
  in_logger->warn("排队渲染, 输出目录 {}", l_check_arg.out_file_dir_);
  if (exists(l_check_arg.out_file_dir_)) {
    try {
      FSys::remove_all(l_check_arg.out_file_dir_);
    } catch (FSys::filesystem_error& err) {
      in_logger->error("渲染删除上次输出错误 error:{}", err.what());
    }
  }
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    in_logger->error(" 用户取消操作");
    co_return std::tuple(boost::system::error_code{boost::asio::error::operation_aborted}, std::string{});
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
  }
  co_return std::tuple(l_ec, std::string{});
}

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    boost::uuids::uuid in_check_path, logger_ptr in_logger
) {
  if (in_check_path.is_nil())
    co_return std::tuple(
        boost::asio::error::make_error_code(boost::asio::error::not_found), std::string{"错误的空文件id"}
    );

  boost::system::error_code l_ec{};
  std::vector<association_data> l_face_data{};
  std::string l_err_msg{};
  std::tie(l_ec, l_face_data) = co_await fetch_association_data({in_check_path}, in_logger);
  if (l_ec) {
    in_logger->error("获取关联数据失败 {}", l_ec.message());
    co_return std::tuple(l_ec, std::string{"错误的关联数据"});
  }
  auto l_arg              = std::make_shared<check_files_arg_t>();
  l_arg->maya_rig_file_   = l_face_data.front().maya_file_;
  l_arg->ue_project_path_ = l_face_data.front().ue_prj_path_;
  l_arg->project_         = l_face_data.front().project_;
  if (auto l_type = l_face_data.front().type_;
      l_type == details::assets_type_enum::character || l_type == details::assets_type_enum::prop) {
    l_arg->maya_has_export_fbx_ = true;
  }

  auto l_ue_dir = l_arg->ue_project_path_.parent_path();
  if (auto l_type = l_face_data.front().type_; l_type == details::assets_type_enum::scene) {
    l_arg->ue_main_file_ = fmt::format(
        "{}/{}", doodle_config::ue4_game,
        l_face_data.front().ue_file_.lexically_relative(l_ue_dir / doodle_config::ue4_content).replace_extension()
    );
  }

  std::tie(l_ec, l_err_msg) = co_await check_files(l_arg, in_logger);
  if (l_ec) {
    in_logger->error("检查文件失败 {}", l_ec.message());
    co_return std::tuple(l_ec, l_err_msg);
  }
  std::vector<FSys::path> l_move_paths{};
  std::tie(l_ec, l_move_paths) = clean_1001_before_frame(l_arg->out_files_dir_, 1001);
  if (l_ec) {
    in_logger->error("检查文件失败, 无法获取到渲染输出的文件 {}", l_ec.message());
    co_return std::tuple(l_ec, l_err_msg);
  }

  std::vector<movie::image_attr> l_attr{};
  auto l_t = chrono::floor<chrono::seconds>(time_point_wrap{}.get_local_time());
  for (auto l_p : l_move_paths) {
    movie::image_attr l_attribute{};
    l_attribute.path_attr = l_p;
    l_attribute.watermarks_attr.emplace_back(
        fmt::format("{:%Y-%m-%d %H:%M:%S}", l_t), 0.8, 0.1, movie::image_watermark::rgb_default
    );
    l_attr.emplace_back(std::move(l_attribute));
  }
  movie::image_attr::extract_num(l_attr);
  auto l_out_file = l_arg->out_files_dir_.parent_path() / fmt::format("{}.mp4", l_arg->out_files_dir_.stem());
  l_ec            = detail::create_move(l_out_file, in_logger, l_attr);
  if (l_ec) {
    in_logger->error("检查文件失败, 无法获取到渲染输出的文件 {}", l_ec.message());
    co_return std::tuple(l_ec, l_err_msg);
  }
  std::error_code l_ec2{};
  auto l_target = l_face_data.front().project_.path_ / "03_Workflow" /
                  magic_enum::enum_name(l_face_data.front().type_) / l_out_file.filename();
  if (!FSys::exists(l_target.parent_path())) FSys::create_directories(l_target.parent_path());
  FSys::copy(
      l_out_file,
      l_face_data.front().project_.path_ / "03_Workflow" / magic_enum::enum_name(l_face_data.front().type_) /
          l_out_file.filename(),
      FSys::copy_options::update_existing, l_ec2
  );
  if (l_ec2) {
    in_logger->error("复制文件失败 {}", l_ec2.message());
    co_return std::tuple(l_ec2, l_err_msg);
  }
  co_return std::tuple(boost::system::error_code{}, std::string{});

}

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> check_files(
    const FSys::path& in_check_path, logger_ptr in_logger
) {
  return check_files(FSys::software_flag_file(in_check_path), in_logger);
}


}