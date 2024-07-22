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
import_and_render_ue_ns::import_data_t gen_import_config(const import_and_render_ue_ns::args in_args) {
  auto l_maya_out_arg = in_args.maya_out_arg_.out_file_list |
                        ranges::views::filter([](const maya_exe_ns::maya_out_arg::out_file_t& in_arg) {
                          return !in_arg.out_file.empty() && FSys::exists(in_arg.out_file);
                        }) |
                        ranges::to_vector;
  import_and_render_ue_ns::import_data_t l_import_data;
  l_import_data.episode    = in_args.episodes_;
  l_import_data.shot       = in_args.shot_;
  l_import_data.begin_time = in_args.maya_out_arg_.begin_time;
  l_import_data.end_time   = in_args.maya_out_arg_.end_time;

  l_import_data.project_     = in_args.project_;
  l_import_data.out_file_dir =
      in_args.down_info_.render_project_.parent_path() / doodle_config::ue4_saved / doodle_config::ue4_movie_renders /
      fmt::format(
        "{}_Ep{:04}_sc{:04}{}", l_import_data.project_.p_shor_str, l_import_data.episode.p_episodes,
        l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum
      );

  // 渲染配置
  {
    l_import_data.movie_pipeline_config = fmt::format(
      "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/{0}_EP{1:03}_SC{2:03}{3}_Config",
      l_import_data.project_.p_shor_str, l_import_data.episode.p_episodes,
      l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum
    );
    l_import_data.movie_pipeline_config.replace_extension(l_import_data.movie_pipeline_config.stem());

    l_import_data.level_sequence_import = fmt::format(
      "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/{0}_EP{1:03}_SC{2:03}{3}", l_import_data.project_.p_shor_str,
      l_import_data.episode.p_episodes, l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum
    );
    l_import_data.level_sequence_vfx = l_import_data.level_sequence_import.generic_string() + "_Vfx";
    l_import_data.level_sequence_import.replace_extension(l_import_data.level_sequence_import.stem());

    l_import_data.create_map = fmt::format(
      "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/{0}_EP{1:03}_SC{2:03}{3}_LV",
      l_import_data.project_.p_shor_str, l_import_data.episode.p_episodes,
      l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum
    );
    l_import_data.vfx_map = l_import_data.create_map + "_Vfx_LV";

    l_import_data.import_dir = fmt::format(
      "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/Import_{4:%m_%d_%H_%M}", l_import_data.project_.p_shor_str,
      l_import_data.episode.p_episodes, l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum,
      time_point_wrap{}.get_local_time()
    );

    l_import_data.render_map = fmt::format(
      "/Game/Shot/ep{0:04}/sc{1:03}{2}", l_import_data.episode.p_episodes, l_import_data.shot.p_shot,
      l_import_data.shot.p_shot_enum
    );
  }

  l_import_data.files = l_maya_out_arg |
                        ranges::views::transform([](const maya_exe_ns::maya_out_arg::out_file_t& in_arg) {
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
                          return import_and_render_ue_ns::import_files_t{l_type, in_arg.out_file};
                        }) |
                        ranges::to_vector;
  l_import_data.original_map = in_args.down_info_.scene_file_.generic_string();
  return l_import_data;
}

void fix_project(const import_and_render_ue_ns::args& in_args) {
  auto l_json     = nlohmann::json::parse(FSys::ifstream{in_args.down_info_.render_project_});
  auto&& l_plugin = l_json["Plugins"];
  l_plugin.clear();
  auto&& l_plugin_obj     = l_plugin.emplace_back(nlohmann::json::object());
  l_plugin_obj["Name"]    = "Doodle";
  l_plugin_obj["Enabled"] = true;
  FSys::ofstream{in_args.down_info_.render_project_} << l_json.dump();
}

