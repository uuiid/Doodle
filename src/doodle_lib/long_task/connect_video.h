//
// Created by TD on 2023/12/21.
//

#pragma once

#include "doodle_core/metadata/image_size.h"

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::detail {

boost::system::error_code connect_video(
    const FSys::path &in_out_path, logger_ptr in_msg, const std::vector<FSys::path> &in_vector,
    const image_size &in_size
);

struct connect_video_t {
  FSys::path out_path_;
  std::vector<FSys::path> file_list_{};
  image_size image_size_;

  logger_ptr msg_;

  friend void from_json(const nlohmann::json &nlohmann_json_j, connect_video_t &nlohmann_json_t) {
    nlohmann_json_j["out_path"].get_to(nlohmann_json_t.out_path_);
    nlohmann_json_j["file_list"].get_to(nlohmann_json_t.file_list_);
    nlohmann_json_j["image_size"].get_to(nlohmann_json_t.image_size_);
  }
};
}  // namespace doodle::detail
