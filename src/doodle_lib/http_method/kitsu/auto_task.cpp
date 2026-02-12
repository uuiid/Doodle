//
// Created by TD on 24-12-30.
//

#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/episodes.h"
#include "doodle_core/metadata/shot.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/entity_path.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/exe_warp/export_fbx_arg.h>
#include <doodle_lib/exe_warp/export_rig_sk.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/task_sync.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <boost/asio/awaitable.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fmt/format.h>
#include <map>
#include <range/v3/view/unique.hpp>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <vector>

namespace doodle::http {
namespace {
struct sim_map_anim {
  std::size_t to_fbx_idx_{};
  std::size_t to_abc_idx_{};
};
// 检查是否存在多个场景
auto check_multiple_scene(auto& in_vector) {
  auto l_r = std::count_if(in_vector.begin(), in_vector.end(), [](const auto& in_info) -> bool {
    return std::get<0>(in_info).entity_type_id_ == asset_type::get_ground_id();
  });
  if (l_r == 0)
    throw_exception(
        http_request_error{boost::beast::http::status::bad_request, "未找到场景类型资产，无法生成 ue 主工程路径"}
    );

  // 大于两个场景的情况下, 擦除多余场景, 直到只剩一个场景
  while (l_r > 1) {
    if (auto l_it = std::find_if(
            in_vector.begin(), in_vector.end(),
            [](const auto& in_pair) { return std::get<0>(in_pair).entity_type_id_ == asset_type::get_ground_id(); }
        );
        l_it != in_vector.end()) {
      in_vector.erase(l_it);
      --l_r;
    }
  }

  return std::find_if(in_vector.begin(), in_vector.end(), [](const auto& in_pair) {
    return std::get<0>(in_pair).entity_type_id_ == asset_type::get_ground_id();
  });
}

}  // namespace
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

  bool l_is_simulation_task = l_shot_task.task_type_id_ == task_type::get_simulation_task_id();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 开始生成 UE 装配参数 project_id {} task_id {} shot_entity_id {} episode_entity_id {} is_simulation {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_shot_task.uuid_id_, l_shot_entity.uuid_id_,
      l_episode_entity.uuid_id_, l_is_simulation_task
  );

  episodes l_episodes{l_episode_entity};
  shot l_shot{l_shot_entity};

  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();

  run_ue_assembly_local::run_ue_assembly_arg l_ret{};
  l_ret.episodes_ = l_episodes;
  l_ret.shot_     = l_shot;

  l_ret.size_     = l_prj.get_resolution();
  FSys::path l_shot_path_dir{};
  FSys::path l_sim_shot_path_dir{};
  std::set<std::string> l_sim_output_key{};
  /// tag: 格式化路径
  l_shot_path_dir           = get_shots_animation_output_path(l_episode_entity.name_, l_shot_entity.name_, l_prj.code_);
  if (l_is_simulation_task)
    l_sim_shot_path_dir = get_shots_simulation_output_path(l_episode_entity.name_, l_shot_entity.name_, l_prj.code_);

