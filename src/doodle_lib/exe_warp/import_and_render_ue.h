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
enum class output_quality_type { low, medium, high };
NLOHMANN_JSON_SERIALIZE_ENUM(
    output_quality_type,
    {{output_quality_type::low, "low"}, {output_quality_type::medium, "medium"}, {output_quality_type::high, "high"}}
)


struct import_files_t {
  std::string type_;
  FSys::path path_;

  friend void to_json(nlohmann::json& j, const import_files_t& p) {
    j["type"] = p.type_;
    j["path"] = p.path_.generic_string();
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

  image_size size_; // 渲染的尺寸

  output_quality_type output_quality_;

  friend void to_json(nlohmann::json& j, const import_data_t& p) {
    j["project"]            = p.project_.shor_str_;
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

    j["size"]               = p.size_;
    j["output_quality"]     = p.output_quality_;
  }
};

struct down_info {
 public:
  FSys::path render_project_{};  // 渲染工程文件(.project)
  // 场景文件
  FSys::path scene_file_{};
};



struct args {
  /// 需要填写
  episodes episodes_{};
  shot shot_{};
  project_helper::database_t project_{};
  std::shared_ptr<maya_exe_ns::arg> maya_arg_{};
  image_size size_{};
  output_quality_type output_quality_{};

  // 不需要填写
  maya_exe_ns::maya_out_arg maya_out_arg_{};
  down_info down_info_{};
  // from json
  friend void from_json(const nlohmann::json& j, args& p) {
    j["episodes"].get_to(p.episodes_);
    j["shot"].get_to(p.shot_);
    j["project"].get_to(p.project_);
    j["image_size"].get_to(p.size_);
    j["output_quality"].get_to(p.output_quality_);
  }
};
void fix_project(const FSys::path& in_project_path);
void fix_config(const FSys::path& in_project_path);
}  // namespace import_and_render_ue_ns

struct association_data {
  boost::uuids::uuid id_{};
  FSys::path maya_file_{};
  FSys::path ue_file_{};
  details::assets_type_enum type_{};
  FSys::path ue_prj_path_{};
  FSys::path export_file_{};
};

// 清除 1001 以前的帧数
std::tuple<boost::system::error_code, std::vector<FSys::path>> clean_1001_before_frame(
    const FSys::path& in_path, std::int32_t in_frame
);
/// 这个函数不打印错误日志, 返回值测试后, 由调用函数打印错误日志
boost::asio::awaitable<std::tuple<boost::system::error_code, std::vector<association_data>>> fetch_association_data(
    std::vector<boost::uuids::uuid> in_uuid, logger_ptr in_logger
);
boost::asio::awaitable<std::tuple<boost::system::error_code, FSys::path>> async_import_and_render_ue(
    std::shared_ptr<import_and_render_ue_ns::args> in_args, logger_ptr in_logger
);

boost::asio::awaitable<std::tuple<boost::system::error_code, FSys::path>> async_auto_loght(
  std::shared_ptr<import_and_render_ue_ns::args> in_args, logger_ptr in_logger
);
} // namespace doodle