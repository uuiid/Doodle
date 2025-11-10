//
// Created by TD on 24-12-30.
//

#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/episodes.h"
#include "doodle_core/metadata/shot.h"
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/entity_path.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/exe_warp/export_rig_sk.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <boost/asio/awaitable.hpp>

#include <filesystem>
#include <fmt/format.h>
#include <map>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <vector>

namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_shots_run_ue_assembly::post(
    session_data_ptr in_handle
) {
  auto l_sql         = g_ctx().get<sqlite_database>();
  auto l_shot_task   = l_sql.get_by_uuid<task>(id_);
  auto l_shot_entity = l_sql.get_by_uuid<entity>(l_shot_task.entity_id_);
  if (l_shot_entity.parent_id_.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "镜头实体缺少父级序列信息"});
  auto l_episode_entity = l_sql.get_by_uuid<entity>(l_shot_entity.parent_id_);
  auto l_prj            = l_sql.get_by_uuid<project>(project_id_);

  episodes l_episodes{l_episode_entity};
  shot l_shot{l_shot_entity};

  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();

  run_ue_assembly_local::run_ue_assembly_arg l_ret{};
  l_ret.episodes_ = l_episodes;
  l_ret.shot_     = l_shot;

  l_ret.size_     = image_size{.width = l_prj.get_resolution().first, .height = l_prj.get_resolution().second};
  FSys::path l_shot_path_dir{};
  FSys::path l_sim_shot_path_dir{};
  std::set<std::string> l_sim_output_key{};
  bool l_is_simulation_task = l_shot_task.task_type_id_ == task_type::get_simulation_task_id();
  /// tag: 格式化路径
  if (l_shot_task.task_type_id_ == task_type::get_animation_id()) {
    l_shot_path_dir = get_shots_animation_output_path(l_episode_entity.name_, l_shot_entity.name_, l_prj.code_);
  } else if (l_is_simulation_task) {
    l_sim_shot_path_dir = get_shots_simulation_output_path(l_episode_entity.name_, l_shot_entity.name_, l_prj.code_);
  } else
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "任务类型不支持该操作"});

  l_shot_path_dir = l_prj.path_ / l_shot_path_dir;
  if (l_is_simulation_task) l_sim_shot_path_dir = l_prj.path_ / l_shot_path_dir;
  auto l_shot_file_name =
      get_shots_animation_file_name(l_episode_entity.name_, l_shot_entity.name_, l_prj.code_).generic_string();

  if (l_is_simulation_task) {
    for (auto&& l_path : FSys::directory_iterator{l_sim_shot_path_dir}) {
      auto l_stem = l_path.path().stem().string();
      if (!(l_stem.starts_with(l_shot_file_name) &&
            (l_path.path().extension() == ".abc" || l_path.path().extension() == ".fbx")))
        continue;
      if (auto l_cam = l_stem.find("_camera_"); l_cam != std::string::npos) continue;

      l_ret.asset_infos_.emplace_back(
          run_ue_assembly_local::run_ue_assembly_asset_info{
              .shot_output_path_ = l_path.path(), .type_ = l_path.path().extension() == ".fbx" ? "char" : "geo"
          }
      );
      l_sim_output_key.emplace(l_stem);
    }
  }

  for (auto&& l_path : FSys::directory_iterator{l_shot_path_dir}) {
    auto l_stem = l_path.path().stem().string();
    if (!(l_stem.starts_with(l_shot_file_name) && l_path.path().extension() == ".fbx")) continue;

    if (l_sim_output_key.contains(l_stem)) continue;
    if (auto l_cam = l_stem.find("_camera_"); l_cam != std::string::npos) {
      l_ret.camera_file_path_ = l_path.path();
      // 名称形式 _camera_0001-0240
      l_ret.begin_time_       = std::stoll(l_stem.substr(l_cam + 8, 4));   // "_camera_" 长度8, 时间长度 4
      l_ret.end_time_         = std::stoll(l_stem.substr(l_cam + 13, 4));  // "_camera_" 长度8, 时间长度 4 加 '-' = 13
      continue;
    }

    l_ret.asset_infos_.emplace_back(
        run_ue_assembly_local::run_ue_assembly_asset_info{
            .shot_output_path_ = l_path.path(), .type_ = l_path.path().extension() == ".fbx" ? "char" : "geo"
        }
    );
  }
  if (l_ret.camera_file_path_.empty())
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request, "未找到镜头(camera)文件，请确保镜头文件已上传至服务器"
        }
    );
  if (l_ret.asset_infos_.empty())
    throw_exception(
        http_request_error{boost::beast::http::status::bad_request, "未找到资产文件，请确保资产文件已上传至服务器"}
    );
  std::map<std::string, std::vector<std::size_t>> l_asset_infos_key_map{};
  for (std::size_t i = 0; i < l_ret.asset_infos_.size(); ++i) {
    auto&& l_info = l_ret.asset_infos_[i];
    auto l_stem   = l_info.shot_output_path_.stem().string();
    auto l_key    = l_stem.substr(l_shot_file_name.size() + 1);  // add '_'
    // find _rig and _Low
    if (auto l_rig_post = l_key.find("_rig"); l_rig_post != std::string::npos) {
      l_key = l_key.substr(0, l_rig_post);
    } else if (auto l_low_post = l_key.find("_Low"); l_low_post != std::string::npos) {
      l_key = l_key.substr(0, l_low_post + 4);
    }
    l_asset_infos_key_map[l_key].emplace_back(i);
  }

  auto l_assets = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<entity_asset_extend>(true)), from<entity>(),
      left_outer_join<entity_asset_extend>(on(c(&entity_asset_extend::entity_id_) == c(&entity::uuid_id_))),
      where(in(
          &entity::uuid_id_, select(
                                 &entity_link::entity_out_id_, from<entity_link>(),
                                 join<shot>(on(c(&entity_link::entity_in_id_) == c(shot->*&entity::uuid_id_))),
                                 join<sequence>(on(c(shot->*&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
                                 where(c(shot->*&entity::uuid_id_) == l_shot_entity.uuid_id_)
                             )
      ))
  );
  FSys::path l_scene_ue_path{core_set::get_set().get_cache_root().parent_path()};
  /// 寻找主场景资产, 并生成对应的本地ue资产路径
  if (auto l_scene_it = std::find_if(
          l_assets.begin(), l_assets.end(),
          [](const auto& in_pair) { return std::get<0>(in_pair).entity_type_id_ == asset_type::get_ground_id(); }
      );
      l_scene_it == l_assets.end()) {
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request, "镜头关联的资产中未找到场景类型资产，无法生成 ue 主工程路径"
        }
    );
  } else {
    auto&& [l_scene_asset, l_scene_asset_extend] = *l_scene_it;
    const auto l_suffix                          = l_is_simulation_task ? "_JS" : "_DH";
    if (l_scene_asset_extend.gui_dang_ && l_scene_asset_extend.kai_shi_ji_shu_) {
      auto l_ue_main_map = l_prj.path_ / get_entity_ground_ue_path(l_prj, l_scene_asset_extend) /
                           get_entity_ground_ue_map_name(l_scene_asset_extend);
      l_ret.original_map_          = conv_ue_game_path(get_entity_ground_ue_map_name(l_scene_asset_extend));
      l_ret.movie_pipeline_config_ = fmt::format(
          "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/{0}_EP{1:03}_SC{2:03}{3}_Config", l_prj.code_,
          l_episodes.get_episodes(), l_shot.get_shot(), l_shot.get_shot_ab()
      );
      l_ret.level_sequence_import_ = fmt::format(
          "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/Import{4}/{0}_EP{1:03}_SC{2:03}{3}{4}", l_prj.code_,
          l_episodes.get_episodes(), l_shot.get_shot(), l_shot.get_shot_ab(), l_suffix
      );
      l_ret.create_map_ = fmt::format(
          "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/Import{4}/{0}_EP{1:03}_SC{2:03}{3}_LV", l_prj.code_,
          l_episodes.get_episodes(), l_shot.get_shot(), l_shot.get_shot_ab(), l_suffix
      );
      l_ret.import_dir_ = fmt::format(
          "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/Import{4}/files/", l_prj.code_, l_episodes.get_episodes(),
          l_shot.get_shot(), l_shot.get_shot_ab(), l_suffix
      );
      l_ret.render_map_ = fmt::format(
          "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}{3}/Import{4}/sc{2:03}{3}{4}", l_prj.code_, l_episodes.get_episodes(),
          l_shot.get_shot(), l_shot.get_shot_ab(), l_suffix
      );
      auto&& l_uprj = ue_exe_ns::find_ue_project_file(l_ue_main_map);
      l_scene_ue_path /= l_prj.code_ / l_uprj.stem();

      l_ret.ue_main_project_path_ = l_scene_ue_path / l_uprj.filename();
      l_ret.out_file_dir_ =
          l_scene_ue_path / doodle_config::ue4_saved / doodle_config::ue4_movie_renders /
          fmt::format(
              "{}_EP{:03}_SC{:03}{}", l_prj.code_, l_episodes.get_episodes(), l_shot.get_shot(), l_shot.get_shot_ab()
          );
      l_ret.create_move_path_ =
          l_ret.out_file_dir_.parent_path() / l_ret.out_file_dir_.filename().replace_extension(".mp4");

      l_ret.ue_asset_path_.emplace_back(l_uprj, l_scene_ue_path / l_uprj.filename());
      l_ret.ue_asset_path_.emplace_back(
          l_prj.path_ / get_entity_ground_ue_path(l_prj, l_scene_asset_extend) / doodle_config::ue4_content,
          l_scene_ue_path / doodle_config::ue4_content
      );
      l_ret.ue_asset_path_.emplace_back(
          l_prj.path_ / get_entity_ground_ue_path(l_prj, l_scene_asset_extend) / doodle_config::ue4_config,
          l_scene_ue_path / doodle_config::ue4_config
      );
      l_ret.update_ue_path_.emplace_back(
          l_scene_ue_path / doodle_config::ue4_content, FSys::path{l_prj.auto_upload_path_} /
                                                            fmt::format("{}_{}", l_ret.episodes_, l_ret.shot_) /
                                                            l_scene_ue_path.stem() / doodle_config::ue4_content

      );
      l_ret.update_ue_path_.emplace_back(
          l_scene_ue_path / doodle_config::ue4_saved / doodle_config::ue4_movie_renders,
          FSys::path{l_prj.auto_upload_path_} / fmt::format("EP{}_SC{}", l_ret.episodes_, l_ret.shot_) /
              l_scene_ue_path.stem() / doodle_config::ue4_saved / doodle_config::ue4_movie_renders

      );
    } else
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request,
              fmt::format("资产 {} 缺少归档或开始集信息，无法生成 ue 主工程路径", l_scene_asset.name_)
          }
      );
  }

  for (auto&& [l_asset, l_asset_extend] : l_assets) {
    if (l_asset.entity_type_id_ == asset_type::get_character_id()) {
      auto l_key = fmt::format("Ch{}", l_asset_extend.bian_hao_);
      if (l_asset_infos_key_map.contains(l_key)) {
        if (l_asset_extend.gui_dang_ && l_asset_extend.kai_shi_ji_shu_) {
          for (auto&& l_idx : l_asset_infos_key_map[l_key]) {
            l_ret.asset_infos_[l_idx].skin_path_ = get_entity_character_ue_name(l_asset_extend);

            l_ret.ue_asset_path_.emplace_back(
                l_prj.path_ / get_entity_character_ue_path(l_prj, l_asset_extend) / doodle_config::ue4_content,
                l_scene_ue_path / doodle_config::ue4_content
            );
          }
        } else
          throw_exception(
              http_request_error{
                  boost::beast::http::status::bad_request,
                  fmt::format("资产 {} 缺少归档或开始集信息，无法生成 ue 资产路径", l_asset.name_)
              }
          );
      }

    } else if (l_asset.entity_type_id_ == asset_type::get_prop_id() ||
               l_asset.entity_type_id_ == asset_type::get_effect_id()) {
      auto l_key = fmt::format(
          "{}{}{}", l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_.empty() ? "" : "_",
          l_asset_extend.ban_ben_
      );
      if (l_asset_infos_key_map.contains(l_key)) {
        if (l_asset_extend.gui_dang_ && l_asset_extend.kai_shi_ji_shu_) {
          for (auto&& l_idx : l_asset_infos_key_map[l_key]) {
            l_ret.asset_infos_[l_idx].skin_path_ = get_entity_prop_ue_name(
                l_asset_extend.bian_hao_, l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_
            );

            l_ret.ue_asset_path_.emplace_back(
                l_prj.path_ / get_entity_prop_ue_path(l_prj, l_asset_extend) / get_entity_prop_ue_public_files_path(),
                l_scene_ue_path / get_entity_prop_ue_public_files_path()
            );
            l_ret.ue_asset_path_.emplace_back(
                l_prj.path_ / get_entity_prop_ue_path(l_prj, l_asset_extend) /
                    get_entity_prop_ue_files_path(l_asset_extend),
                l_scene_ue_path / get_entity_prop_ue_files_path(l_asset_extend)
            );
          }
        } else
          throw_exception(
              http_request_error{
                  boost::beast::http::status::bad_request,
                  fmt::format("资产 {} 缺少归档或开始集信息，无法生成 ue 资产路径", l_asset.name_)
              }
          );
      }

    } else if (l_asset.entity_type_id_ == asset_type::get_ground_id()) {
      auto l_key = fmt::format(
          "{}{}{}_Low", l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_.empty() ? "" : "_",
          l_asset_extend.ban_ben_
      );
      if (l_asset_infos_key_map.contains(l_key)) {
        for (auto&& l_idx : l_asset_infos_key_map[l_key])
          l_ret.asset_infos_[l_idx].skin_path_ =
              get_entity_ground_ue_sk_name(l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_);
      }

    } else {
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request,
              fmt::format("不支持的资产类型: {}", l_sql.get_by_uuid<asset_type>(l_asset.entity_type_id_).name_)
          }
      );
    }
  }

  for (auto&& l_info : l_ret.asset_infos_) {
    if (l_info.skin_path_.empty()) {
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request,
              fmt::format("无法为输出文件 {} 生成对应的 ue 资产路径", l_info.shot_output_path_.string())
          }
      );
    }
    l_info.skin_path_ = conv_ue_game_path(l_info.skin_path_);
    sk_conv_bone_name(l_info.skin_path_);
  }