  l_shot_path_dir = l_prj.path_ / l_shot_path_dir;
  if (l_is_simulation_task) l_sim_shot_path_dir = l_prj.path_ / l_sim_shot_path_dir;
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
              .shot_output_path_ = l_path.path(),
              .type_             = l_path.path().extension() == ".fbx" ? run_ue_assembly_local::import_ue_type::char_
                                                                       : run_ue_assembly_local::import_ue_type::geo
          }
      );

      if (l_stem.find("_cloth") != std::string::npos) l_ret.asset_infos_.back().simulation_type_.set(0);
      if (l_stem.find("_hair") != std::string::npos) l_ret.asset_infos_.back().simulation_type_.set(1);

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
            .shot_output_path_ = l_path.path(),
            .type_             = l_path.path().extension() == ".fbx" ? run_ue_assembly_local::import_ue_type::char_
                                                                     : run_ue_assembly_local::import_ue_type::geo
        }
    );
  }
  if (l_ret.camera_file_path_.empty())
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request, "未找到镜头(camera)文件，请确保镜头文件已上传至服务器"
        }
    );

  // 构建对应的寻找 sk 的 key map
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
  /// 如果是解算, 还需要构建对应的解算 abc 文件和 fbx 文件对, 并将 fbx 的 simulation_type_ 赋值给 char_ 类型的
  /// asset_info
  if (l_is_simulation_task) {
    for (std::size_t i = 0; i < l_ret.asset_infos_.size(); ++i) {
      if (l_ret.asset_infos_[i].type_ != run_ue_assembly_local::import_ue_type::geo) continue;

      auto&& l_info = l_ret.asset_infos_[i];
      auto l_stem   = l_info.shot_output_path_.stem().string();
      // remove _cloth or _hair
      if (l_info.simulation_type_.test(0)) {
        auto l_cloth_pos = l_stem.find("_cloth");
        if (l_cloth_pos != std::string::npos) {
          l_stem = l_stem.erase(l_cloth_pos, 6);
        }
      }
      if (l_info.simulation_type_.test(1)) {
        auto l_hair_pos = l_stem.find("_hair");
        if (l_hair_pos != std::string::npos) {
          l_stem = l_stem.erase(l_hair_pos, 5);
        }
      }

      if (auto l_it = std::ranges::find_if(
              l_ret.asset_infos_,
              [&](const auto& in_info) {
                if (in_info.type_ != run_ue_assembly_local::import_ue_type::char_) return false;
                auto l_char_stem = in_info.shot_output_path_.stem().string();
                return l_char_stem == l_stem;
              }
          );
          l_it != l_ret.asset_infos_.end()) {
        l_it->simulation_type_ |= l_info.simulation_type_;
      }
    }
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
  FSys::path l_scene_ue_path{"D:/sy_magic/ue_projects/"};  // 默认路径
  /// 寻找主场景资产, 并生成对应的本地ue资产路径

  auto&& [l_scene_asset, l_scene_asset_extend] = *check_multiple_scene(l_assets);
  DOODLE_CHICK_HTTP(l_scene_asset_extend.gui_dang_, bad_request, "场景资产 {} 缺少扩展信息 归档", l_scene_asset.name_);
  DOODLE_CHICK_HTTP(
      l_scene_asset_extend.kai_shi_ji_shu_, bad_request, "场景资产 {} 缺少扩展信息 开始集数", l_scene_asset.name_
  );
  const auto l_suffix = l_is_simulation_task ? "_JS" : "_DH";
  auto l_ue_main_map  = l_prj.path_ / get_entity_ground_ue_path(l_prj, l_scene_asset_extend) /
                       get_entity_ground_ue_map_name(l_scene_asset_extend);
  l_ret.original_map_ = conv_ue_game_path(get_entity_ground_ue_map_name(l_scene_asset_extend));
  l_ret.clear_path_ =
      fmt::format("{3}/Shot/ep{1:04}/{0}{1:03}_sc{2:03}", l_prj.code_, l_episodes, l_shot, doodle_config::ue4_content);
  l_ret.movie_pipeline_config_ = fmt::format(
      "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}/{0}_EP{1:03}_SC{2:03}_Config", l_prj.code_, l_episodes, l_shot
  );
  l_ret.level_sequence_import_ = fmt::format(
      "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}/Import{3}/{0}_EP{1:03}_SC{2:03}{3}", l_prj.code_, l_episodes, l_shot,
      l_suffix
  );
  l_ret.create_map_ = fmt::format(
      "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}/Import{3}/{0}_EP{1:03}_SC{2:03}{3}_LV", l_prj.code_, l_episodes, l_shot,
      l_suffix
  );
  l_ret.import_dir_ =
      fmt::format("/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}/Import{3}/files/", l_prj.code_, l_episodes, l_shot, l_suffix);
  l_ret.render_map_ = fmt::format(
      "/Game/Shot/ep{1:04}/{0}{1:03}_sc{2:03}/Import{3}/sc{2:03}{3}", l_prj.code_, l_episodes, l_shot, l_suffix
  );
  auto&& l_uprj = ue_exe_ns::find_ue_project_file(l_ue_main_map);
  if (l_uprj.empty())
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request, "未找到场景 {} 对应的 ue 工程文件，无法生成 ue 主工程路径",
            l_scene_asset.name_
        }
    );
  l_scene_ue_path /= l_prj.code_;
  l_scene_ue_path /= fmt::format("EP{:04}", l_episodes) / l_uprj.stem();

  l_ret.ue_main_project_path_ = l_scene_ue_path / l_uprj.filename();
  l_ret.out_file_dir_         = l_scene_ue_path / doodle_config::ue4_saved / doodle_config::ue4_movie_renders /
                        fmt::format("{}_EP{:03}_SC{:03}", l_prj.code_, l_episodes, l_shot);
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
      l_scene_ue_path / doodle_config::ue4_content, l_prj.path_ / "03_Workflow" / "Shot" /
                                                        fmt::format("EP{:04}", l_ret.episodes_) /
                                                        l_scene_ue_path.stem() / doodle_config::ue4_content

  );
  l_ret.update_ue_path_.emplace_back(
      l_scene_ue_path / doodle_config::ue4_config, l_prj.path_ / "03_Workflow" / "Shot" /
                                                       fmt::format("EP{:04}", l_ret.episodes_) /
                                                       l_scene_ue_path.stem() / doodle_config::ue4_config

  );
  l_ret.update_ue_path_.emplace_back(
      l_scene_ue_path / l_uprj.filename(), l_prj.path_ / "03_Workflow" / "Shot" /
                                               fmt::format("EP{:04}", l_ret.episodes_) / l_scene_ue_path.stem() /
                                               l_uprj.filename()

  );
  l_ret.update_ue_path_.emplace_back(
      l_scene_ue_path / doodle_config::ue4_saved / doodle_config::ue4_movie_renders,
      FSys::path{l_prj.auto_upload_path_} / fmt::format("EP{:03}", l_ret.episodes_) / "自动灯光序列"

  );

  for (auto&& [l_asset, l_asset_extend] : l_assets) {
    if (l_asset.entity_type_id_ == asset_type::get_character_id()) {
      auto l_key = fmt::format("Ch{}", l_asset_extend.bian_hao_);
      if (l_asset_infos_key_map.contains(l_key)) {
        if (l_asset_extend.gui_dang_ && l_asset_extend.kai_shi_ji_shu_) {
          for (auto&& l_idx : l_asset_infos_key_map[l_key]) {
            if (l_ret.asset_infos_[l_idx].type_ == run_ue_assembly_local::import_ue_type::char_)
              l_ret.asset_infos_[l_idx].skin_path_ =
                  l_ret.asset_infos_[l_idx].simulation_type_.any()
                      ? get_entity_sim_character_ue_name(l_asset_extend, l_ret.asset_infos_[l_idx].simulation_type_)
                      : get_entity_character_ue_name(l_asset_extend);
            l_ret.asset_infos_[l_idx].ue_project_dir_ =
                l_prj.path_ / get_entity_character_ue_path(l_prj, l_asset_extend);
            l_ret.ue_asset_path_.emplace_back(
                l_prj.path_ / get_entity_character_ue_path(l_prj, l_asset_extend) / doodle_config::ue4_content,
                l_scene_ue_path / doodle_config::ue4_content
            );
          }
        } else
          throw_exception(
              http_request_error{
                  boost::beast::http::status::bad_request, "资产 {} 缺少归档或开始集信息，无法生成 ue 资产路径",
                  l_asset.name_
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
            l_ret.asset_infos_[l_idx].skin_path_ =
                l_ret.asset_infos_[l_idx].simulation_type_.any()
                    ? get_entity_sim_prop_ue_name(l_asset_extend, l_ret.asset_infos_[l_idx].simulation_type_)
                    : get_entity_prop_ue_name(
                          l_asset_extend.bian_hao_, l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_
                      );
            l_ret.asset_infos_[l_idx].ue_project_dir_ = l_prj.path_ / get_entity_prop_ue_path(l_prj, l_asset_extend);
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
                  boost::beast::http::status::bad_request, "资产 {} 缺少归档或开始集信息，无法生成 ue 资产路径",
                  l_asset.name_
              }
          );
      }

    } else if (l_asset.entity_type_id_ == asset_type::get_ground_id()) {
      auto l_key = fmt::format(
          "{}{}{}_Low", l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_.empty() ? "" : "_",
          l_asset_extend.ban_ben_
      );
      if (l_asset_infos_key_map.contains(l_key)) {
        for (auto&& l_idx : l_asset_infos_key_map[l_key]) {
          l_ret.asset_infos_[l_idx].skin_path_ =
              l_ret.asset_infos_[l_idx].simulation_type_.any()
                  ? get_entity_sim_ground_ue_sk_name(l_asset_extend, l_ret.asset_infos_[l_idx].simulation_type_)
                  : get_entity_ground_ue_sk_name(l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_);
          l_ret.asset_infos_[l_idx].ue_project_dir_ = l_prj.path_ / get_entity_ground_ue_path(l_prj, l_asset_extend);
        }
      }

    } else {
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request, "不支持的资产类型: {}",
              l_sql.get_by_uuid<asset_type>(l_asset.entity_type_id_).name_
          }
      );
    }
  }

