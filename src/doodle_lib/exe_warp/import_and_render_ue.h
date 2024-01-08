//
// Created by TD on 2024/1/8.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/shot.h>

namespace doodle {

class import_and_render_ue {
 public:
  import_and_render_ue();
  ~import_and_render_ue() = default;

  struct import_files_t {
    std::string type_;
    FSys::path path_;
    friend void to_json(nlohmann::json &j, const import_files_t &p) {
      j["type"] = p.type_;
      j["path"] = p.path_.generic_string();
    }
  };
  struct import_data_t {
    project project_;
    std::int32_t begin_time;
    std::int32_t end_time;
    episodes episode;
    shot shot;

    FSys::path out_file_dir;
    std::string original_map;
    std::string import_dir;
    std::string create_map;

    std::string render_map;             // 渲染关卡
    std::string level_sequence;         // 渲染关卡序列
    std::string movie_pipeline_config;  // 渲染配置

    std::vector<import_files_t> files;
    friend void to_json(nlohmann::json &j, const import_data_t &p) {
      j["project"]               = p.project_.p_shor_str;
      j["begin_time"]            = p.begin_time;
      j["end_time"]              = p.end_time;
      j["episode"]               = p.episode.p_episodes;
      j["shot"]                  = p.shot.p_shot;
      j["shot_ab"]               = p.shot.p_shot_ab;
      j["out_file_dir"]          = p.out_file_dir.generic_string();
      j["original_map"]          = p.original_map;
      j["render_map"]            = p.render_map;
      j["files"]                 = p.files;
      j["import_dir"]            = p.import_dir;
      j["level_sequence"]        = p.level_sequence;
      j["create_map"]            = p.create_map;
      j["movie_pipeline_config"] = p.movie_pipeline_config;
    }
  };

  struct args {
    FSys::path ue_project_path_{};

    import_data_t import_data_{};
  };
};
}  // namespace doodle