#ifdef NDEBUG
  for (auto&& l_path : l_ret.ue_asset_path_) {
    if (!FSys::exists(l_path.from_)) {
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request, fmt::format("UE 资产源路径不存在: {}", l_path.from_.string())
          }
      );
    }
  }
#endif
  co_return in_handle->make_msg(nlohmann::json{} = l_ret);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_export_rig_sk::get(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = l_sql.get_by_uuid<task>(task_id_);
  if (l_task.task_type_id_ != task_type::get_binding_id())
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "只有绑定任务才支持导出 rig sk");
  auto l_asset         = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_asset_extends = l_sql.get_entity_asset_extend(l_asset.uuid_id_);
  auto l_prj           = l_sql.get_by_uuid<project>(l_asset.project_id_);
  if (!l_asset_extends)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "实体缺少资产扩展信息");
  export_rig_sk_arg::data_t l_arg{};
  auto l_ue_scene_path = core_set::get_set().get_cache_root().parent_path() / l_prj.code_;
  if (l_asset.entity_type_id_ == asset_type::get_character_id()) {
    auto l_ue_name          = get_entity_character_ue_name(*l_asset_extends);
    auto l_ue_path          = get_entity_character_ue_path(l_prj, *l_asset_extends);
    auto l_ue_project       = ue_exe_ns::find_ue_project_file(l_prj.path_ / l_ue_path);
    l_arg.import_game_path_ = conv_ue_game_path(l_ue_name);
    l_arg.ue_asset_copy_path_.emplace_back(
        l_prj.path_ / l_ue_path / doodle_config::ue4_content, l_ue_scene_path / doodle_config::ue4_content
    );
    l_arg.ue_asset_copy_path_.emplace_back(
        l_prj.path_ / l_ue_path / doodle_config::ue4_config, l_ue_scene_path / doodle_config::ue4_config
    );
    l_arg.ue_asset_copy_path_.emplace_back(l_ue_project, l_ue_scene_path / l_ue_project.filename());
    l_arg.ue_project_path_ = l_ue_scene_path / l_ue_project.filename();
    l_arg.update_ue_path_  = l_ue_scene_path / l_ue_name.parent_path();

  } else if (l_asset.entity_type_id_ == asset_type::get_prop_id() ||
             l_asset.entity_type_id_ == asset_type::get_effect_id()) {
    auto l_ue_name          = get_entity_prop_ue_name(*l_asset_extends);
    auto l_ue_path          = get_entity_prop_ue_path(l_prj, *l_asset_extends);
    auto l_ue_project       = ue_exe_ns::find_ue_project_file(l_prj.path_ / l_ue_path);
    l_arg.import_game_path_ = conv_ue_game_path(l_ue_name);
    l_arg.ue_asset_copy_path_.emplace_back(
        l_prj.path_ / l_ue_path / get_entity_prop_ue_public_files_path(),
        l_ue_scene_path / get_entity_prop_ue_public_files_path()
    );
    l_arg.ue_asset_copy_path_.emplace_back(
        l_prj.path_ / l_ue_path / get_entity_prop_ue_files_path(*l_asset_extends),
        l_ue_scene_path / get_entity_prop_ue_files_path(*l_asset_extends)
    );
    l_arg.ue_asset_copy_path_.emplace_back(l_ue_project, l_ue_scene_path / l_ue_project.filename());
    l_arg.ue_project_path_ = l_ue_scene_path / l_ue_project.filename();
    l_arg.update_ue_path_  = l_ue_scene_path / l_ue_name.parent_path();

  } else if (l_asset.entity_type_id_ == asset_type::get_ground_id()) {
    auto l_ue_name    = get_entity_ground_ue_sk_name(l_asset_extends->pin_yin_ming_cheng_, l_asset_extends->ban_ben_);
    auto l_ue_path    = get_entity_ground_ue_path(l_prj, *l_asset_extends);
    auto l_ue_project = ue_exe_ns::find_ue_project_file(l_prj.path_ / l_ue_path);
    l_arg.import_game_path_ = conv_ue_game_path(l_ue_name);
    l_arg.ue_asset_copy_path_.emplace_back(
        l_prj.path_ / l_ue_path / doodle_config::ue4_content, l_ue_scene_path / doodle_config::ue4_content
    );
    l_arg.ue_asset_copy_path_.emplace_back(
        l_prj.path_ / l_ue_path / doodle_config::ue4_config, l_ue_scene_path / doodle_config::ue4_config
    );
    l_arg.ue_asset_copy_path_.emplace_back(l_ue_project, l_ue_scene_path / l_ue_project.filename());
    l_arg.ue_project_path_ = l_ue_scene_path / l_ue_project.filename();
    l_arg.update_ue_path_  = l_ue_scene_path / l_ue_name.parent_path();

  } else {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, "仅支持角色和道具, 地编, 特效类型资产导出 rig sk"
    );
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_arg);
}

}  // namespace doodle::http