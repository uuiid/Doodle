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
tl::expected<void, std::string> copy_diff(const FSys::path& from, const FSys::path& to, logger_ptr in_logger);

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

boost::asio::awaitable<tl::expected<FSys::path, std::string>> args::run() {
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none)
    co_return logger_ptr_->error("用户取消操作"), tl::make_unexpected("用户取消操作");

  // 添加三次重试
  maya_exe_ns::maya_out_arg l_out{};
  boost::system::error_code l_ec{};
  for (int i = 0; i < 3; ++i) {
    std::tie(l_ec, l_out) = co_await async_run_maya(maya_arg_, logger_ptr_);
    if (!l_ec) {
      break;
    }
    logger_ptr_->warn("运行maya错误, 开始第{}次重试", i + 1);
  }
  if (l_ec) co_return tl::make_unexpected(l_ec.what());

  maya_out_arg_ = l_out;
  auto l_ret    = co_await async_import_and_render_ue();
  if (!l_ret) co_return l_ret;

  // 合成视屏
  logger_ptr_->warn("开始合成视屏 :{}", *l_ret);
  auto l_movie_path = detail::create_out_path(l_ret->parent_path(), episodes_, shot_, project_.code_);
  {
    auto l_move_paths = clean_1001_before_frame(*l_ret, maya_out_arg_.begin_time);
    if (!l_move_paths) co_return tl::make_unexpected(l_move_paths.error());
    l_ec = detail::create_move(
        l_movie_path, logger_ptr_, movie::image_attr::make_default_attr(&episodes_, &shot_, *l_move_paths), size_
    );
  }
  if (l_ec) co_return tl::make_unexpected(l_ec.what());

  logger_ptr_->warn("开始上传文件夹 :{}", *l_ret);
  auto l_u_project = down_info_.render_project_;
  auto l_scene     = l_u_project.parent_path();
  auto l_rem_path  = project_.path_ / "03_Workflow" / doodle_config::ue4_shot /
                    fmt::format("EP{:04}", episodes_.p_episodes) / l_u_project.stem();
  // maya输出
  auto l_maya_out =
      maya_out_arg_.out_file_list | ranges::views::transform([](const maya_exe_ns::maya_out_arg::out_file_t& in_arg) {
        return in_arg.out_file.parent_path();
      }) |
      ranges::views::filter([](const FSys::path& in_path) { return !in_path.empty(); }) | ranges::to_vector;
  l_maya_out |= ranges::actions::unique;
  std::vector<std::pair<FSys::path, FSys::path>> l_up_file_list{};
  // 渲染输出文件
  if (auto l_e = copy_diff(*l_ret, l_rem_path / l_ret->lexically_proximate(l_scene), logger_ptr_); !l_e)
    co_return tl::make_unexpected(l_e.error());
  // 渲染工程文件
  if (auto l_e = copy_diff(l_scene / doodle_config::ue4_config, l_rem_path / doodle_config::ue4_config, logger_ptr_);
      !l_e)
    co_return tl::make_unexpected(l_e.error());
  if (auto l_e = copy_diff(l_scene / doodle_config::ue4_content, l_rem_path / doodle_config::ue4_content, logger_ptr_);
      !l_e)
    co_return tl::make_unexpected(l_e.error());
  if (auto l_e = copy_diff(l_u_project, l_rem_path / l_u_project.filename(), logger_ptr_); !l_e)
    co_return tl::make_unexpected(l_e.error());
  // maya输出文件
  for (const auto& l_maya : l_maya_out) {
    if (auto l_e = copy_diff(l_maya, l_rem_path.parent_path() / l_maya.stem(), logger_ptr_); !l_e)
      co_return tl::make_unexpected(l_e.error());
  }
  if (auto l_e = copy_diff(l_movie_path, l_rem_path.parent_path() / "mov" / l_movie_path.filename(), logger_ptr_); !l_e)
    co_return tl::make_unexpected(l_e.error());
  // 额外要求上传的序列图片
  if (auto l_e = copy_diff(
          *l_ret,
          FSys::path{project_.auto_upload_path_} / fmt::format("EP{:03}", episodes_.p_episodes) / "自动灯光序列帧" /
              l_ret->filename(),
          logger_ptr_
      ))
    co_return tl::make_unexpected(l_e.error());
  co_return tl::expected<FSys::path, std::string>{*l_ret};
}

