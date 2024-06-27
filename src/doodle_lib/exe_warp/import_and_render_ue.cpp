//
// Created by TD on 2024/1/8.
//

#include "import_and_render_ue.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
namespace doodle {

void import_and_render_ue::init() {
  if (!g_ctx().contains<ue_exe_ptr>()) g_ctx().emplace<ue_exe_ptr>(std::make_shared<ue_exe>());
  data_->logger_ = msg_.get<process_message>().logger();
}
FSys::path import_and_render_ue::gen_import_config() const {
  auto l_maya_out     = msg_.get<maya_exe_ns::maya_out_arg>();
  auto l_maya_out_arg = l_maya_out.out_file_list |
                        ranges::views::filter([](const maya_exe_ns::maya_out_arg::out_file_t &in_arg) {
                          return !in_arg.out_file.empty() && FSys::exists(in_arg.out_file);
                        }) |
                        ranges::to_vector;

  data_->import_data_.episode    = msg_.get<episodes>();
  data_->import_data_.shot       = msg_.get<shot>();
  data_->import_data_.begin_time = l_maya_out.begin_time;
  data_->import_data_.end_time   = l_maya_out.end_time;

  data_->import_data_.project_   = msg_.get<project>();
  data_->import_data_.out_file_dir =
      data_->down_info_.render_project_.parent_path() / doodle_config::ue4_saved / doodle_config::ue4_movie_renders /
      fmt::format(
          "{}_Ep{:04}_sc{:04}{}", data_->import_data_.project_.p_shor_str, data_->import_data_.episode.p_episodes,
          data_->import_data_.shot.p_shot, data_->import_data_.shot.p_shot_enum
      );

  // 渲染配置
  {
    data_->import_data_.movie_pipeline_config = fmt::format(
        "/Game/Shot/ep{1:04}/{0}{1:04}_sc{2:04}{3}/{0}_EP{1:04}_SC{2:04}{3}_Config",
        data_->import_data_.project_.p_shor_str, data_->import_data_.episode.p_episodes,
        data_->import_data_.shot.p_shot, data_->import_data_.shot.p_shot_enum
    );
    data_->import_data_.movie_pipeline_config.replace_extension(data_->import_data_.movie_pipeline_config.stem());

    data_->import_data_.level_sequence_import = fmt::format(
        "/Game/Shot/ep{1:04}/{0}{1:04}_sc{2:04}{3}/{0}_EP{1:04}_SC{2:04}{3}", data_->import_data_.project_.p_shor_str,
        data_->import_data_.episode.p_episodes, data_->import_data_.shot.p_shot, data_->import_data_.shot.p_shot_enum
    );
    data_->import_data_.level_sequence_vfx = data_->import_data_.level_sequence_import.generic_string() + "_Vfx";
    data_->import_data_.level_sequence_import.replace_extension(data_->import_data_.level_sequence_import.stem());

    data_->import_data_.create_map = fmt::format(
        "/Game/Shot/ep{1:04}/{0}{1:04}_sc{2:04}{3}/{0}_EP{1:04}_SC{2:04}{3}_LV",
        data_->import_data_.project_.p_shor_str, data_->import_data_.episode.p_episodes,
        data_->import_data_.shot.p_shot, data_->import_data_.shot.p_shot_enum
    );
    data_->import_data_.vfx_map    = data_->import_data_.create_map + "_Vfx_LV";

    data_->import_data_.import_dir = fmt::format(
        "/Game/Shot/ep{1:04}/{0}{1:04}_sc{2:04}{3}/Import_{4:%m_%d_%H_%M}", data_->import_data_.project_.p_shor_str,
        data_->import_data_.episode.p_episodes, data_->import_data_.shot.p_shot, data_->import_data_.shot.p_shot_enum,
        time_point_wrap{}.get_local_time()
    );

    data_->import_data_.render_map = fmt::format(
        "/Game/Shot/ep{0:04}/sc{1:04}{2}", data_->import_data_.episode.p_episodes, data_->import_data_.shot.p_shot,
        data_->import_data_.shot.p_shot_enum
    );
  }

  data_->import_data_.files = l_maya_out_arg |
                              ranges::views::transform([](const maya_exe_ns::maya_out_arg::out_file_t &in_arg) {
                                auto l_file_name_str = in_arg.out_file.filename().generic_string();
                                auto l_ext           = in_arg.out_file.extension().generic_string();
                                std::string l_type{};
                                if (l_file_name_str.find("_camera_") != std::string::npos) {
                                  l_type = "cam";
                                } else if (l_ext == ".abc") {
                                  l_type = "geo";
                                } else if (l_ext == ".fbx") {
                                  l_type = "char";
                                }
                                return import_and_render_ue::import_files_t{l_type, in_arg.out_file};
                              }) |
                              ranges::to_vector;
  data_->import_data_.original_map = data_->down_info_.scene_file_.generic_string();

  nlohmann::json l_json{};
  l_json          = data_->import_data_;
  auto l_tmp_path = FSys::get_cache_path(FSys::path{"ue_import"} / version::build_info::get().version_str) /
                    (core_set::get_set().get_uuid_str() + ".json");
  FSys::ofstream{l_tmp_path} << l_json.dump();
  return l_tmp_path;
}
void import_and_render_ue::fix_project() const {
  auto l_json     = nlohmann::json::parse(FSys::ifstream{data_->down_info_.render_project_});
  auto &&l_plugin = l_json["Plugins"];
  l_plugin.clear();
  auto &&l_plugin_obj     = l_plugin.emplace_back(nlohmann::json::object());
  l_plugin_obj["Name"]    = "Doodle";
  l_plugin_obj["Enabled"] = true;
  FSys::ofstream{data_->down_info_.render_project_} << l_json.dump();
}
void import_and_render_ue::fix_config() const {
  auto l_file_path = data_->down_info_.render_project_.parent_path() / "Config" / "DefaultEngine.ini";

  if (!FSys::exists(l_file_path)) {
    FSys::ofstream{l_file_path} << R"(
[/Script/Engine.RendererSettings]
r.TextureStreaming=True
r.GBufferFormat=3
r.AllowStaticLighting=False
r.Streaming.PoolSize=16384
r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject=False
r.Lumen.HardwareRayTracing=False
r.Nanite.ProjectEnabled=False
")";
    return;
  }

  auto l_file = FSys::ifstream{l_file_path};
  std::string l_str{std::istreambuf_iterator<char>{l_file}, std::istreambuf_iterator<char>{}};
  auto l_find_render_setting = l_str.find("[/Script/Engine.RendererSettings]");
  if (l_find_render_setting == std::string::npos) {
    l_str += R"([/Script/Engine.RendererSettings]
r.TextureStreaming=True
r.GBufferFormat=1
r.AllowStaticLighting=False
r.Streaming.PoolSize=16384
r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject=False
r.Lumen.HardwareRayTracing=False
r.Nanite.ProjectEnabled=False
)";
    FSys::ofstream{l_file_path} << l_str;
    return;
  }

