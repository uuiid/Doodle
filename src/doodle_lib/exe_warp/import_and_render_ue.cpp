//
// Created by TD on 2024/1/8.
//

#include "import_and_render_ue.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/metadata/task_status.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/core/alembic_file.h>
#include <doodle_lib/core/fbx_file.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/system.hpp>

#include "core/entity_path.h"
#include "http_client/kitsu_client.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace doodle {

namespace import_and_render_ue_ns {

void fix_project(const FSys::path& in_project_path) {
  auto l_json                = nlohmann::json::parse(FSys::ifstream{in_project_path});
  auto&& l_plugin            = l_json["Plugins"];

  static auto l_enabled_plug = [](nlohmann::json& in_json, const std::string& in_plug_name, bool in_enable = true) {
    bool l_has{};
    for (auto&& l_v : in_json) {
      if (l_v.contains("Name") && l_v["Name"] == in_plug_name) {
        l_v["Enabled"] = in_enable;
        l_has          = true;
      }
    }

    if (!l_has) {
      auto&& l_plugin_obj     = in_json.emplace_back(nlohmann::json::object());
      l_plugin_obj["Name"]    = in_plug_name;
      l_plugin_obj["Enabled"] = in_enable;
    }
  };

  l_enabled_plug(l_plugin, "Doodle");
  l_enabled_plug(l_plugin, "MoviePipelineMaskRenderPass");
  l_enabled_plug(l_plugin, "MovieRenderPipeline");
  l_enabled_plug(l_plugin, "UAssetBrowser", false);
  FSys::ofstream{in_project_path} << l_json.dump();
}

void fix_config(const FSys::path& in_project_path) {
  auto l_file_path = in_project_path.parent_path() / "Config" / "DefaultEngine.ini";

  if (!FSys::exists(l_file_path)) {
    FSys::ofstream{l_file_path} << R"(
[/Script/Engine.RendererSettings]
r.TextureStreaming=True
r.GBufferFormat=3
r.AllowStaticLighting=True
r.Streaming.PoolSize=16384
r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject=False
r.Lumen.HardwareRayTracing=False
r.Nanite.ProjectEnabled=True
r.AllowOcclusionQueries=False
r.DefaultFeature.MotionBlur=False
r.CustomDepth=3

[/Script/Engine.Engine]
NearClipPlane=0.500000
")";
    return;
  }

  auto l_file = FSys::ifstream{l_file_path};
  std::string l_str{std::istreambuf_iterator<char>{l_file}, std::istreambuf_iterator<char>{}};

  if (auto l_find_render_setting = l_str.find("[/Script/Engine.RendererSettings]");
      l_find_render_setting == std::string::npos) {
    l_str += R"([/Script/Engine.RendererSettings]
r.TextureStreaming=True
r.GBufferFormat=1
r.AllowStaticLighting=True
r.Streaming.PoolSize=16384
r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject=False
r.Lumen.HardwareRayTracing=False
r.Nanite.ProjectEnabled=True
r.AllowOcclusionQueries=False
r.DefaultFeature.MotionBlur=False
r.CustomDepth=3

)";
    FSys::ofstream{l_file_path} << l_str;
  } else {
    auto l_set_data = [&](const std::string& in_key, const std::string& in_value) {
      auto l_find = l_str.find(in_key);
      if (l_find == std::string::npos) {
        l_str.insert(l_find_render_setting + 34, fmt::format("{}={}\n", in_key, in_value));
      } else {
        l_str.replace(l_find, l_str.find("\n", l_find) - l_find, fmt::format("{}={}", in_key, in_value));
      }
    };
    l_set_data("r.TextureStreaming", "True");
    l_set_data("r.GBufferFormat", "1");
    l_set_data("r.AllowStaticLighting", "True");
    l_set_data("r.Streaming.PoolSize", "16384");
    l_set_data("r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject", "False");
    l_set_data("r.Lumen.HardwareRayTracing", "False");
    l_set_data("r.Nanite.ProjectEnabled", "True");
    l_set_data("r.AllowOcclusionQueries", "False");      // 遮蔽剔除
    l_set_data("r.DefaultFeature.MotionBlur", "False");  // 移动模糊
    l_set_data("r.CustomDepth", "3");                    // 自定义深度 3(启用模板)
  }

  if (auto l_find_engine = l_str.find("[/Script/Engine.Engine]"); l_find_engine == std::string::npos) {
    l_str += R"([/Script/Engine.Engine]
NearClipPlane=0.500000
)";
  } else {
    auto l_find = l_str.find("NearClipPlane");
    if (l_find == std::string::npos) {
      l_str.insert(l_find_engine + 24, fmt::format("{}={}\n", "NearClipPlane", "0.500000"));
    } else {
      l_str.replace(l_find, l_str.find("\n", l_find) - l_find, fmt::format("{}={}", "NearClipPlane", "0.500000"));
    }
  }

  FSys::ofstream{l_file_path} << l_str;
}
}  // namespace import_and_render_ue_ns