void fix_config(const import_and_render_ue_ns::args& in_args) {
  auto l_file_path = in_args.down_info_.render_project_.parent_path() / "Config" / "DefaultEngine.ini";

  if (!FSys::exists(l_file_path)) {
    FSys::ofstream{l_file_path} << R"(
[/Script/Engine.RendererSettings]
r.TextureStreaming=True
r.GBufferFormat=3
r.AllowStaticLighting=True
r.Streaming.PoolSize=16384
r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject=False
r.Lumen.HardwareRayTracing=False
r.Nanite.ProjectEnabled=False

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
r.Nanite.ProjectEnabled=False
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
    l_set_data("r.Nanite.ProjectEnabled", "False");
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

struct association_data {
  boost::uuids::uuid id_{};
  FSys::path maya_file_{};
  FSys::path ue_file_{};
  details::assets_type_enum type_{};
  FSys::path ue_prj_path_{};
  FSys::path export_file_{};
};


std::vector<down_auto_light_anim_file::association_data> fetch_association_data(
  const std::vector<association_data>& in_uuid, boost::system::error_code& out_error_code
) {
  std::vector<down_auto_light_anim_file::association_data> l_out{};
  boost::beast::tcp_stream l_stream{g_io_context()};

  try {
    l_stream.connect(boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string("192.168.40.181"), 50026});
    for (auto&& i : in_uuid) {
      boost::beast::http::request<boost::beast::http::empty_body> l_req{
          boost::beast::http::verb::get, fmt::format("/api/doodle/file_association/{}", i.id_), 11
      };
      l_req.set(boost::beast::http::field::host, "192.168.40.181:50026");
      l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      l_req.set(boost::beast::http::field::accept, "application/json");
      l_req.prepare_payload();
      boost::beast::http::write(l_stream, l_req);
      boost::beast::flat_buffer l_buffer{};
      boost::beast::http::response<boost::beast::http::string_body> l_res;
      boost::beast::http::read(l_stream, l_buffer, l_res);
      if (l_res.result() != boost::beast::http::status::ok) {
        if (l_res.result() == boost::beast::http::status::not_found) {
          data_->logger_->log(log_loc(), level::err, "未找到关联数据:{} {}", i.export_file_, i.id_);
          out_error_code = boost::system::error_code{
              boost::system::errc::no_such_file_or_directory, boost::system::generic_category()
          };
          return l_out;
        }
        continue;
      }

      auto l_json = nlohmann::json::parse(l_res.body());

      association_data l_data{
          .id_ = i.id_,
          .maya_file_ = l_json.at("maya_file").get<std::string>(),
          .ue_file_ = l_json.at("ue_file").get<std::string>(),
          .type_ = l_json.at("type").get<details::assets_type_enum>(),
      };
      l_out.emplace_back(std::move(l_data));
    }
  } catch (const std::exception& e) {
    data_->logger_->log(log_loc(), level::err, "连接服务器失败:{}", e.what());
    out_error_code =
        boost::system::error_code{boost::system::errc::connection_refused, boost::system::generic_category()};
  }
  for (auto&& i : l_out) {
    i.ue_prj_path_ = ue_main_map::find_ue_project_file(i.ue_file_);
  }
  return l_out;
}


boost::asio::awaitable<std::tuple<boost::system::error_code, down_auto_light_anim_file::down_info>> analysis_out_file(
  import_and_render_ue_ns::args in_args, logger_ptr in_logger) {
  std::vector<association_data> l_refs_tmp{};

  for (auto&& i : in_args.maya_out_arg_.out_file_list) {
    if (!FSys::exists(i.ref_file)) continue;
    in_logger->warn("引用文件:{}", i.ref_file);
    auto l_uuid = FSys::software_flag_file(i.ref_file);
    if (l_uuid.is_nil()) continue;

    l_refs_tmp.emplace_back(association_data{.id_ = l_uuid, .export_file_ = i.ref_file});
  }

  std::sort(l_refs_tmp.begin(), l_refs_tmp.end(), [](const auto& l, const auto& r) { return l.id_ < r.id_; });
  l_refs_tmp.erase(
    std::unique(l_refs_tmp.begin(), l_refs_tmp.end(), [](const auto& l, const auto& r) { return l.id_ == r.id_; }),
    l_refs_tmp.end()
  );
  auto l_refs = fetch_association_data(l_refs_tmp, in_error_code);
  if (in_error_code) {
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }

  // 检查文件

  auto l_scene_uuid = boost::uuids::nil_uuid();
  FSys::path l_down_path_file_name{};

  for (auto&& h : l_refs) {
    if (auto l_is_e = h.ue_file_.empty(), l_is_ex = FSys::exists(h.ue_file_); l_is_e || !l_is_ex) {
      if (l_is_e)
        data_->logger_->log(log_loc(), level::level_enum::err, "文件 {} 的 ue 引用无效, 为空", h.maya_file_);
      else if (!l_is_ex)
        data_->logger_->log(log_loc(), level::level_enum::err, "文件 {} 的 ue {} 引用不存在", h.maya_file_, h.ue_file_);

      in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
      BOOST_ASIO_ERROR_LOCATION(in_error_code);
      wait_op_->ec_ = in_error_code;
      wait_op_->complete();
      return;
    }

    if (h.type_ == details::assets_type_enum::scene) {
      l_scene_uuid          = h.id_;
      l_down_path_file_name = h.ue_prj_path_.parent_path().filename();
    }
  }
  if (l_scene_uuid.is_nil()) {
    data_->logger_->log(log_loc(), level::level_enum::err, "未查找到主项目文件(没有找到场景文件)");
    in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }

  static auto g_root{FSys::path{"D:/doodle/cache/ue"}};
  std::vector<std::pair<FSys::path, FSys::path>> l_copy_path{};

  for (auto&& h : l_refs) {
    auto l_down_path  = h.ue_prj_path_.parent_path();
    auto l_root       = h.ue_prj_path_.parent_path() / doodle_config::ue4_content;
    auto l_local_path = g_root / in_args.project_.p_shor_str / l_down_path_file_name;

    switch (h.type_) {
      // 场景文件
      case details::assets_type_enum::scene: {
        auto l_original               = h.ue_file_.lexically_relative(l_root);
        data_->down_info_.scene_file_ =
            fmt::format("/Game/{}/{}", l_original.parent_path().generic_string(), l_original.stem());

        l_copy_path.emplace_back(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content);
        // 配置文件夹复制
        l_copy_path.emplace_back(l_down_path / doodle_config::ue4_config, l_local_path / doodle_config::ue4_config);
        // 复制项目文件
        if (!FSys::exists(l_local_path / h.ue_prj_path_.filename()))
          l_copy_path.emplace_back(h.ue_prj_path_, l_local_path / h.ue_prj_path_.filename());
        data_->down_info_.render_project_ = l_local_path / h.ue_prj_path_.filename();
      }
      break;

      // 角色文件
      case details::assets_type_enum::character: {
        l_copy_path.emplace_back(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content);
      }
      break;

      // 道具文件
      case details::assets_type_enum::prop: {
        auto l_prop_path = h.ue_file_.lexically_relative(l_root / "Prop");
        if (l_prop_path.empty()) continue;
        auto l_prop_path_name = *l_prop_path.begin();
        l_copy_path.emplace_back(
          l_down_path / doodle_config::ue4_content / "Prop" / l_prop_path_name,
          l_local_path / doodle_config::ue4_content / "Prop" / l_prop_path_name
        );
      }
      break;

      default:
        break;
    }
  }
  data_->logger_->log(log_loc(), level::off, magic_enum::enum_name(process_message::state::pause));
  g_ctx().get<ue_exe_ptr>()->async_copy_old_project(
    msg_, l_copy_path, boost::asio::bind_executor(g_io_context(), *this)
  );
}


boost::asio::awaitable<std::tuple<boost::system::error_code, FSys::path>> async_import_and_render_ue(
  import_and_render_ue_ns::args in_args, logger_ptr in_logger
) {
  // 先下载文件

  // 导入文件
  in_logger->warn("开始导入文件 {} ", in_args.down_info_.render_project_);
  fix_config(in_args);
  fix_project(in_args);
  auto l_import_data = gen_import_config(in_args);
  nlohmann::json l_json{};
  l_json          = l_import_data;
  auto l_tmp_path = FSys::write_tmp_file("ue_import", l_json.dump(), ".json");
  auto l_ec1      = co_await async_run_ue(fmt::format(
                                            "{} -windowed -log -stdout -AllowStdOutLogVerbosity -ForceLogFlush -Unattended -run=DoodleAutoAnimation  -Params={}",
                                            in_args.down_info_.render_project_, l_tmp_path),
                                          in_logger);
  if (l_ec1) co_return std::tuple(l_ec1, FSys::path{});

  in_logger->warn("渲染开始, 输出目录 {}", l_import_data.out_file_dir);
  if (exists(l_import_data.out_file_dir)) {
    try {
      FSys::remove_all(l_import_data.out_file_dir);
    } catch (FSys::filesystem_error& err) {
      in_logger->error("渲染删除上次输出错误 error:{}", err.what());
    }
  }
  l_ec1 = co_await async_run_ue(fmt::format(
                                  R"({} {} -game -LevelSequence="{}" -MoviePipelineConfig="{}" -windowed -log -stdout -AllowStdOutLogVerbosity -ForceLogFlush -Unattended)",
                                  in_args.down_info_.render_project_, l_import_data.render_map,
                                  l_import_data.level_sequence_import, l_import_data.movie_pipeline_config),
                                in_logger);
  in_logger->warn("完成渲染, 输出目录 {}", l_import_data.out_file_dir);
}
} // namespace doodle