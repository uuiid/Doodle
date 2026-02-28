//
// Created by TD on 2022/3/23.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_lib/configure/doodle_lib_export.h>

namespace doodle {
class DOODLELIB_API authorization {
  class impl;
  std::shared_ptr<impl> p_i;

  bool is_build_near();

 public:
  explicit authorization(const std::string& in_data);

  /// 检查授权 是否有效 true 有效 false 无效
  [[nodiscard]] bool is_valid() const;
  void load_authorization_data(const std::string& in_str);

  chrono::sys_time_pos::duration get_expire_time() const;

  static void generate_token(const FSys::path& in_path);
};
}  // namespace doodle