  auto l_set_data = [&](const std::string &in_key, const std::string &in_value) {
    auto l_find = l_str.find(in_key);
    if (l_find == std::string::npos) {
      l_str.insert(l_find_render_setting + 34, fmt::format("{}={}\n", in_key, in_value));
    } else {
      l_str.replace(l_find, l_str.find("\n", l_find) - l_find, fmt::format("{}={}", in_key, in_value));
    }
  };
  l_set_data("r.TextureStreaming", "True");
  l_set_data("r.GBufferFormat", "1");
  l_set_data("r.AllowStaticLighting", "False");
  l_set_data("r.Streaming.PoolSize", "16384");
  l_set_data("r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject", "False");
  l_set_data("r.Lumen.HardwareRayTracing", "False");
  l_set_data("r.Nanite.ProjectEnabled", "False");

  FSys::ofstream{l_file_path} << l_str;
}

void import_and_render_ue::operator()(
    boost::system::error_code in_error_code, down_auto_light_anim_file::down_info in_down_info
) const {
  if (!data_->logger_) {
    default_logger_raw()->log(log_loc(), level::level_enum::err, "缺失组建错误 缺失日志组件");
    in_error_code.assign(error_enum::component_missing_error, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  if (in_error_code) {
    data_->logger_->log(log_loc(), level::level_enum::err, "导入ue错误 error:{}", in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }

  data_->down_info_ = in_down_info;
  data_->logger_->log(log_loc(), level::level_enum::warn, "开始导入文件 {} ", data_->down_info_.render_project_);
  fix_project();
  fix_config();
  g_ctx().get<ue_exe_ptr>()->async_run(
      msg_,
      fmt::format("{} -run=DoodleAutoAnimation -Params={}", data_->down_info_.render_project_, gen_import_config()),
      boost::asio::bind_executor(g_io_context(), std::move(*this))
  );
}

void import_and_render_ue::operator()(boost::system::error_code in_error_code) const {
  if (in_error_code) {
    data_->logger_->log(log_loc(), level::level_enum::err, "渲染错误 error:{}", in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  switch (data_->status_) {
    case status::import_file: {
      data_->logger_->log(
          log_loc(), level::level_enum::warn, "渲染开始, 输出目录 {}", data_->import_data_.out_file_dir
      );
      if (FSys::exists(data_->import_data_.out_file_dir)) {
        try {
          FSys::remove_all(data_->import_data_.out_file_dir);
        } catch (FSys::filesystem_error &err) {
          data_->logger_->log(log_loc(), level::level_enum::err, "渲染删除上次输出错误 error:{}", err.what());
        }
      }
      data_->status_ = status::render;
      g_ctx().get<ue_exe_ptr>()->async_run(
          msg_,
          fmt::format(
              R"({} {} -game -LevelSequence="{}" -MoviePipelineConfig="{}" -windowed -log -stdout -AllowStdOutLogVerbosity -ForceLogFlush -Unattended)",
              data_->down_info_.render_project_, data_->import_data_.render_map,
              data_->import_data_.level_sequence_import, data_->import_data_.movie_pipeline_config
          ),
          boost::asio::bind_executor(g_io_context(), std::move(*this))
      );
    } break;
    case status::render: {
      data_->logger_->log(
          log_loc(), level::level_enum::warn, "完成渲染, 输出目录 {}", data_->import_data_.out_file_dir
      );
      set_out_file_dir_(data_->import_data_.out_file_dir);
      wait_op_->ec_ = in_error_code;
      wait_op_->complete();
      break;
    }
  }
}

}  // namespace doodle