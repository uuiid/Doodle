//
// Created by TD on 24-12-30.
//

#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/entity_type.h"
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/entity_path.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <filesystem>
#include <map>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <vector>

namespace doodle::http {
namespace {

struct run_ue_assembly_asset_info {
  FSys::path shot_output_path_;
  FSys::path ue_asset_path_;
  FSys::path ue_sk_path_;
  // to json
  friend void to_json(nlohmann::json& j, const run_ue_assembly_asset_info& p) {
    j["shot_output_path"] = p.shot_output_path_;
    j["ue_asset_path"]    = p.ue_asset_path_;
    j["ue_sk_path"]       = p.ue_sk_path_;
  }
};

struct run_ue_assembly_ret {
  std::vector<run_ue_assembly_asset_info> asset_infos_;
  FSys::path camera_file_path_;
  FSys::path ue_main_project_path_;
  // to josn
  friend void to_json(nlohmann::json& j, const run_ue_assembly_ret& p) {
    j["asset_infos"]          = p.asset_infos_;
    j["camera_file_path"]     = p.camera_file_path_;
    j["ue_main_project_path"] = p.ue_main_project_path_;
  }
};

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
  using namespace sqlite_orm;
  constexpr auto shot     = "shot"_alias.for_<entity>();
  constexpr auto sequence = "sequence"_alias.for_<entity>();

  run_ue_assembly_ret l_ret{};
  FSys::path l_shot_path_dir{};
  /// tag: 格式化路径
  if (l_shot_task.task_type_id_ == task_type::get_animation_id()) {
    l_shot_path_dir = get_shots_animation_output_path(l_episode_entity.name_, l_shot_entity.name_, l_prj.code_);
  } else if (l_shot_task.task_type_id_ == task_type::get_simulation_task_id()) {
    l_shot_path_dir = get_shots_simulation_output_path(l_episode_entity.name_, l_shot_entity.name_, l_prj.code_);
  } else
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "任务类型不支持该操作"});

  l_shot_path_dir       = l_prj.path_ / l_shot_path_dir;
  auto l_shot_file_name = get_shots_animation_file_name(l_episode_entity.name_, l_shot_entity.name_, l_prj.code_);

  for (auto&& l_path : FSys::directory_iterator{l_shot_path_dir}) {
    auto l_stem = l_path.path().stem().string();
    if (l_stem.starts_with(l_shot_file_name) &&
        (l_path.path().extension() == ".abc" || l_path.path().extension() == ".fbx")) {
      if (l_stem.find("_camera_") != std::string::npos)
        l_ret.camera_file_path_ = l_path.path();
      else
        l_ret.asset_infos_.emplace_back(run_ue_assembly_asset_info{.shot_output_path_ = l_path.path()});
    }
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
    auto l_key    = l_stem.substr(l_shot_file_name.size());
    // find _rig and _Low
    auto l_size   = l_key.size();
    if (auto l_rig_post = l_key.find("_rig"); l_rig_post != std::string::npos) {
      l_key = l_key.substr(0, l_size - l_rig_post);
    } else if (auto l_low_post = l_key.find("_Low"); l_low_post != std::string::npos) {
      l_key = l_key.substr(0, l_size - l_low_post + 4);
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
  for (auto&& [l_asset, l_asset_extend] : l_assets) {
    if (l_asset.entity_type_id_ == asset_type::get_character_id()) {
      if (l_asset_infos_key_map.contains(l_asset_extend.bian_hao_)) {
        if (l_asset_extend.gui_dang_ && l_asset_extend.kai_shi_ji_shu_) {
          for (auto&& l_idx : l_asset_infos_key_map[l_asset_extend.bian_hao_]) {
            l_ret.asset_infos_[l_idx].ue_asset_path_ = get_entity_character_ue_path(
                l_prj.asset_root_path_, l_asset_extend.gui_dang_.value(), l_asset_extend.kai_shi_ji_shu_.value(),
                l_asset_extend.bian_hao_, l_asset_extend.pin_yin_ming_cheng_
            );
            l_ret.asset_infos_[l_idx].ue_sk_path_ =
                get_entity_character_ue_name(l_asset_extend.bian_hao_, l_asset_extend.pin_yin_ming_cheng_);
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
            l_ret.asset_infos_[l_idx].ue_asset_path_ = get_entity_prop_ue_path(
                l_prj.asset_root_path_, l_asset_extend.gui_dang_.value(), l_asset_extend.kai_shi_ji_shu_.value()
            );
            l_ret.asset_infos_[l_idx].ue_sk_path_ = get_entity_prop_ue_name(
                l_asset_extend.bian_hao_, l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_
            );
            l_ret.asset_infos_[l_idx].ue_asset_path_ /=
                l_ret.asset_infos_[l_idx].ue_sk_path_.parent_path().parent_path();
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
      if (l_asset_extend.gui_dang_ && l_asset_extend.kai_shi_ji_shu_) {
        l_ret.ue_main_project_path_ =
            get_entity_ground_ue_path(
                l_prj.asset_root_path_, l_asset_extend.gui_dang_.value_or(0),
                l_asset_extend.kai_shi_ji_shu_.value_or(0), l_asset_extend.bian_hao_, l_asset_extend.pin_yin_ming_cheng_
            ) /
            get_entity_ground_ue_map_name(l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_);
      } else
        throw_exception(
            http_request_error{
                boost::beast::http::status::bad_request,
                fmt::format("资产 {} 缺少归档或开始集信息，无法生成 ue 主工程路径", l_asset.name_)
            }
        );
      auto l_key = fmt::format(
          "{}{}{}_Low", l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_.empty() ? "" : "_",
          l_asset_extend.ban_ben_
      );
      if (l_asset_infos_key_map.contains(l_key)) {
        for (auto&& l_idx : l_asset_infos_key_map[l_key]) {
          l_ret.asset_infos_[l_idx].ue_sk_path_ =
              get_entity_ground_ue_sk_name(l_asset_extend.pin_yin_ming_cheng_, l_asset_extend.ban_ben_);
        }
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
    if (l_info.ue_asset_path_.empty() && l_info.ue_sk_path_.empty()) {
      throw_exception(
          http_request_error{
              boost::beast::http::status::bad_request,
              fmt::format("无法为输出文件 {} 生成对应的 ue 资产路径", l_info.shot_output_path_.string())
          }
      );
    }
    l_info.ue_sk_path_ = conv_ue_game_path(l_info.ue_sk_path_);
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_ret);
}
}  // namespace doodle::http