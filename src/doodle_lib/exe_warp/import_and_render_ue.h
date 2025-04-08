//
// Created by TD on 2024/1/8.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/image_size.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/shot.h>

#include <doodle_lib/core/scan_assets/base.h>
#include <doodle_lib/exe_warp/maya_exe.h>

namespace doodle {
namespace import_and_render_ue_ns {

struct import_files_t {
  std::string type_;
  FSys::path path_;
  FSys::path skin_path_;
  std::vector<std::string> hide_materials_;

  friend void to_json(nlohmann::json& j, const import_files_t& p) {
    j["type"]           = p.type_;
    j["path"]           = p.path_;
    j["skin_path"]      = p.skin_path_.generic_string();
    j["hide_materials"] = p.hide_materials_;
  }
};

struct import_data_t {
  project_helper::database_t project_;
  std::int32_t begin_time;
  std::int32_t end_time;
  episodes episode;
  shot shot;

  FSys::path out_file_dir;
  std::string import_dir;

  std::string original_map;  // 地编提供的主场景路径, 我们需要抓取子场景

  FSys::path render_map;             // 渲染关卡, 这个放置外面, 包含下面两个子关卡
  std::string create_map;            // 创建的关卡(放置骨骼网格体)
  FSys::path vfx_map;                // 特效关卡, 放特效
  FSys::path level_sequence_import;  // 渲染关卡序列(包的路径), 包括下面的子关卡
  FSys::path level_sequence_vfx;     // 额外的特效关卡序列(包的路径)

  FSys::path movie_pipeline_config;  // 渲染配置(包的路径)

  std::vector<import_files_t> files;

  image_size size_;  // 渲染的尺寸

  bool layering_;

  friend void to_json(nlohmann::json& j, const import_data_t& p) {
    j["project"]            = p.project_.code_;
    j["begin_time"]         = p.begin_time;
    j["end_time"]           = p.end_time;
    j["episode"]            = p.episode.p_episodes;
    j["shot"]               = p.shot.p_shot;
    j["shot_ab"]            = p.shot.get_shot_ab();
    j["out_file_dir"]       = p.out_file_dir.generic_string();
    j["original_map"]       = p.original_map;
    j["render_map"]         = p.render_map;
    j["files"]              = p.files;
    j["import_dir"]         = p.import_dir;
    j["create_map"]         = p.create_map;
    j["vfx_map"]            = p.vfx_map;
    j["level_sequence_vfx"] = p.level_sequence_vfx;

    auto l_path             = p.level_sequence_import;
    l_path.replace_extension();
    j["level_sequence"] = l_path;

    auto l_path2        = p.movie_pipeline_config;
    l_path2.replace_extension();
    j["movie_pipeline_config"] = l_path2;

    j["size"]                  = p.size_;
    j["layering"]              = p.layering_;
  }
};

struct import_skin_file {
  FSys::path fbx_file_{};
  FSys::path import_dir_{};
  friend void to_json(nlohmann::json& j, const import_skin_file& p) {
    j["fbx_file"]   = p.fbx_file_;
    j["import_dir"] = p.import_dir_;
  }
};

struct association_data {
  boost::uuids::uuid id_{};
  FSys::path maya_file_{};
  FSys::path ue_file_{};
  details::assets_type_enum type_{};
  FSys::path ue_prj_path_{};
  FSys::path export_file_{};
};
struct args {
  /// 需要填写
  episodes episodes_{};
  shot shot_{};
  project_helper::database_t project_{};
  std::shared_ptr<maya_exe_ns::arg> maya_arg_{};
  image_size size_{};
  bool layering_{};
  bool bind_skin_{};
  logger_ptr logger_ptr_{};
  bool is_sim_{};

  boost::asio::awaitable<void> run();

 private:
  boost::asio::awaitable<FSys::path> async_import_and_render_ue();
  import_data_t gen_import_config();

  boost::asio::awaitable<void> fetch_association_data();
  void down_files();
  void up_files(const FSys::path& in_out_image_path, const FSys::path& in_move_path) const;
  /// 检查文件的材质名称, 错误直接抛出异常
  void check_materials();
  boost::asio::awaitable<void> crate_skin();
  FSys::path create_move(const FSys::path& in_out_image_path) const;

  FSys::path render_project_{};  // 渲染工程文件(.project)
  // 场景文件
  FSys::path scene_file_{};
  // 导入文件和对应是 skin(skin 对应的是 /Game/路径)
  struct import_file {
    uuid id{};                          // 导出文件对应的id
    bool is_camera_{};                  // 是否是相机文件
    FSys::path file_{};                 // 导出的文件
    FSys::path skin_{};                 // 对应的骨骼文件
    details::assets_type_enum type_{};  // 类型
    FSys::path maya_file_{};            // 对应的maya文件
    FSys::path maya_solve_file_{};      // 对应的解算文件
    FSys::path maya_local_file_{};      // 对应的maya文件(从服务器中复制到本地的文件)

    FSys::path ue_file_{};      // 对应的ue文件
    FSys::path ue_prj_path_{};  // 对应的ue工程文件

    bool update_files{};  // UE 文件是否被更新

    /// 需要隐藏的材质名称
    std::vector<std::string> hide_materials_{};
  };
  std::vector<import_file> import_files_{};
  std::uint32_t begin_time_{};
  std::uint32_t end_time_{};

  // from json
  friend void from_json(const nlohmann::json& j, args& p) {
    j["episodes"].get_to(p.episodes_);
    j["shot"].get_to(p.shot_);
    j["project"].get_to(p.project_);
    j["image_size"].get_to(p.size_);
    j["layering"].get_to(p.layering_);
    if (j.contains("bind_skin")) j["bind_skin"].get_to(p.bind_skin_);
  }
};
void fix_project(const FSys::path& in_project_path);
void fix_config(const FSys::path& in_project_path);
}  // namespace import_and_render_ue_ns

// 清除 1001 以前的帧数
tl::expected<std::vector<FSys::path>, std::string> clean_1001_before_frame(
    const FSys::path& in_path, std::int32_t in_frame
);

}  // namespace doodle