boost::asio::awaitable<tl::expected<FSys::path, std::string>> args::async_import_and_render_ue() {
  // 先分析和下载文件
  if (auto l_r = co_await analysis_out_file(); !l_r) co_return tl::make_unexpected(l_r.error());
  // 导入文件
  fix_config(down_info_.render_project_);
  fix_project(down_info_.render_project_);
  auto l_import_data = gen_import_config();
  nlohmann::json l_json{};
  l_json          = l_import_data;
  auto l_tmp_path = FSys::write_tmp_file("ue_import", l_json.dump(), ".json");
  logger_ptr_->warn("排队导入文件 {} ", down_info_.render_project_);
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    logger_ptr_->error("用户取消操作");
    co_return tl::make_unexpected("用户取消操作"s);
  }
  boost::system::error_code l_ec;
  // 添加三次重试
  for (int i = 0; i < 3; ++i) {
    l_ec = co_await async_run_ue(
        {down_info_.render_project_.generic_string(), "-windowed", "-log", "-stdout", "-AllowStdOutLogVerbosity",
         "-ForceLogFlush", "-Unattended", "-run=DoodleAutoAnimation", fmt::format("-Params={}", l_tmp_path)},
        logger_ptr_
    );
    if (!l_ec) break;
    logger_ptr_->warn("导入文件失败 开始第 {} 重试", i + 1);
    // 这个错误可以忽略, 有错误的情况下, 将状态设置为运行
  }
  if (l_ec) co_return tl::make_unexpected(l_ec.what());

  logger_ptr_->warn("导入文件完成");
  logger_ptr_->warn("排队渲染, 输出目录 {}", l_import_data.out_file_dir);
  if (exists(l_import_data.out_file_dir)) {
    try {
      FSys::remove_all(l_import_data.out_file_dir);
    } catch (FSys::filesystem_error& err) {
      logger_ptr_->error("渲染删除上次输出错误 error:{}", err.what());
    }
  }
  if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
    logger_ptr_->error(" 用户取消操作");
    co_return tl::make_unexpected("用户取消操作");
  }
  for (int i = 0; i < 3; ++i) {
    l_ec = co_await async_run_ue(
        {down_info_.render_project_.generic_string(), l_import_data.render_map.generic_string(), "-game",
         fmt::format(R"(-LevelSequence="{}")", l_import_data.level_sequence_import),
         fmt::format(R"(-MoviePipelineConfig="{}")", l_import_data.movie_pipeline_config), "-windowed", "-log",
         "-stdout", "-AllowStdOutLogVerbosity", "-ForceLogFlush", "-Unattended"},
        logger_ptr_
    );
    if (!l_ec) break;
    logger_ptr_->warn("渲染失败 开始第 {} 重试", i + 1);
    // 这个错误可以忽略, 有错误的情况下, 将状态设置为运行
  }
  if (l_ec) co_return tl::make_unexpected(l_ec.what());

  logger_ptr_->warn("完成渲染, 输出目录 {}", l_import_data.out_file_dir);
  co_return tl::expected<FSys::path, std::string>{l_import_data.out_file_dir};
}
boost::asio::awaitable<tl::expected<void, std::string>> args::analysis_out_file() {
  std::vector<boost::uuids::uuid> l_uuids_tmp{};
  std::map<boost::uuids::uuid, FSys::path> l_refs_tmp{};
  import_and_render_ue_ns::down_info l_out{};

  for (auto&& i : maya_out_arg_.out_file_list) {
    if (!FSys::exists(i.ref_file)) continue;
    logger_ptr_->warn("引用文件不存在:{}", i.ref_file);
    auto l_uuid = FSys::software_flag_file(i.ref_file);
    if (l_uuid.is_nil()) {
      logger_ptr_->error("获取引用文件失败 {}", i.ref_file);
      co_return tl::make_unexpected(fmt::format("获取引用文件失败 {}", i.ref_file));
    }

    l_uuids_tmp.push_back(l_uuid);
    l_refs_tmp.emplace(l_uuid, i.ref_file);
  }

  std::sort(l_uuids_tmp.begin(), l_uuids_tmp.end(), [](const auto& l, const auto& r) { return l < r; });
  l_uuids_tmp.erase(
      std::unique(l_uuids_tmp.begin(), l_uuids_tmp.end(), [](const auto& l, const auto& r) { return l == r; }),
      l_uuids_tmp.end()
  );
  auto l_refs = co_await fetch_association_data(l_refs_tmp);
  if (!l_refs) co_return tl::make_unexpected(l_refs.error());

  // 检查文件
  auto l_scene_uuid = boost::uuids::nil_uuid();
  FSys::path l_down_path_file_name{};

  for (auto&& [id, h] : *l_refs) {
    if (auto l_is_e = h.ue_file_.empty(), l_is_ex = FSys::exists(h.ue_file_); l_is_e || !l_is_ex) {
      if (l_is_e)
        co_return logger_ptr_->error("文件 {} 的 ue 引用无效, 为空", h.maya_file_),
            tl::make_unexpected(fmt::format("文件 {} 的 ue 引用无效, 为空", h.maya_file_));
      else if (!l_is_ex)
        co_return logger_ptr_->error("文件 {} 的 ue {} 引用不存在", h.maya_file_, h.ue_file_),
            tl::make_unexpected(fmt::format("文件 {} 的 ue {} 引用不存在", h.maya_file_, h.ue_file_));
    }

    if (h.type_ == details::assets_type_enum::scene) {
      l_scene_uuid          = h.id_;
      l_down_path_file_name = h.ue_prj_path_.parent_path().filename();
    }
  }
  if (l_scene_uuid.is_nil()) {
    co_return logger_ptr_->error("未查找到主项目文件(没有找到场景文件)"),
        tl::make_unexpected("未查找到主项目文件(没有找到场景文件)");
  }

  // 添加导入问价对应的sk文件
  for (auto&& l_path : maya_out_arg_.out_file_list) {
    if (l_path.ref_file.empty()) {
      l_out.file_list_.emplace_back(l_path.out_file, "");
      continue;  /// 如果为空,是相机, 无引用, 不查找
    }

    auto l_id = FSys::software_flag_file(l_path.ref_file);
    if (!l_refs->contains(l_id)) co_return tl::make_unexpected(fmt::format("路径中未找到sk {}", l_path.ref_file));
    if (l_refs->at(l_id).type_ == details::assets_type_enum::scene) continue;  // 场景文件, 不用管

    auto l_root     = l_refs->at(l_id).ue_prj_path_.parent_path() / doodle_config::ue4_content;
    auto l_original = l_refs->at(l_id).ue_file_.lexically_relative(l_root);
    l_out.file_list_.emplace_back(
        l_path.out_file, fmt::format("/Game/{}/{}", l_original.parent_path().generic_string(), l_original.stem())
    );
  }

  static auto g_root{FSys::path{doodle_config::g_cache_path}};
  std::vector<std::pair<FSys::path, FSys::path>> l_copy_path{};
  logger_ptr_->warn("排队复制文件");
  // 开始复制文件
  // 先获取UE线程(只能在单线程复制, 要不然会出现边渲染边复制的情况, 会出错)
  auto l_g = co_await g_ctx().get<ue_ctx>().queue_->queue(boost::asio::use_awaitable);
  for (auto&& [id, h] : *l_refs) {
    auto l_down_path  = h.ue_prj_path_.parent_path();
    auto l_root       = h.ue_prj_path_.parent_path() / doodle_config::ue4_content;
    auto l_local_path = g_root / project_.code_ / l_down_path_file_name;
    if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
      logger_ptr_->error("用户取消操作");
      co_return tl::make_unexpected("用户取消操作");
    }
    switch (h.type_) {
      // 场景文件
      case details::assets_type_enum::scene: {
        auto l_original   = h.ue_file_.lexically_relative(l_root);
        l_out.scene_file_ = fmt::format("/Game/{}/{}", l_original.parent_path().generic_string(), l_original.stem());

        if (auto l_ec_copy = copy_diff(
                l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content, logger_ptr_
            );
            !l_ec_copy)
          co_return tl::make_unexpected(l_ec_copy.error());
        // 配置文件夹复制
        if (auto l_ec_copy = copy_diff(
                l_down_path / doodle_config::ue4_config, l_local_path / doodle_config::ue4_config, logger_ptr_
            );
            !l_ec_copy)
          co_return tl::make_unexpected(l_ec_copy.error());
        // 复制项目文件
        if (!FSys::exists(l_local_path / h.ue_prj_path_.filename()))
          if (auto l_ec_copy = copy_diff(h.ue_prj_path_, l_local_path / h.ue_prj_path_.filename(), logger_ptr_);
              l_ec_copy)
            co_return tl::make_unexpected(l_ec_copy.error());

        l_out.render_project_ = l_local_path / h.ue_prj_path_.filename();
      } break;

      // 角色文件
      case details::assets_type_enum::character:
        if (auto l_ec_copy = copy_diff(
                l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content, logger_ptr_
            );
            !l_ec_copy)
          co_return tl::make_unexpected(l_ec_copy.error());
        break;

      // 道具文件
      case details::assets_type_enum::prop: {
        auto l_prop_path = h.ue_file_.lexically_relative(l_root / "Prop");
        if (l_prop_path.empty()) continue;
        auto l_prop_path_name = *l_prop_path.begin();
        if (auto l_ec_copy = copy_diff(
                l_down_path / doodle_config::ue4_content / "Prop" / l_prop_path_name,
                l_local_path / doodle_config::ue4_content / "Prop" / l_prop_path_name, logger_ptr_
            );
            !l_ec_copy)
          co_return tl::make_unexpected(l_ec_copy.error());
        /// 此处忽略错误
        copy_diff(
            l_down_path / doodle_config::ue4_content / "Prop" / "a_PropPublicFiles",
            l_local_path / doodle_config::ue4_content / "Prop" / "a_PropPublicFiles", logger_ptr_
        );
      } break;
      default:
        break;
    }
  }
  down_info_ = l_out;
}

