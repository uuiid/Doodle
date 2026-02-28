#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
struct maya_out_arg {
  std::uint32_t begin_time{};
  std::uint32_t end_time{};
  std::vector<FSys::path> out_file_list{};
  FSys::path movie_file_dir{};

  friend void from_json(const nlohmann::json& nlohmann_json_j, maya_out_arg& nlohmann_json_t);

  friend void to_json(nlohmann::json& nlohmann_json_j, const maya_out_arg& nlohmann_json_t);
};
}  // namespace doodle