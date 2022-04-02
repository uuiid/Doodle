//
// Created by TD on 2022/3/24.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {

class export_file_info {
  friend void to_json(nlohmann::json& j, const export_file_info& p);
  friend void from_json(const nlohmann::json& j, export_file_info& p);

 public:
  enum class export_type : std::uint32_t {
    abc    = 1,
    fbx    = 2,
    camera = 3
  };
  export_file_info();
  explicit export_file_info(FSys::path in_path,
                            std::int32_t in_start_frame,
                            std::int32_t in_end_frame,
                            FSys::path in_ref_path);
  FSys::path file_path;
  std::int32_t start_frame;
  std::int32_t end_frame;
  FSys::path ref_file;

  static void write_file(const entt::handle& in_handle);
};
}  // namespace doodle