boost::asio::awaitable<tl::expected<std::map<uuid, association_data>, std::string>> args::fetch_association_data(
    std::map<uuid, FSys::path> in_uuid
) {
  std::map<uuid, association_data> l_out{};
  boost::beast::tcp_stream l_stream{g_io_context()};
  auto l_c = std::make_shared<http::detail::http_client_data_base>(co_await boost::asio::this_coro::executor);
  l_c->init(core_set::get_set().server_ip);
  try {
    for (auto&& [i, l_path] : in_uuid) {
      boost::beast::http::request<boost::beast::http::empty_body> l_req{
          boost::beast::http::verb::get, fmt::format("/api/doodle/file_association/{}", i), 11
      };
      l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      l_req.set(boost::beast::http::field::accept, "application/json");
      l_req.prepare_payload();
      auto [l_ec, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(l_c, l_req);

      if (l_res.result() != boost::beast::http::status::ok) {
        logger_ptr_->log(log_loc(), level::warn, "未找到关联数据: {} {}", i, l_path);
        co_return tl::make_unexpected(fmt::format("未找到关联数据: {} {}", i, l_path));
      }

      auto l_json = nlohmann::json::parse(l_res.body());
      association_data l_data{
          .id_        = i,
          .maya_file_ = l_json.at("maya_file").get<std::string>(),
          .ue_file_   = l_json.at("ue_file").get<std::string>(),
          .type_      = l_json.at("type").get<details::assets_type_enum>()
      };
      l_out.emplace(i, std::move(l_data));
    }
  } catch (const std::exception& e) {
    logger_ptr_->error("连接服务器失败:{}", e.what());
    co_return tl::make_unexpected(fmt::format("连接服务器失败:{}", e.what()));
  }
  for (auto&& [k, i] : l_out) {
    i.ue_prj_path_ = ue_exe_ns::find_ue_project_file(i.ue_file_);
  }
  co_return tl::expected<std::map<uuid, association_data>, std::string>{std::move(l_out)};
}

import_data_t args::gen_import_config() {
  auto l_maya_out_arg = maya_out_arg_.out_file_list |
                        ranges::views::filter([](const maya_exe_ns::maya_out_arg::out_file_t& in_arg) {
                          return !in_arg.out_file.empty() && FSys::exists(in_arg.out_file);
                        }) |
                        ranges::to_vector;
  import_data_t l_import_data;
  l_import_data.episode    = episodes_;
  l_import_data.shot       = shot_;
  l_import_data.begin_time = maya_out_arg_.begin_time;
  l_import_data.end_time   = maya_out_arg_.end_time;
  l_import_data.size_      = size_;
  l_import_data.layering_  = layering_;

  l_import_data.project_   = project_;
  l_import_data.out_file_dir =
      down_info_.render_project_.parent_path() / doodle_config::ue4_saved / doodle_config::ue4_movie_renders /
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
        chrono::current_zone()->to_local(chrono::system_clock::now())
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
  l_import_data.original_map = down_info_.scene_file_.generic_string();
  return l_import_data;
}
}  // namespace import_and_render_ue_ns

