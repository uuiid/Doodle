//
// Created by TD on 2021/12/27.
//
#pragma once

#include "doodle_core/metadata/image_size.h"
#include <doodle_core/metadata/move_create.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
namespace detail {
FSys::path create_out_path(
    const FSys::path& in_dir, const episodes& in_eps, const shot& in_shot,
    const std::string& in_project_short_string = {}
);
void create_move(
    const FSys::path& in_out_path, logger_ptr in_msg, const std::vector<movie::image_attr>& in_vector,
    const image_size& in_image_size
);

struct image_to_move {
  FSys::path out_path_;
  FSys::path path_{};
  image_size image_size_;
  shot shot_;
  episodes eps_;
  std::string user_name_;
  friend void from_json(const nlohmann::json& nlohmann_json_j, image_to_move& nlohmann_json_t) {
    nlohmann_json_j["path"].get_to(nlohmann_json_t.path_);
    nlohmann_json_j["out_path"].get_to(nlohmann_json_t.out_path_);
    nlohmann_json_j["image_size"].get_to(nlohmann_json_t.image_size_);
    nlohmann_json_j["shot"].get_to(nlohmann_json_t.shot_);
    nlohmann_json_j["episodes"].get_to(nlohmann_json_t.eps_);
    nlohmann_json_j["user_name"].get_to(nlohmann_json_t.user_name_);
  }

 private:
  std::vector<movie::image_attr> vector_;
};

}  // namespace detail
}  // namespace doodle