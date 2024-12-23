//
// Created by TD on 2024/1/8.
//

#include "import_and_render_ue.h"

#include <doodle_core/core/http_client_core.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/core/scan_assets/base.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/system.hpp>

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

  l_import_data.project_   = in_args.project_;
  l_import_data.out_file_dir =
      in_args.down_info_.render_project_.parent_path() / doodle_config::ue4_saved / doodle_config::ue4_movie_renders /
      fmt::format(
          "{}_EP{:03}_SC{:03}{}", l_import_data.project_.code_, l_import_data.episode.p_episodes,
          l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum
      );

  // 渲染配置
  {
    l_import_data.movie_pipeline_config = fmt::format(
        "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/{0}_EP{1:03}_SC{2:03}{3}_Config", l_import_data.project_.code_,
        l_import_data.episode.p_episodes, l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum
    );
    l_import_data.movie_pipeline_config.replace_extension(l_import_data.movie_pipeline_config.stem());

    l_import_data.level_sequence_import = fmt::format(
        "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/{0}_EP{1:03}_SC{2:03}{3}", l_import_data.project_.code_,
        l_import_data.episode.p_episodes, l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum
    );
    l_import_data.level_sequence_vfx = l_import_data.level_sequence_import.generic_string() + "_Vfx";
    l_import_data.level_sequence_import.replace_extension(l_import_data.level_sequence_import.stem());

    l_import_data.create_map = fmt::format(
        "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/{0}_EP{1:03}_SC{2:03}{3}_LV", l_import_data.project_.code_,
        l_import_data.episode.p_episodes, l_import_data.shot.p_shot, l_import_data.shot.p_shot_enum
    );
    l_import_data.vfx_map    = l_import_data.create_map + "_Vfx_LV";

    l_import_data.import_dir = fmt::format(
        "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/Import_{4:%m_%d_%H_%M}", l_import_data.project_.code_,
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
namespace import_and_render_ue_ns {
void fix_project(const FSys::path& in_project_path) {
  auto l_json                = nlohmann::json::parse(FSys::ifstream{in_project_path});
  auto&& l_plugin            = l_json["Plugins"];

  static auto l_enabled_plug = [](nlohmann::json& in_json, const std::string& in_plug_name) {
    bool l_has{};
    for (auto&& l_v : in_json) {
      if (l_v.contains("Name") && l_v["Name"] == in_plug_name) {
        l_v["Enabled"] = true;
        l_has          = true;
      }
    }

    if (!l_has) {
      auto&& l_plugin_obj     = in_json.emplace_back(nlohmann::json::object());
      l_plugin_obj["Name"]    = in_plug_name;
      l_plugin_obj["Enabled"] = true;
    }
  };

  l_enabled_plug(l_plugin, "Doodle");
  l_enabled_plug(l_plugin, "MoviePipelineMaskRenderPass");
  l_enabled_plug(l_plugin, "MovieRenderPipeline");
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
r.DefaultFeature.AutoExposure=False
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
r.DefaultFeature.AutoExposure=False
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
    l_set_data("r.DefaultFeature.AutoExposure", "False");  // 自动曝光
    l_set_data("r.AllowOcclusionQueries", "False");        // 遮蔽剔除
    l_set_data("r.DefaultFeature.MotionBlur", "False");    // 移动模糊
    l_set_data("r.CustomDepth", "3");                      // 自定义深度 3(启用模板)
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

boost::asio::awaitable<std::tuple<boost::system::error_code, std::vector<association_data>>> fetch_association_data(
    std::vector<boost::uuids::uuid> in_uuid, logger_ptr in_logger
) {
  std::vector<association_data> l_out{};
  boost::beast::tcp_stream l_stream{g_io_context()};
  auto l_c = std::make_shared<http::detail::http_client_data_base>(co_await boost::asio::this_coro::executor);
  l_c->init(core_set::get_set().server_ip);
  try {
    for (auto&& i : in_uuid) {
      boost::beast::http::request<boost::beast::http::empty_body> l_req{
          boost::beast::http::verb::get, fmt::format("/api/doodle/file_association/{}", i), 11
      };
      l_req.set(boost::beast::http::field::host, "192.168.40.182");
      l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      l_req.set(boost::beast::http::field::accept, "application/json");
      l_req.prepare_payload();
      auto [l_ec, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(l_c, l_req);

      if (l_res.result() != boost::beast::http::status::ok) {
        in_logger->log(log_loc(), level::warn, "未找到关联数据: {}", i);
        l_out.emplace_back(association_data{.id_ = i});
        co_return std::tuple{
            boost::system::error_code{
                boost::system::errc::no_such_file_or_directory, boost::system::generic_category()
            },
            l_out
        };
      }

      auto l_json = nlohmann::json::parse(l_res.body());
      association_data l_data{
          .id_        = i,
          .maya_file_ = l_json.at("maya_file").get<std::string>(),
          .ue_file_   = l_json.at("ue_file").get<std::string>(),
          .type_      = l_json.at("type").get<details::assets_type_enum>()
      };
      l_out.emplace_back(std::move(l_data));
    }
  } catch (const std::exception& e) {
    in_logger->error("连接服务器失败:{}", e.what());
    co_return std::tuple{
        boost::system::error_code{boost::system::errc::connection_refused, boost::system::generic_category()}, l_out
    };
  }
  for (auto&& i : l_out) {
    i.ue_prj_path_ = ue_exe_ns::find_ue_project_file(i.ue_file_);
  }
  co_return std::tuple{boost::system::error_code{}, l_out};
}

void copy_diff_impl(const FSys::path& from, const FSys::path& to) {
  if (!FSys::exists(to) || FSys::file_size(from) != FSys::file_size(to) ||
      FSys::last_write_time(from) != FSys::last_write_time(to)) {
    if (!FSys::exists(to) || FSys::last_write_time(from) > FSys::last_write_time(to)) {
      if (!FSys::exists(to.parent_path())) FSys::create_directories(to.parent_path());

      auto l_to   = to;
      auto l_from = from;
      static FSys::path l_path{R"(\\?\)"};
      if (l_to.make_preferred().native().size() > MAX_PATH) {
        l_to = l_path / l_to;
      }
      if (l_from.make_preferred().native().size() > MAX_PATH) {
        l_from = l_path / l_from;
      }

      FSys::copy_file(l_from, l_to, FSys::copy_options::overwrite_existing);
    }
  }
}

boost::system::error_code copy_diff(const FSys::path& from, const FSys::path& to, logger_ptr in_logger) {
  boost::system::error_code l_ec{};
  std::error_code l_error_code_NETNAME_DELETED{ERROR_NETNAME_DELETED, std::system_category()};
  for (int i = 0; i < 10; ++i) {
    try {
      in_logger->warn("复制 {} -> {}", from, to);
      if (FSys::is_regular_file(from) && !FSys::is_hidden(from) &&
          from.extension() != doodle_config::doodle_flag_name) {
        copy_diff_impl(from, to);
        return l_ec;
      }

      for (auto&& l_file : FSys::recursive_directory_iterator(from)) {
        auto l_to_file = to / l_file.path().lexically_proximate(from);
        if (l_file.is_regular_file() && !FSys::is_hidden(l_file.path()) &&
            l_file.path().extension() != doodle_config::doodle_flag_name) {
          copy_diff_impl(l_file.path(), l_to_file);
        }
      }

      return l_ec;
    } catch (const FSys::filesystem_error& in_error) {
      if (in_error.code() == l_error_code_NETNAME_DELETED) {
        in_logger->log(log_loc(), spdlog::level::warn, "复制文件网络错误 开始重试第 {}次 {}, ", i++, in_error.what());
      } else {
        in_logger->log(log_loc(), spdlog::level::err, "复制文件错误 {}", in_error.what());
        l_ec = in_error.code();
        BOOST_ASIO_ERROR_LOCATION(l_ec);
        break;
      }
    } catch (const std::system_error& in_error) {
      in_logger->log(log_loc(), spdlog::level::err, in_error.what());
      l_ec = in_error.code();
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      break;
    } catch (...) {
      in_logger->log(log_loc(), spdlog::level::err, "未知错误 {}", boost::current_exception_diagnostic_information());
      l_ec = boost::system::errc::make_error_code(boost::system::errc::io_error);
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      break;
    }
  }

  return l_ec;
}

boost::asio::awaitable<std::tuple<boost::system::error_code, import_and_render_ue_ns::down_info>> analysis_out_file(
    std::shared_ptr<import_and_render_ue_ns::args> in_args, logger_ptr in_logger
) {
  std::vector<boost::uuids::uuid> l_uuids_tmp{};
  std::map<boost::uuids::uuid, FSys::path> l_refs_tmp{};
  import_and_render_ue_ns::down_info l_out{};

  for (auto&& i : in_args->maya_out_arg_.out_file_list) {
    if (!FSys::exists(i.ref_file)) continue;
    in_logger->warn("引用文件:{}", i.ref_file);
    auto l_uuid = FSys::software_flag_file(i.ref_file);
    if (l_uuid.is_nil()) {
      in_logger->error("获取引用文件失败 {}", i.ref_file);
      co_return std::tuple<boost::system::error_code, import_and_render_ue_ns::down_info>{
          boost::system::errc::make_error_code(boost::system::errc::not_supported), import_and_render_ue_ns::down_info{}
      };
    }

    l_uuids_tmp.push_back(l_uuid);
    l_refs_tmp.emplace(l_uuid, i.ref_file);
  }

  std::sort(l_uuids_tmp.begin(), l_uuids_tmp.end(), [](const auto& l, const auto& r) { return l < r; });
  l_uuids_tmp.erase(
      std::unique(l_uuids_tmp.begin(), l_uuids_tmp.end(), [](const auto& l, const auto& r) { return l == r; }),
      l_uuids_tmp.end()
  );
  auto [l_ec, l_refs] = co_await fetch_association_data(l_uuids_tmp, in_logger);
  if (l_ec) {
    in_logger->error("获取引用文件失败 {} {}", l_refs_tmp[l_refs.back().id_], l_refs.back().id_);
    co_return std::tuple{l_ec, import_and_render_ue_ns::down_info{}};
  }

  // 检查文件

  auto l_scene_uuid = boost::uuids::nil_uuid();
  FSys::path l_down_path_file_name{};

  for (auto&& h : l_refs) {
    if (auto l_is_e = h.ue_file_.empty(), l_is_ex = FSys::exists(h.ue_file_); l_is_e || !l_is_ex) {
      if (l_is_e)
        in_logger->error("文件 {} 的 ue 引用无效, 为空", h.maya_file_);
      else if (!l_is_ex)
        in_logger->error("文件 {} 的 ue {} 引用不存在", h.maya_file_, h.ue_file_);
      co_return std::tuple{
          boost::system::error_code{error_enum::file_not_exists, doodle_category::get()},
          import_and_render_ue_ns::down_info{}
      };
    }

    if (h.type_ == details::assets_type_enum::scene) {
      l_scene_uuid          = h.id_;
      l_down_path_file_name = h.ue_prj_path_.parent_path().filename();
    }
  }
  if (l_scene_uuid.is_nil()) {
    in_logger->error("未查找到主项目文件(没有找到场景文件)");
    co_return std::tuple{
        boost::system::error_code{error_enum::file_not_exists, doodle_category::get()},
        import_and_render_ue_ns::down_info{}
    };
  }

  static auto g_root{FSys::path{doodle_config::g_cache_path}};
  std::vector<std::pair<FSys::path, FSys::path>> l_copy_path{};
  in_logger->warn("排队复制文件");
  // 开始复制文件
  // 先获取UE线程(只能在单线程复制, 要不然会出现边渲染边复制的情况, 会出错)
  auto l_g = co_await g_ctx().get<ue_ctx>().queue_->queue(boost::asio::use_awaitable);
  for (auto&& h : l_refs) {
    auto l_down_path  = h.ue_prj_path_.parent_path();
    auto l_root       = h.ue_prj_path_.parent_path() / doodle_config::ue4_content;
    auto l_local_path = g_root / in_args->project_.code_ / l_down_path_file_name;
    if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
      in_logger->error(" 用户取消操作");
      co_return std::tuple(
          boost::system::error_code{boost::asio::error::operation_aborted}, import_and_render_ue_ns::down_info{}
      );
    }
    switch (h.type_) {
      // 场景文件
      case details::assets_type_enum::scene: {
        auto l_original   = h.ue_file_.lexically_relative(l_root);
        l_out.scene_file_ = fmt::format("/Game/{}/{}", l_original.parent_path().generic_string(), l_original.stem());
        auto l_ec_copy =
            copy_diff(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content, in_logger);
        if (l_ec_copy) co_return std::tuple{l_ec_copy, import_and_render_ue_ns::down_info{}};
        // 配置文件夹复制
        l_ec_copy =
            copy_diff(l_down_path / doodle_config::ue4_config, l_local_path / doodle_config::ue4_config, in_logger);
        if (l_ec_copy) co_return std::tuple{l_ec_copy, import_and_render_ue_ns::down_info{}};
        // 复制项目文件
        if (!FSys::exists(l_local_path / h.ue_prj_path_.filename())) {
          l_ec_copy = copy_diff(h.ue_prj_path_, l_local_path / h.ue_prj_path_.filename(), in_logger);
          if (l_ec_copy) co_return std::tuple{l_ec_copy, import_and_render_ue_ns::down_info{}};
        }
        l_out.render_project_ = l_local_path / h.ue_prj_path_.filename();
      } break;

      // 角色文件
      case details::assets_type_enum::character: {
        auto l_ec_copy =
            copy_diff(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content, in_logger);
        if (l_ec_copy) co_return std::tuple{l_ec_copy, import_and_render_ue_ns::down_info{}};
      } break;

      // 道具文件
      case details::assets_type_enum::prop: {
        auto l_prop_path = h.ue_file_.lexically_relative(l_root / "Prop");
        if (l_prop_path.empty()) continue;
        auto l_prop_path_name = *l_prop_path.begin();
        auto l_ec_copy        = copy_diff(
            l_down_path / doodle_config::ue4_content / "Prop" / l_prop_path_name,
            l_local_path / doodle_config::ue4_content / "Prop" / l_prop_path_name, in_logger
        );
        if (l_ec_copy) co_return std::tuple{l_ec_copy, import_and_render_ue_ns::down_info{}};
        l_ec_copy = copy_diff(
            l_down_path / doodle_config::ue4_content / "Prop" / "a_PropPublicFiles",
            l_local_path / doodle_config::ue4_content / "Prop" / "a_PropPublicFiles", in_logger
        );
        // 这个错误可以忽略, 有错误的情况下, 将状态设置为运行
      } break;
      default:
        break;
    }
  }
  co_return std::tuple{boost::system::error_code{}, l_out};
}

boost::asio::awaitable<std::tuple<boost::system::error_code, FSys::path>> async_import_and_render_ue(
    std::shared_ptr<import_and_render_ue_ns::args> in_args, logger_ptr in_logger
) {
  // 先下载文件
  {
    auto [l_ec, l_down_info] = co_await analysis_out_file(in_args, in_logger);
    if (l_ec) co_return std::tuple(l_ec, FSys::path{});
    in_args->down_info_ = l_down_info;
  }

  // 导入文件
  import_and_render_ue_ns::fix_config(in_args->down_info_.render_project_);
  import_and_render_ue_ns::fix_project(in_args->down_info_.render_project_);
  auto l_import_data = gen_import_config(*in_args);
  nlohmann::json l_json{};
  l_json          = l_import_data;
  auto l_tmp_path = FSys::write_tmp_file("ue_import", l_json.dump(), ".json");
  in_logger->warn("排队导入文件 {} ", in_args->down_info_.render_project_);
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    in_logger->error(" 用户取消操作");
    co_return std::tuple(boost::system::error_code{boost::asio::error::operation_aborted}, FSys::path{});
  }
  boost::system::error_code l_ec;
  // 添加三次重试
  for (int i = 0; i < 3; ++i) {
    l_ec = co_await async_run_ue(
        {in_args->down_info_.render_project_.generic_string(), "-windowed", "-log", "-stdout",
         "-AllowStdOutLogVerbosity", "-ForceLogFlush", "-Unattended", "-run=DoodleAutoAnimation",
         fmt::format("-Params={}", l_tmp_path)},
        in_logger
    );
    if (!l_ec) break;
    in_logger->warn("导入文件失败 开始第 {} 重试", i + 1);
    // 这个错误可以忽略, 有错误的情况下, 将状态设置为运行
  }
  if (l_ec) co_return std::tuple(l_ec, FSys::path{});

  in_logger->warn("导入文件完成");
  in_logger->warn("排队渲染, 输出目录 {}", l_import_data.out_file_dir);
  if (exists(l_import_data.out_file_dir)) {
    try {
      FSys::remove_all(l_import_data.out_file_dir);
    } catch (FSys::filesystem_error& err) {
      in_logger->error("渲染删除上次输出错误 error:{}", err.what());
    }
  }
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    in_logger->error(" 用户取消操作");
    co_return std::tuple(boost::system::error_code{boost::asio::error::operation_aborted}, FSys::path{});
  }
  for (int i = 0; i < 3; ++i) {
    l_ec = co_await async_run_ue(
        {in_args->down_info_.render_project_.generic_string(), l_import_data.render_map.generic_string(), "-game",
         fmt::format(R"(-LevelSequence="{}")", l_import_data.level_sequence_import),
         fmt::format(R"(-MoviePipelineConfig="{}")", l_import_data.movie_pipeline_config), "-windowed", "-log",
         "-stdout", "-AllowStdOutLogVerbosity", "-ForceLogFlush", "-Unattended"},
        in_logger
    );
    if (!l_ec) break;
    in_logger->warn("渲染失败 开始第 {} 重试", i + 1);
    // 这个错误可以忽略, 有错误的情况下, 将状态设置为运行
  }
  if (l_ec) co_return std::tuple(l_ec, FSys::path{});

  in_logger->warn("完成渲染, 输出目录 {}", l_import_data.out_file_dir);
  co_return std::tuple(boost::system::error_code{}, l_import_data.out_file_dir);
}

std::tuple<boost::system::error_code, std::vector<FSys::path>> clean_1001_before_frame(
    const FSys::path& in_path, std::int32_t in_frame
) {
  std::vector<FSys::path> l_move_paths{};
  std::vector<FSys::path> l_remove_paths{};
  if (!FSys::is_directory(in_path))
    return std::make_tuple(boost::system::errc::make_error_code(boost::system::errc::not_a_directory), l_move_paths);

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
  if (l_move_paths.empty()) {
    return {boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory), l_move_paths};
  }
  std::error_code l_sys_ec{};
  for (auto&& l_path : l_remove_paths) {
    FSys::remove(l_path, l_sys_ec);
  }
  return {boost::system::errc::make_error_code(boost::system::errc::success), l_move_paths};
}

boost::asio::awaitable<std::tuple<boost::system::error_code, FSys::path>> async_auto_loght(
    std::shared_ptr<import_and_render_ue_ns::args> in_args, logger_ptr in_logger
) {
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    in_logger->error(" 用户取消操作");
    co_return std::tuple(boost::system::error_code{boost::asio::error::operation_aborted}, FSys::path{});
  }
  // 添加三次重试
  maya_exe_ns::maya_out_arg l_out{};
  boost::system::error_code l_ec{};
  for (int i = 0; i < 3; ++i) {
    std::tie(l_ec, l_out) = co_await async_run_maya(in_args->maya_arg_, in_logger);
    if (!l_ec) {
      break;
    }
    in_logger->warn("运行maya错误, 开始第{}次重试", i + 1);
  }
  if (l_ec) co_return std::tuple(l_ec, FSys::path{});

  in_args->maya_out_arg_ = l_out;
  auto [l_ec2, l_ret]    = co_await async_import_and_render_ue(in_args, in_logger);
  if (l_ec2) {
    co_return std::tuple(l_ec2, FSys::path{});
  }

  // 合成视屏
  in_logger->warn("开始合成视屏 :{}", l_ret);
  auto l_movie_path =
      detail::create_out_path(l_ret.parent_path(), in_args->episodes_, in_args->shot_, in_args->project_.code_);
  {
    std::vector<FSys::path> l_move_paths{};
    std::tie(l_ec, l_move_paths) = clean_1001_before_frame(l_ret, in_args->maya_out_arg_.begin_time);
    l_ec                         = detail::create_move(
        l_movie_path, in_logger,
        movie::image_attr::make_default_attr(&in_args->episodes_, &in_args->shot_, l_move_paths), in_args->size_
    );
  }
  if (l_ec) {
    co_return std::tuple(l_ec, FSys::path{});
  }
  in_logger->warn("开始上传文件夹 :{}", l_ret);
  auto l_u_project = in_args->down_info_.render_project_;
  auto l_scene     = l_u_project.parent_path();
  auto l_rem_path  = in_args->project_.path_ / "03_Workflow" / doodle_config::ue4_shot /
                    fmt::format("EP{:04}", in_args->episodes_.p_episodes) / l_u_project.stem();
  // maya输出
  auto l_maya_out = in_args->maya_out_arg_.out_file_list |
                    ranges::views::transform([](const maya_exe_ns::maya_out_arg::out_file_t& in_arg) {
                      return in_arg.out_file.parent_path();
                    }) |
                    ranges::views::filter([](const FSys::path& in_path) { return !in_path.empty(); }) |
                    ranges::to_vector;
  l_maya_out |= ranges::actions::unique;
  std::vector<std::pair<FSys::path, FSys::path>> l_up_file_list{};
  // 渲染输出文件
  if (auto l_e = copy_diff(l_ret, l_rem_path / l_ret.lexically_proximate(l_scene), in_logger); l_e)
    co_return std::tuple{l_e, FSys::path{}};
  // 渲染工程文件
  if (auto l_e = copy_diff(l_scene / doodle_config::ue4_config, l_rem_path / doodle_config::ue4_config, in_logger))
    co_return std::tuple{l_e, FSys::path{}};
  if (auto l_e = copy_diff(l_scene / doodle_config::ue4_content, l_rem_path / doodle_config::ue4_content, in_logger))
    co_return std::tuple{l_e, FSys::path{}};
  if (auto l_e = copy_diff(l_u_project, l_rem_path / l_u_project.filename(), in_logger))
    co_return std::tuple{l_e, FSys::path{}};
  // maya输出文件
  for (const auto& l_maya : l_maya_out) {
    if (auto l_e = copy_diff(l_maya, l_rem_path.parent_path() / l_maya.stem(), in_logger))
      co_return std::tuple{l_e, FSys::path{}};
  }
  if (auto l_e = copy_diff(l_movie_path, l_rem_path.parent_path() / "mov" / l_movie_path.filename(), in_logger))
    co_return std::tuple{l_e, FSys::path{}};
  // 额外要求上传的序列图片
    if (auto l_e = copy_diff(l_ret, FSys::path{in_args->project_.auto_upload_path_} / fmt::format("EP{:03}", in_args->episodes_.p_episodes)/ "自动灯光序列帧"/l_ret.filename(), in_logger))
      co_return std::tuple{l_e, FSys::path{}};
  co_return std::tuple{l_ec, l_ret};
}
} // namespace doodle