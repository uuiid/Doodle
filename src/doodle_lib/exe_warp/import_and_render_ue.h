//
// Created by TD on 2024/1/8.
//

#pragma once
#include "doodle_core/metadata/server_task_info.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/image_size.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_lib/core/asyn_task.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/signals2/signal.hpp>

#include <bitset>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <vector>

namespace doodle {
namespace import_and_render_ue_ns {
struct import_skin_file {
  FSys::path fbx_file_{};
  FSys::path import_dir_{};
  friend void to_json(nlohmann::json& j, const import_skin_file& p) {
    j["fbx_file"]   = p.fbx_file_;
    j["import_dir"] = p.import_dir_;
  }
};
void fix_project(const FSys::path& in_project_path);
void fix_config(const FSys::path& in_project_path);

}  // namespace import_and_render_ue_ns

// 清除 1001 以前的帧数
std::vector<FSys::path> clean_1001_before_frame(const FSys::path& in_path, std::int32_t in_frame);

class run_ue_assembly_local : public async_task {
 public:
  boost::asio::awaitable<void> run() override;

  struct run_ue_assembly_asset_info {
    FSys::path shot_output_path_;     // 需要组装的fbx
    FSys::path skin_path_;            // 组装对应的fbx
    std::string type_;                // 是fbx, 还是 abc
    std::bitset<2> simulation_type_;  // 0: 带布料 1: 带毛发
    FSys::path ue_project_dir_;       // ue项目文件夹路径

    // to json
    friend void to_json(nlohmann::json& j, const run_ue_assembly_asset_info& p) {
      j["path"]      = p.shot_output_path_;
      j["skin_path"] = p.skin_path_;
      j["type"]      = p.type_;
    }
    // from json
    friend void from_json(const nlohmann::json& j, run_ue_assembly_asset_info& p) {
      j.at("path").get_to(p.shot_output_path_);
      j.at("skin_path").get_to(p.skin_path_);
      j.at("type").get_to(p.type_);
    }
  };
  struct file_copy_info {
    FSys::path from_;
    FSys::path to_;
    // to json
    friend void to_json(nlohmann::json& j, const file_copy_info& p) {
      j["from"] = p.from_;
      j["to"]   = p.to_;
    }
    // from json
    friend void from_json(const nlohmann::json& j, file_copy_info& p) {
      j.at("from").get_to(p.from_);
      j.at("to").get_to(p.to_);
    }
  };

  struct run_ue_assembly_arg {
    std::vector<run_ue_assembly_asset_info> asset_infos_;
    FSys::path camera_file_path_;                 // 相机路径
    FSys::path ue_main_project_path_;             // 主项目路径
    std::vector<file_copy_info> ue_asset_path_;   // 需要复制的UE路径
    std::vector<file_copy_info> update_ue_path_;  // 上传的UE路径

    std::int32_t begin_time_;
    std::int32_t end_time_;
    FSys::path out_file_dir_;
    FSys::path original_map_;           // 主场景路径
    FSys::path render_map_;             // 渲染关卡, 这个放置外面, 包含下面两个子关卡
    FSys::path create_map_;             // 创建的关卡(放置骨骼网格体)
    FSys::path import_dir_;             // 导入的fbx和abc路径
    FSys::path movie_pipeline_config_;  // 电影管线配置
    FSys::path level_sequence_import_;  // 渲染关卡序列(包的路径), 包括下面的子关卡
    image_size size_;                   // 渲染的尺寸
    bool layering_;
    FSys::path create_move_path_;  // 合成视屏的路径
    shot shot_;
    episodes episodes_;
    // to josn
    friend void to_json(nlohmann::json& j, const run_ue_assembly_arg& p) {
      j["files"]                 = p.asset_infos_;
      j["camera_file_path"]      = p.camera_file_path_;
      j["ue_main_project_path"]  = p.ue_main_project_path_;
      j["ue_asset_path"]         = p.ue_asset_path_;

      j["begin_time"]            = p.begin_time_;
      j["end_time"]              = p.end_time_;
      j["out_file_dir"]          = p.out_file_dir_;
      j["original_map"]          = p.original_map_;
      j["render_map"]            = p.render_map_;
      j["create_map"]            = p.create_map_;
      j["import_dir"]            = p.import_dir_;
      j["movie_pipeline_config"] = p.movie_pipeline_config_;
      j["level_sequence"]        = p.level_sequence_import_;
      j["size"]                  = p.size_;
      j["layering"]              = p.layering_;
      j["create_move_path"]      = p.create_move_path_;
      j["shot"]                  = p.shot_;
      j["episodes"]              = p.episodes_;
      j["update_ue_path"]        = p.update_ue_path_;
    }
    // from json
    friend void from_json(const nlohmann::json& j, run_ue_assembly_arg& p) {
      j.at("files").get_to(p.asset_infos_);
      j.at("camera_file_path").get_to(p.camera_file_path_);
      j.at("ue_main_project_path").get_to(p.ue_main_project_path_);
      j.at("ue_asset_path").get_to(p.ue_asset_path_);

      j.at("begin_time").get_to(p.begin_time_);
      j.at("end_time").get_to(p.end_time_);
      j.at("out_file_dir").get_to(p.out_file_dir_);
      j.at("original_map").get_to(p.original_map_);
      j.at("render_map").get_to(p.render_map_);
      j.at("create_map").get_to(p.create_map_);
      j.at("import_dir").get_to(p.import_dir_);
      j.at("movie_pipeline_config").get_to(p.movie_pipeline_config_);
      j.at("level_sequence").get_to(p.level_sequence_import_);
      j.at("size").get_to(p.size_);
      j.at("layering").get_to(p.layering_);
      j.at("create_move_path").get_to(p.create_move_path_);
      j.at("shot").get_to(p.shot_);
      j.at("episodes").get_to(p.episodes_);
      j.at("update_ue_path").get_to(p.update_ue_path_);
    }
  };
  uuid shot_task_id_{};
  uuid project_id_{};
  std::shared_ptr<kitsu::kitsu_client> kitsu_client_{};

  boost::signals2::signal<void(const server_task_info::run_time_info_t&)> on_run_time_info_;

 private:
  run_ue_assembly_arg arg_;

  // to json
  friend void to_json(nlohmann::json& j, const run_ue_assembly_local& p) { j = p.arg_; }
  // from json
  friend void from_json(const nlohmann::json& j, run_ue_assembly_local& p) { j.get_to(p.arg_); }
};

}  // namespace doodle