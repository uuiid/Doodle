//
// Created by TD on 2024/1/8.
//

#include "import_and_render_ue.h"

#include <doodle_core/core/http_client_core.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/core/alembic_file.h>
#include <doodle_lib/core/fbx_file.h>
#include <doodle_lib/core/scan_assets/base.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/system.hpp>

namespace doodle {

namespace import_and_render_ue_ns {
bool copy_diff_impl(const FSys::path& from, const FSys::path& to) {
  if (from.extension() == doodle_config::doodle_flag_name) return false;
  if (!FSys::exists(to) || FSys::file_size(from) != FSys::file_size(to) ||
      FSys::last_write_time(from) > FSys::last_write_time(to)) {
    if (auto l_p = to.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    return FSys::copy_file(from, to, FSys::copy_options::overwrite_existing);
  }
  return false;
}

bool copy_diff(const FSys::path& from, const FSys::path& to, logger_ptr in_logger) {
  if (!FSys::exists(from)) return false;
  in_logger->warn("复制 {} -> {}", from, to);
  if (FSys::is_regular_file(from)) return copy_diff_impl(from, to);

  for (auto&& l_file : FSys::recursive_directory_iterator(from)) {
    auto l_to_file = to / l_file.path().lexically_proximate(from);
    if (l_file.is_regular_file()) return copy_diff_impl(l_file.path(), l_to_file);
  }
  return false;
}
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
void args::check_materials() {
  std::vector<import_file*> l_files =
      import_files_ | ranges::views::transform([](import_file& in_arg) { return &in_arg; }) |
      ranges::views::filter([](import_file* in_arg) -> bool {
        auto l_path = in_arg->file_;
        return l_path.extension() == ".abc" && FSys::exists(l_path.replace_extension(".fbx"));
      }) |
      ranges::to_vector;

  for (auto&& l_file : l_files) {
    auto l_fbx_path = l_file->file_;
    l_fbx_path.replace_extension(".fbx");

    auto l_fbx_mat = fbx::get_all_materials(l_fbx_path);
    auto l_abc_mat = alembic::get_all_materials(l_file->file_);
    std::vector<std::string> l_intersection{};
    std::ranges::set_intersection(l_abc_mat, l_fbx_mat, std::back_inserter(l_intersection));
    if (!l_intersection.empty())
      throw_exception(doodle_error{"错误的材质, 同时存在于解算和不解算物体上 :{}", l_intersection});
    l_file->hide_materials_ = l_fbx_mat;
  }
}
boost::asio::awaitable<void> args::run() {
  // 添加三次重试
  maya_exe_ns::maya_out_arg l_out{};
  for (int i = 0; i < 3; ++i) {
    try {
      l_out = co_await async_run_maya(maya_arg_, logger_ptr_);
      break;
    } catch (const doodle_error& err) {
      logger_ptr_->warn("运行maya错误 {}, 开始第{}次重试", err.what(), i + 1);
      if (i == 2) throw;
    }
  }
  /// 将导出数据转移到数据块中
  begin_time_ = l_out.begin_time;
  end_time_   = l_out.end_time;
  for (auto&& i : l_out.out_file_list) {
    auto&& l_data     = import_files_.emplace_back();
    l_data.file_      = i.out_file;
    l_data.maya_file_ = i.ref_file;
    l_data.is_camera_ = i.out_file.generic_string().find("_camera_") != std::string::npos;
    logger_ptr_->info("{},is_camera {}", l_data.file_, l_data.is_camera_);
    if (i.ref_file.empty() || !FSys::exists(i.ref_file)) continue;
    logger_ptr_->info("引用文件:{}", i.ref_file);
    l_data.id = FSys::software_flag_file(i.ref_file);
    if (l_data.id.is_nil()) throw_exception(doodle_error{"获取引用文件失败 {}", i.ref_file});
  }

  co_await fetch_association_data();
  // if (bind_skin_) check_materials();

  {
    // 开始复制文件
    // 先获取UE线程(只能在单线程复制, 要不然会出现边渲染边复制的情况, 会出错)
    auto l_g = co_await g_ctx().get<ue_ctx>().queue_->queue(boost::asio::use_awaitable);
    down_files();
    fix_config(render_project_);
    fix_project(render_project_);
  }
  if (bind_skin_) co_await crate_skin();

  auto l_ret = co_await async_import_and_render_ue();
  // 合成视屏, 并上传文件
  up_files(l_ret, create_move(l_ret));

  co_return;
}
void args::up_files(const FSys::path& in_out_image_path, const FSys::path& in_move_path) const {
  auto l_u_project = render_project_;
  auto l_scene     = l_u_project.parent_path();
  auto l_rem_path  = project_.path_ / "03_Workflow" / doodle_config::ue4_shot /
                    fmt::format("EP{:04}", episodes_.p_episodes) / l_u_project.stem();
  // maya输出
  auto l_maya_out =
      import_files_ | ranges::views::transform([](const import_file& in_arg) { return in_arg.file_.parent_path(); }) |
      ranges::views::filter([](const FSys::path& in_path) { return !in_path.empty(); }) | ranges::to_vector;
  l_maya_out |= ranges::actions::unique;

  // 渲染输出文件
  copy_diff(in_out_image_path, l_rem_path / in_out_image_path.lexically_proximate(l_scene), logger_ptr_);
  // 渲染工程文件
  copy_diff(l_scene / doodle_config::ue4_config, l_rem_path / doodle_config::ue4_config, logger_ptr_);
  copy_diff(l_scene / doodle_config::ue4_content, l_rem_path / doodle_config::ue4_content, logger_ptr_);
  copy_diff(l_u_project, l_rem_path / l_u_project.filename(), logger_ptr_);
  // maya输出文件
  for (const auto& l_maya : l_maya_out) {
    copy_diff(l_maya, l_rem_path.parent_path() / l_maya.stem(), logger_ptr_);
  }
  copy_diff(in_move_path, l_rem_path.parent_path() / "mov" / in_move_path.filename(), logger_ptr_);
  // 额外要求上传的序列图片
  copy_diff(
      in_out_image_path,
      FSys::path{project_.auto_upload_path_} / fmt::format("EP{:03}", episodes_.p_episodes) / "自动灯光序列帧" /
          in_out_image_path.filename(),
      logger_ptr_
  );
}
boost::asio::awaitable<FSys::path> args::async_import_and_render_ue() {
  // 导入文件
  auto l_import_data = gen_import_config();
  nlohmann::json l_json{};
  l_json          = l_import_data;
  auto l_tmp_path = FSys::write_tmp_file("ue_import", l_json.dump(), ".json");
  logger_ptr_->warn("排队导入文件 {} ", render_project_);
  // 添加三次重试
  for (int i = 0; i < 3; ++i) {
    try {
      co_await async_run_ue(
          {render_project_.generic_string(), "-windowed", "-log", "-stdout", "-AllowStdOutLogVerbosity",
           "-ForceLogFlush", "-Unattended", "-run=DoodleAutoAnimation", fmt::format("-Params={}", l_tmp_path)},
          logger_ptr_
      );
      break;
    } catch (const doodle_error& err) {
      logger_ptr_->warn("导入文件失败 开始第 {} 重试", i + 1);
      if (i == 2) throw;
    }
  }

  logger_ptr_->warn("导入文件完成");
  logger_ptr_->warn("排队渲染, 输出目录 {}", l_import_data.out_file_dir);
  if (exists(l_import_data.out_file_dir)) {
    try {
      FSys::remove_all(l_import_data.out_file_dir);
    } catch (const FSys::filesystem_error& err) {
      logger_ptr_->error("渲染删除上次输出错误:{}", err.what());
    }
  }
  for (int i = 0; i < 3; ++i) {
    try {
      co_await async_run_ue(
          {render_project_.generic_string(), l_import_data.render_map.generic_string(), "-game",
           fmt::format(R"(-LevelSequence="{}")", l_import_data.level_sequence_import),
           fmt::format(R"(-MoviePipelineConfig="{}")", l_import_data.movie_pipeline_config), "-windowed", "-log",
           "-stdout", "-AllowStdOutLogVerbosity", "-ForceLogFlush", "-Unattended"},
          logger_ptr_
      );
      break;
    } catch (const doodle_error& err) {
      logger_ptr_->warn("渲染失败 开始第 {} 重试", i + 1);
      if (i == 2) throw;
    }
  }

  logger_ptr_->warn("完成渲染, 输出目录 {}", l_import_data.out_file_dir);
  co_return l_import_data.out_file_dir;
}

boost::asio::awaitable<void> args::fetch_association_data() {
  std::map<uuid, association_data> l_out{};
  boost::beast::tcp_stream l_stream{g_io_context()};
  auto l_c = std::make_shared<http::detail::http_client_data_base>(co_await boost::asio::this_coro::executor);
  l_c->init(core_set::get_set().server_ip);
  for (auto&& l_data : import_files_) {
    if (l_data.is_camera_) {
      l_data.type_ = details::assets_type_enum::other;
      continue;
    }
    boost::beast::http::request<boost::beast::http::empty_body> l_req{
        boost::beast::http::verb::get, fmt::format("/api/doodle/file_association/{}", l_data.id), 11
    };
    l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    l_req.set(boost::beast::http::field::accept, "application/json");
    l_req.prepare_payload();
    auto [l_ec, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(l_c, l_req);

    if (l_res.result() != boost::beast::http::status::ok)
      throw_exception(doodle_error{"未找到关联数据: {} {}", l_data.id, l_data.maya_file_});

    auto l_json         = nlohmann::json::parse(l_res.body());

    l_data.ue_file_     = l_json.at("ue_file").get<std::string>();
    l_data.type_        = l_json.at("type").get<details::assets_type_enum>();
    l_data.ue_prj_path_ = ue_exe_ns::find_ue_project_file(l_data.ue_file_);
  }
  // 检查文件
  auto l_scene_uuid = boost::uuids::nil_uuid();

  for (auto&& l_data : import_files_) {
    if (l_data.is_camera_) continue;
    if (l_data.ue_file_.empty())
      throw_exception(doodle_error{"文件 {} 的 ue 引用无效, 为空 {}", l_data.maya_file_, l_data.id});
    if (!FSys::exists(l_data.ue_file_))
      throw_exception(doodle_error{"文件 {} 的 ue {} 引用不存在", l_data.maya_file_, l_data.ue_file_});

    if (l_data.type_ == details::assets_type_enum::scene) {
      l_scene_uuid = l_data.id;
    }
  }
  if (l_scene_uuid.is_nil()) throw_exception(doodle_error{"未查找到主项目文件(没有找到场景文件)"});
}
FSys::path args::create_move(const FSys::path& in_out_image_path) const {
  // 合成视屏
  logger_ptr_->warn("开始合成视屏 :{}", in_out_image_path);
  auto l_movie_path = detail::create_out_path(in_out_image_path.parent_path(), episodes_, shot_, project_.code_);
  {
    boost::system::error_code l_ec{};
    auto l_move_paths = clean_1001_before_frame(in_out_image_path, begin_time_);
    if (!l_move_paths) throw_exception(doodle_error{l_move_paths.error()});
    l_ec = detail::create_move(
        l_movie_path, logger_ptr_, movie::image_attr::make_default_attr(&episodes_, &shot_, *l_move_paths), size_
    );
    if (l_ec) throw_exception(doodle_error{l_ec.what()});
  }
  return l_movie_path;
}
void args::down_files() {
  static auto g_root{FSys::path{doodle_config::g_cache_path}};
  logger_ptr_->warn("排队复制文件");
  FSys::path l_down_path_file_name =
      std::ranges::find_if(
          import_files_, [](const import_file& in_data) { return in_data.type_ == details::assets_type_enum::scene; }
      )
          ->ue_prj_path_.parent_path()
          .filename();
  // 复制UE文件
  for (auto&& l_data : import_files_) {
    auto l_down_path  = l_data.ue_prj_path_.parent_path();
    auto l_root       = l_data.ue_prj_path_.parent_path() / doodle_config::ue4_content;
    auto l_local_path = g_root / project_.code_ / l_down_path_file_name;
    switch (l_data.type_) {
      // 场景文件
      case details::assets_type_enum::scene: {
        auto l_original = l_data.ue_file_.lexically_relative(l_root);
        scene_file_     = fmt::format("/Game/{}/{}", l_original.parent_path().generic_string(), l_original.stem());

        copy_diff(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content, logger_ptr_);
        // 配置文件夹复制
        copy_diff(l_down_path / doodle_config::ue4_config, l_local_path / doodle_config::ue4_config, logger_ptr_);
        // 复制项目文件
        if (!FSys::exists(l_local_path / l_data.ue_prj_path_.filename()))
          copy_diff(l_data.ue_prj_path_, l_local_path / l_data.ue_prj_path_.filename(), logger_ptr_);

        render_project_ = l_local_path / l_data.ue_prj_path_.filename();
      } break;

      // 角色文件
      case details::assets_type_enum::character:
        l_data.update_files =
            copy_diff(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content, logger_ptr_);
        break;

      // 道具文件
      case details::assets_type_enum::prop: {
        auto l_prop_path = l_data.ue_file_.lexically_relative(l_root / "Prop");
        if (l_prop_path.empty()) continue;
        auto l_prop_path_name = *l_prop_path.begin();
        l_data.update_files   = copy_diff(
            l_down_path / doodle_config::ue4_content / "Prop" / l_prop_path_name,
            l_local_path / doodle_config::ue4_content / "Prop" / l_prop_path_name, logger_ptr_
        );
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

  // 复制maya文件
  if (bind_skin_)
    for (auto&& l_data : import_files_) {
      if (!(l_data.type_ == details::assets_type_enum::character || l_data.type_ == details::assets_type_enum::prop))
        continue;
      auto l_local_path       = g_root / project_.code_ / "maya_file";
      l_data.maya_local_file_ = l_local_path / l_data.maya_file_.filename();
      l_data.update_files |= copy_diff(l_data.maya_file_, l_data.maya_local_file_, logger_ptr_);
    }
}

boost::asio::awaitable<void> args::crate_skin() {
  auto l_ue_g = co_await g_ctx().get<ue_ctx>().queue_->queue(boost::asio::use_awaitable);

  for (auto&& l_data : import_files_) {
    if (l_data.is_camera_) continue;  // 相机文件
    if (l_data.file_.extension() == ".abc") continue;
    if (l_data.maya_local_file_.empty()) continue;  // 这个场景文件
    auto l_import_game_path  = FSys::path{doodle_config::ue4_game} / "auto_light";
    l_data.skin_             = l_import_game_path / l_data.maya_local_file_.stem();

    auto l_import_local_path = render_project_.parent_path() / FSys::path{doodle_config::ue4_content} / "auto_light" /
                               l_data.maya_local_file_.filename();
    l_import_local_path.replace_extension(".uasset");
    if (FSys::exists(l_import_local_path) && !l_data.update_files) continue;

    auto l_maya_arg       = std::make_shared<maya_exe_ns::export_rig_arg>();
    l_maya_arg->file_path = l_data.maya_local_file_;
    auto l_maya_file      = co_await async_run_maya(l_maya_arg, logger_ptr_);
    if (l_maya_file.out_file_list.empty()) throw_exception(doodle_error{"文件 {}, 未能输出骨架fbx", l_data.maya_file_});
    auto l_fbx = l_maya_file.out_file_list.front().out_file;
    nlohmann::json l_json{};
    l_json          = import_skin_file{.fbx_file_ = l_fbx, .import_dir_ = l_import_game_path};
    auto l_tmp_path = FSys::write_tmp_file("ue_import", l_json.dump(), ".json");
    logger_ptr_->warn("排队导入skin文件 {} ", render_project_);
    // 添加三次重试
    for (int i = 0; i < 3; ++i) {
      try {
        co_await async_run_ue(
            {render_project_.generic_string(), "-windowed", "-log", "-stdout", "-AllowStdOutLogVerbosity",
             "-ForceLogFlush", "-Unattended", "-run=DoodleAutoAnimation", fmt::format("-ImportRig={}", l_tmp_path)},
            logger_ptr_, false
        );
        break;
      } catch (const doodle_error& error) {
        logger_ptr_->warn("导入文件失败 开始第 {} 重试", i + 1);
        if (i == 2) throw;
      }
    }
  }
}

import_data_t args::gen_import_config() {
  auto l_maya_out_arg = import_files_ | ranges::views::filter([](const import_file& in_arg) {
                          return in_arg.type_ != details::assets_type_enum::scene;
                        }) |
                        ranges::to_vector;
  import_data_t l_import_data;
  l_import_data.episode    = episodes_;
  l_import_data.shot       = shot_;
  l_import_data.begin_time = begin_time_;
  l_import_data.end_time   = end_time_;
  l_import_data.size_      = size_;
  l_import_data.layering_  = layering_;

  l_import_data.project_   = project_;
  l_import_data.out_file_dir =
      render_project_.parent_path() / doodle_config::ue4_saved / doodle_config::ue4_movie_renders /
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

  l_import_data.files = l_maya_out_arg | ranges::views::transform([](const import_file& in_arg) {
                          auto l_file_name_str = in_arg.file_.filename().generic_string();
                          auto l_ext           = in_arg.file_.extension().generic_string();
                          std::string l_type{};
                          if (in_arg.is_camera_) {
                            l_type = "cam";
                          } else if (l_ext == ".abc") {
                            l_type = "geo";
                          } else if (l_ext == ".fbx") {
                            l_type = "char";
                          }
                          return import_files_t{l_type, in_arg.file_, in_arg.skin_, in_arg.hide_materials_};
                        }) |
                        ranges::to_vector;
  l_import_data.original_map = scene_file_.generic_string();
  return l_import_data;
}
}  // namespace import_and_render_ue_ns

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

}  // namespace doodle