void copy_diff_impl(const FSys::path& from, const FSys::path& to) {
  if (!FSys::exists(to) || FSys::file_size(from) != FSys::file_size(to) ||
      FSys::last_write_time(from) != FSys::last_write_time(to)) {
    if (!FSys::exists(to) || FSys::last_write_time(from) > FSys::last_write_time(to)) {
      if (!FSys::exists(to.parent_path())) FSys::create_directories(to.parent_path());
      FSys::copy_file(from, to, FSys::copy_options::overwrite_existing);
    }
  }
}

tl::expected<void, std::string> copy_diff(const FSys::path& from, const FSys::path& to, logger_ptr in_logger) {
  for (int i = 0; i < 10; ++i) {
    try {
      in_logger->warn("复制 {} -> {}", from, to);
      if (FSys::is_regular_file(from) && !FSys::is_hidden(from) &&
          from.extension() != doodle_config::doodle_flag_name) {
        copy_diff_impl(from, to);
        return {};
      }
      for (auto&& l_file : FSys::recursive_directory_iterator(from)) {
        auto l_to_file = to / l_file.path().lexically_proximate(from);
        if (l_file.is_regular_file() && !FSys::is_hidden(l_file.path()) &&
            l_file.path().extension() != doodle_config::doodle_flag_name) {
          copy_diff_impl(l_file.path(), l_to_file);
        }
      }
      return {};
    } catch (...) {
      in_logger->log(log_loc(), spdlog::level::err, "未知错误 {}", boost::current_exception_diagnostic_information());
      return tl::make_unexpected(boost::current_exception_diagnostic_information());
    }
  }

  return {};
}

tl::expected<std::vector<FSys::path>, std::string> clean_1001_before_frame(
    const FSys::path& in_path, std::int32_t in_frame
) {
  std::vector<FSys::path> l_move_paths{};
  std::vector<FSys::path> l_remove_paths{};
  if (!FSys::is_directory(in_path)) return tl::make_unexpected("没有这样的目录");

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
  if (l_move_paths.empty()) return tl::make_unexpected("未扫描到文件");
  std::error_code l_sys_ec{};
  for (auto&& l_path : l_remove_paths) {
    FSys::remove(l_path, l_sys_ec);
  }
  return l_move_paths;
}

boost::asio::awaitable<std::tuple<boost::system::error_code, FSys::path>> async_auto_loght(
    std::shared_ptr<import_and_render_ue_ns::args> in_args, logger_ptr in_logger
) {
  in_args->logger_ptr_ = in_logger;
  co_await in_args->run();
  co_return std::tuple<boost::system::error_code, FSys::path>{};
}

}  // namespace doodle