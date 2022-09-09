//
// Created by TD on 2022/3/24.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

class DOODLE_CORE_API export_file_info {
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const export_file_info& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, export_file_info& p);

 public:
  enum class export_type : std::uint32_t {
    none   = 0,
    abc    = 1,
    fbx    = 2,
    camera = 3
  };

 private:
  friend void to_json(nlohmann::json& j, const export_type& p);
  friend void from_json(const nlohmann::json& j, export_type& p);

 public:
  export_file_info();
  explicit export_file_info(
      FSys::path in_path,
      std::int32_t in_start_frame,
      std::int32_t in_end_frame,
      FSys::path in_ref_path,
      export_type in_export_type
  );
  FSys::path file_path;
  std::int32_t start_frame;
  std::int32_t end_frame;
  FSys::path ref_file;
  export_type export_type_;

  /**
   * @brief 这个数据时上传需要的路径前缀,
   * 会和项目设置中的路径进行结合,
   * 同时每种不同的插件都对这个路径有者不同的解释
   */
  FSys::path upload_path_;

  static void write_file(const entt::handle& in_handle);
  static entt::handle read_file(const FSys::path& in_path);
};
}  // namespace doodle