std::vector<FSys::path> clean_1001_before_frame(const FSys::path& in_path, std::int32_t in_frame) {
  std::vector<FSys::path> l_move_paths{};
  std::vector<FSys::path> l_remove_paths{};
  if (!FSys::is_directory(in_path)) throw_exception(doodle_error{"没有这样的目录 {}", in_path});

  for (auto&& l_path : FSys::directory_iterator{in_path}) {
    auto l_ext = l_path.path().extension();
    if (l_ext == ".png" || l_ext == ".exr" || l_ext == ".jpg") {
      auto l_stem  = l_path.path().stem().generic_string();
      auto l_begin = l_stem.find('.');

      if (l_begin == std::string::npos) {
        l_remove_paths.emplace_back(l_path.path());
        continue;
      }
      std::int32_t l_frame_num{};
      try {
        auto l_id   = l_stem.substr(l_begin + 1, l_stem.find('.', l_begin + 1) - l_begin - 1);
        l_frame_num = std::stoi(l_id);
      } catch (...) {
        l_remove_paths.emplace_back(l_path.path());
        continue;
      }

      if (l_frame_num < in_frame) {
        l_remove_paths.emplace_back(l_path.path());
        continue;
      }
      l_move_paths.emplace_back(l_path.path());
    }
  }
  if (l_move_paths.empty()) throw_exception(doodle_error{"未扫描到文件"});
  std::error_code l_sys_ec{};
  for (auto&& l_path : l_remove_paths) {
    FSys::remove(l_path, l_sys_ec);
  }
  return l_move_paths;
}

boost::asio::awaitable<void> run_ue_assembly_local::run() {
  SPDLOG_LOGGER_WARN(logger_ptr_, "开始运行组装");
  kitsu_client_->set_logger(logger_ptr_);

  {
    auto l_arg_json = co_await kitsu_client_->get_ue_assembly(project_id_, shot_task_id_);
    l_arg_json.get_to(arg_);
  }
  auto l_g = co_await g_ctx().get<ue_ctx>().queue_->queue(boost::asio::use_awaitable);
  //
  for (auto&& [p_from, p_to] : arg_.ue_asset_path_) {
    FSys::copy_diff(p_from, p_to, logger_ptr_);
  }
  import_and_render_ue_ns::fix_project(arg_.ue_main_project_path_);
  import_and_render_ue_ns::fix_config(arg_.ue_main_project_path_);

  nlohmann::json l_json{};
  l_json          = arg_;
  auto l_tmp_path = FSys::write_tmp_file("ue_import", l_json.dump(), ".json");
  logger_ptr_->warn("排队导入文件 {} ", arg_.ue_main_project_path_);

  {  // 删除导入的文件
    auto l_clear_path = arg_.ue_main_project_path_.parent_path() / arg_.clear_path_;
    if (FSys::exists((l_clear_path))) {
      logger_ptr_->warn("删除导入目录 {}", l_clear_path);
      FSys::remove_all(l_clear_path);
    }
  }

  auto l_time_info = std::make_shared<server_task_info::run_time_info_t>();
  co_await async_run_ue(
      {arg_.ue_main_project_path_.generic_string(), "-windowed", "-log", "-AllowStdOutLogVerbosity", "-ForceLogFlush",
       "-Unattended", "-run=DoodleAutoAnimation", fmt::format("-Params={}", l_tmp_path)},
      logger_ptr_, false, l_time_info
  );
  l_time_info->info_ = "导入文件";
  on_run_time_info_(*l_time_info);

  logger_ptr_->warn("导入文件完成");
  logger_ptr_->warn("排队渲染, 输出目录 {}", arg_.out_file_dir_);
  if (exists(arg_.out_file_dir_)) {
    try {
      FSys::remove_all(arg_.out_file_dir_);
    } catch (const FSys::filesystem_error& err) {
      logger_ptr_->error("渲染删除上次输出错误:{}", err.what());
    }
  }
  l_time_info = std::make_shared<server_task_info::run_time_info_t>();
  /// 重试三次
  for (int i = 0; i < 3; ++i) {
    try {
      co_await async_run_ue(
          {arg_.ue_main_project_path_.generic_string(), arg_.render_map_.generic_string(),
           fmt::format(R"(-DoodleLevelSequence="{}")", arg_.level_sequence_import_),
           fmt::format(R"(-DoodleMoviePipelineConfig="{}")", arg_.movie_pipeline_config_), "-log",
           "-AllowStdOutLogVerbosity", "-ForceLogFlush", "-Unattended"},
          logger_ptr_, false, l_time_info
      );
      break;
    } catch (const doodle_error& err) {
      logger_ptr_->error("渲染失败 第 {} 次错误: {}", i + 1, err.what());
      if (i == 2) throw;
      logger_ptr_->warn("等待5秒后重试渲染");
    }
  }
  l_time_info->info_ = "渲染UE";
  on_run_time_info_(*l_time_info);

  logger_ptr_->warn("完成渲染, 输出目录 {}", arg_.out_file_dir_);

  // 合成视屏
  logger_ptr_->warn("开始合成视屏 :{}", arg_.create_move_path_);
  {
    boost::system::error_code l_ec{};
    auto l_move_paths = clean_1001_before_frame(arg_.out_file_dir_, arg_.begin_time_);
    detail::create_move(
        arg_.create_move_path_, logger_ptr_, movie::image_attr::make_default_attr(l_move_paths), arg_.size_
    );
  }
  for (auto&& p : arg_.update_ue_path_) {
    logger_ptr_->info("复制UE资源文件 from {} to {}", p.from_, p.to_);
    FSys::copy_diff(p.from_, p.to_, logger_ptr_);
  }
  co_await kitsu_client_->comment_task(
      kitsu::kitsu_client::comment_task_arg{
          .task_id_             = shot_task_id_,
          .comment_             = "UE组装和渲染完成, 已上传合成视频",
          .attach_files_        = arg_.create_move_path_,
          .task_status_id_      = task_status::get_completed(),
          .preview_file_source_ = preview_file_source_enum::auto_light_generate,
      }
  );
}

}  // namespace doodle