#ifdef NDEBUG
  for (auto&& l_path : l_ret.ue_asset_path_) {
    DOODLE_CHICK_HTTP(!l_path.from_.empty(), bad_request, "UE 场景可能没有启动器等原因");
    DOODLE_CHICK_HTTP(FSys::exists(l_path.from_), bad_request, "UE 资产源路径不存在: {}", l_path.from_.string())
  }
  for (auto&& l_info : l_ret.asset_infos_) {
    DOODLE_CHICK_HTTP(
        !(l_info.type_ == run_ue_assembly_local::import_ue_type::char_ &&
          !FSys::exists(l_info.ue_project_dir_ / l_info.skin_path_)),
        bad_request, "无法找到输出文件 {} 生成对应的 ue 资产路径: {}", l_info.shot_output_path_.string(),
        (l_info.ue_project_dir_ / l_info.skin_path_).string()

    )
  }
#endif

  for (auto&& l_info : l_ret.asset_infos_) {
    DOODLE_CHICK_HTTP(
        !(l_info.skin_path_.empty() && l_info.type_ == run_ue_assembly_local::import_ue_type::char_), bad_request,
        "无法为输出文件 {} 生成对应的 ue 资产路径", l_info.shot_output_path_.string()
    );
    l_info.skin_path_ = conv_ue_game_path(l_info.skin_path_);
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "用户 {}({}) 完成生成 UE 装配参数 project_id {} task_id {} asset_count {} camera_file {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_shot_task.uuid_id_, l_ret.asset_infos_.size(),
      l_ret.camera_file_path_.filename().generic_string()
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_ret);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_export_rig_sk::get(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = l_sql.get_by_uuid<task>(task_id_);
  if (l_task.task_type_id_ != task_type::get_binding_id() && l_task.task_type_id_ != task_type::get_simulation_id())
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "只有绑定任务才支持导出 rig sk");
  auto l_asset         = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_asset_extends = l_sql.get_entity_asset_extend(l_asset.uuid_id_);
  auto l_prj           = l_sql.get_by_uuid<project>(l_asset.project_id_);
  if (!l_asset_extends)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "实体缺少资产扩展信息");
  export_rig_sk_arg::data_t l_arg{};
  auto l_ue_scene_path = FSys::path{"D:/sy_magic/ue_projects"} / doodle_config::doodle_token_name / l_prj.code_;
  bool l_is_sim        = l_task.task_type_id_ == task_type::get_simulation_id();
  if (l_asset.entity_type_id_ == asset_type::get_character_id()) {
    auto l_ue_name    = get_entity_character_ue_name(*l_asset_extends);
    auto l_ue_path    = get_entity_character_ue_path(l_prj, *l_asset_extends);
    auto l_ue_project = ue_exe_ns::find_ue_project_file(l_prj.path_ / l_ue_path);
    if (l_ue_project.empty())
      co_return in_handle->make_error_code_msg(
          boost::beast::http::status::bad_request,
          fmt::format("未找到角色 {} 对应的 ue 工程文件，无法生成 ue 资产路径", l_asset.name_)
      );
    auto l_maya_file_name = l_is_sim ? get_entity_simulation_character_asset_name(*l_asset_extends)
                                     : get_entity_character_rig_maya_name(*l_asset_extends);

    if (!l_is_sim) {
      l_arg.rename_map_.emplace(l_maya_file_name.stem().generic_string(), l_ue_name.stem().generic_string());
    } else {
      auto l_name        = l_maya_file_name.stem().generic_string();
      auto l_ue_name_sim = l_ue_name.stem().generic_string();
      l_arg.rename_map_.emplace(fmt::format("{}_cloth", l_name), fmt::format("{}_cloth", l_ue_name_sim));
      l_arg.rename_map_.emplace(fmt::format("{}_hair", l_name), fmt::format("{}_hair", l_ue_name_sim));
      l_arg.rename_map_.emplace(fmt::format("{}_cloth_hair", l_name), fmt::format("{}_cloth_hair", l_ue_name_sim));
    }

    l_ue_scene_path /= l_ue_project.stem();
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
    auto l_ue_name    = get_entity_prop_ue_name(*l_asset_extends);
    auto l_ue_path    = get_entity_prop_ue_path(l_prj, *l_asset_extends);
    auto l_ue_project = ue_exe_ns::find_ue_project_file(l_prj.path_ / l_ue_path);
    if (l_ue_project.empty())
      co_return in_handle->make_error_code_msg(
          boost::beast::http::status::bad_request,
          fmt::format("未找到道具 {} 对应的 ue 工程文件，无法生成 ue 资产路径", l_asset.name_)
      );

    auto l_maya_file_name = l_is_sim ? get_entity_simulation_prop_asset_name(*l_asset_extends)
                                     : get_entity_prop_rig_maya_name(*l_asset_extends);

    if (!l_is_sim) {
      l_arg.rename_map_.emplace(l_maya_file_name.stem().generic_string(), l_ue_name.stem().generic_string());
    } else {
      auto l_name        = l_maya_file_name.stem().generic_string();
      auto l_ue_name_sim = l_ue_name.stem().generic_string();
      l_arg.rename_map_.emplace(fmt::format("{}_cloth", l_name), fmt::format("{}_cloth", l_ue_name_sim));
      l_arg.rename_map_.emplace(fmt::format("{}_hair", l_name), fmt::format("{}_hair", l_ue_name_sim));
      l_arg.rename_map_.emplace(fmt::format("{}_cloth_hair", l_name), fmt::format("{}_cloth_hair", l_ue_name_sim));
    }

    l_ue_scene_path /= l_ue_project.stem();
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
    auto l_ue_name = get_entity_ground_ue_sk_name(l_asset_extends->pin_yin_ming_cheng_, l_asset_extends->ban_ben_);
    auto l_ue_path = get_entity_ground_ue_path(l_prj, *l_asset_extends);

    auto l_maya_file_name = get_entity_ground_rig_name(*l_asset_extends);
    if (!l_is_sim) {
      l_arg.rename_map_.emplace(l_maya_file_name.stem().generic_string(), l_ue_name.stem().generic_string());
    } else {
      auto l_name        = l_maya_file_name.stem().generic_string();
      auto l_ue_name_sim = l_ue_name.stem().generic_string();
      l_arg.rename_map_.emplace(fmt::format("{}_cloth", l_name), fmt::format("{}_cloth", l_ue_name_sim));
      l_arg.rename_map_.emplace(fmt::format("{}_hair", l_name), fmt::format("{}_hair", l_ue_name_sim));
      l_arg.rename_map_.emplace(fmt::format("{}_cloth_hair", l_name), fmt::format("{}_cloth_hair", l_ue_name_sim));
    }

    auto l_ue_project = ue_exe_ns::find_ue_project_file(l_prj.path_ / l_ue_path);
    if (l_ue_project.empty())
      co_return in_handle->make_error_code_msg(
          boost::beast::http::status::bad_request,
          fmt::format("未找到地编 {} 对应的 ue 工程文件，无法生成 ue 资产路径", l_asset.name_)
      );

    l_ue_scene_path /= l_ue_project.stem();
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

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_export_anim_fbx::get(
    session_data_ptr in_handle
) {
  auto l_sql            = g_ctx().get<sqlite_database>();
  auto l_task           = l_sql.get_by_uuid<task>(task_id_);
  auto l_proj           = l_sql.get_by_uuid<project>(l_task.project_id_);
  auto l_entity         = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_episode_entity = l_sql.get_by_uuid<entity>(l_entity.parent_id_);
  episodes l_episodes{l_episode_entity};
  shot l_shot{l_entity};

  export_fbx_arg::get_export_fbx_arg l_arg{};
  l_arg.movie_file_ = get_shots_animation_file_name(l_episode_entity.name_, l_entity.name_, l_proj.code_);
  l_arg.movie_file_.replace_extension(".mp4");
  l_arg.film_aperture_ = l_proj.get_film_aperture();
  l_arg.size_          = l_proj.get_resolution();
  co_return in_handle->make_msg(nlohmann::json{} = l_arg);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_sync::get(session_data_ptr in_handle) {
  auto l_sql            = g_ctx().get<sqlite_database>();
  auto l_task           = l_sql.get_by_uuid<task>(task_id_);
  auto l_shot_entity    = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  auto l_episode_entity = l_sql.get_by_uuid<entity>(l_shot_entity.parent_id_);
  shot l_shot{l_shot_entity};
  episodes l_episodes{l_episode_entity};
  auto l_prj = l_sql.get_by_uuid<project>(l_task.project_id_);

  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();
  auto l_assets           = l_sql.impl_->storage_any_.select(
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
  /// 寻找主场景资产, 并生成对应的本地ue资产路径
  task_sync::args l_arg{};
  auto&& [l_scene_asset, l_scene_asset_extend] = *check_multiple_scene(l_assets);
  DOODLE_CHICK_HTTP(l_scene_asset_extend.gui_dang_, bad_request, "场景资产 {} 缺少扩展信息 归档", l_scene_asset.name_);
  DOODLE_CHICK_HTTP(
      l_scene_asset_extend.kai_shi_ji_shu_, bad_request, "场景资产 {} 缺少扩展信息 开始集数", l_scene_asset.name_
  );

  auto l_ue_main_map = l_prj.path_ / get_entity_ground_ue_path(l_prj, l_scene_asset_extend) /
                       get_entity_ground_ue_map_name(l_scene_asset_extend);
  auto&& l_uprj = ue_exe_ns::find_ue_project_file(l_ue_main_map);
  DOODLE_CHICK_HTTP(
      !l_uprj.empty(), bad_request, "未找到场景 {} 对应的 ue 工程文件，无法生成 ue 主工程路径", l_scene_asset.name_
  );

  auto l_scene_ue_path = FSys::path{l_prj.code_} / fmt::format("EP{:04}", l_episodes) / l_uprj.stem();

  /// 添加场景文件下载
  l_arg.download_file_list_.emplace_back(l_uprj, l_scene_ue_path / l_uprj.filename());
  l_arg.download_file_list_.emplace_back(
      l_prj.path_ / get_entity_ground_ue_path(l_prj, l_scene_asset_extend) / doodle_config::ue4_content,
      l_scene_ue_path / doodle_config::ue4_content
  );
  l_arg.download_file_list_.emplace_back(
      l_prj.path_ / get_entity_ground_ue_path(l_prj, l_scene_asset_extend) / doodle_config::ue4_config,
      l_scene_ue_path / doodle_config::ue4_config
  );

  for (auto&& [l_asset, l_asset_extend] : l_assets) {
    if (l_asset.entity_type_id_ == asset_type::get_character_id()) {
      DOODLE_CHICK_HTTP(l_asset_extend.gui_dang_, bad_request, "资产 {} 缺少扩展信息 归档", l_asset.name_);
      DOODLE_CHICK_HTTP(l_asset_extend.kai_shi_ji_shu_, bad_request, "资产 {} 缺少扩展信息 开始集数", l_asset.name_);
      l_arg.download_file_list_.emplace_back(
          l_prj.path_ / get_entity_character_ue_path(l_prj, l_asset_extend) / doodle_config::ue4_content,
          l_scene_ue_path / doodle_config::ue4_content
      );

    } else if (l_asset.entity_type_id_ == asset_type::get_prop_id() ||
               l_asset.entity_type_id_ == asset_type::get_effect_id()) {
      DOODLE_CHICK_HTTP(l_asset_extend.gui_dang_, bad_request, "资产 {} 缺少扩展信息 归档", l_asset.name_);
      DOODLE_CHICK_HTTP(l_asset_extend.kai_shi_ji_shu_, bad_request, "资产 {} 缺少扩展信息 开始集数", l_asset.name_);
      // 拼音名称为空时跳过
      if (l_asset_extend.pin_yin_ming_cheng_.empty()) continue;

      l_arg.download_file_list_.emplace_back(
          l_prj.path_ / get_entity_prop_ue_path(l_prj, l_asset_extend) / get_entity_prop_ue_public_files_path(),
          l_scene_ue_path / get_entity_prop_ue_public_files_path()
      );
      l_arg.download_file_list_.emplace_back(
          l_prj.path_ / get_entity_prop_ue_path(l_prj, l_asset_extend) / get_entity_prop_ue_files_path(l_asset_extend),
          l_scene_ue_path / get_entity_prop_ue_files_path(l_asset_extend)
      );

    } else if (l_asset.entity_type_id_ == asset_type::get_ground_id()) {
    } else {
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request, "不支持的资产类型: {}",
              l_sql.get_by_uuid<asset_type>(l_asset.entity_type_id_).name_
          }
      );
    }
  }
  {  // 添加自动灯光生成的文件
    auto l_path_1 = FSys::path{doodle_config::ue4_content} / doodle_config::ue4_shot /
                    fmt::format("ep{:04}", l_episodes) /
                    fmt::format("{}{:03}_sc{:03}", l_prj.code_, l_episodes, l_shot) / "Import_DH";
    auto l_path_2 = FSys::path{doodle_config::ue4_content} / doodle_config::ue4_shot /
                    fmt::format("ep{:04}", l_episodes) /
                    fmt::format("{}{:03}_sc{:03}", l_prj.code_, l_episodes, l_shot) / "Import_JS";

    for (auto&& l_p : {l_path_1, l_path_2})
      l_arg.download_file_list_.emplace_back(
          l_prj.path_ / "03_Workflow" / "Shot" / fmt::format("EP{:04}", l_episodes) / l_uprj.stem() / l_p,
          l_scene_ue_path / l_p
      );
  }

  auto l_vfx_path = FSys::path{doodle_config::ue4_content} / doodle_config::ue4_shot /
                    fmt::format("ep{:04}", l_episodes) /
                    fmt::format("{}{:03}_sc{:03}", l_prj.code_, l_episodes, l_shot) / "Import_Vfx";
  auto l_vfx_path2 =
      FSys::path{doodle_config::ue4_content} / doodle_config::ue4_shot / fmt::format("ep{:04}", l_episodes) / "Vfx";
  // 灯光额外要下载特效的文件
  if (l_task.task_type_id_ == task_type::get_lighting_id()) {
    l_arg.download_file_list_.emplace_back(
        l_prj.path_ / get_shots_effect_ue_path(l_episode_entity) / l_uprj.stem() / l_vfx_path,
        l_scene_ue_path / l_vfx_path

    );
    l_arg.download_file_list_.emplace_back(
        l_prj.path_ / get_shots_effect_ue_path(l_episode_entity) / l_uprj.stem() / l_vfx_path2,
        l_scene_ue_path / l_vfx_path2

    );
  }

  // 开始填充上传列表
  if (l_task.task_type_id_ == task_type::get_shot_effect_id()) {
    l_arg.update_file_list_.emplace_back(
        l_scene_ue_path / l_vfx_path,
        l_prj.path_ / get_shots_effect_ue_path(l_episode_entity) / l_uprj.stem() / l_vfx_path

    );
    l_arg.update_file_list_.emplace_back(
        l_scene_ue_path / l_vfx_path2,
        l_prj.path_ / get_shots_effect_ue_path(l_episode_entity) / l_uprj.stem() / l_vfx_path2

    );
  } else if (l_task.task_type_id_ == task_type::get_lighting_id()) {
    auto l_light_path =
        FSys::path{doodle_config::ue4_content} / doodle_config::ue4_shot / fmt::format("ep{:04}", l_episodes) / "map";
    auto l_light_path2 = FSys::path{doodle_config::ue4_content} / doodle_config::ue4_shot /
                         fmt::format("ep{:04}", l_episodes) /
                         fmt::format("{}{:03}_sc{:03}", l_prj.code_, l_episodes, l_shot) / "Import_Lig";
    auto l_light_path3 =
        FSys::path{doodle_config::ue4_content} / doodle_config::ue4_shot / fmt::format("ep{:04}", l_episodes) /
        fmt::format("{}{:03}_sc{:03}", l_prj.code_, l_episodes, l_shot) /
        fmt::format("{}_EP{:03}_SC{:03}{}", l_prj.code_, l_episodes, l_shot, doodle_config::ue4_uasset_ext);
    auto l_light_path4 =
        FSys::path{doodle_config::ue4_content} / doodle_config::ue4_shot / fmt::format("ep{:04}", l_episodes) /
        fmt::format("{}{:03}_sc{:03}", l_prj.code_, l_episodes, l_shot) /
        fmt::format("{}_EP{:03}_SC{:03}_Zong{}", l_prj.code_, l_episodes, l_shot, doodle_config::ue4_umap_ext);

    l_arg.update_file_list_.emplace_back(
        l_scene_ue_path / l_light_path,
        l_prj.path_ / get_shots_lighting_ue_path(l_episode_entity) / l_uprj.stem() / l_light_path
    );
    l_arg.update_file_list_.emplace_back(
        l_scene_ue_path / l_light_path2,
        l_prj.path_ / get_shots_lighting_ue_path(l_episode_entity) / l_uprj.stem() / l_light_path2
    );
    l_arg.update_file_list_.emplace_back(
        l_scene_ue_path / l_light_path3,
        l_prj.path_ / get_shots_lighting_ue_path(l_episode_entity) / l_uprj.stem() / l_light_path3
    );
    l_arg.update_file_list_.emplace_back(
        l_scene_ue_path / l_light_path4,
        l_prj.path_ / get_shots_lighting_ue_path(l_episode_entity) / l_uprj.stem() / l_light_path4
    );
  } else {
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "仅支持灯光和特效任务同步信息获取"});
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_arg);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_tasks_assets_update_ue, get) {
  std::vector<FSys::path> l_paths;
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_task   = l_sql.get_by_uuid<task>(task_id_);
  auto l_entity = l_sql.get_by_uuid<entity>(l_task.entity_id_);
  if (l_entity.entity_type_id_ == asset_type::get_prop_id() ||
      l_entity.entity_type_id_ == asset_type::get_effect_id()) {
    auto l_asset_extend = l_sql.get_entity_asset_extend(l_entity.uuid_id_);
    DOODLE_CHICK_HTTP(l_asset_extend, bad_request, "实体 {} 缺少资产扩展信息", l_entity.name_);
    l_paths.emplace_back(get_entity_prop_ue_public_files_path());
    l_paths.emplace_back(get_entity_prop_ue_files_path(*l_asset_extend));
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_paths);
}

}  // namespace doodle::http