//
// Created by TD on 2022/3/23.
//
#pragma once

#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/metadata/time_point_wrap.h>
namespace doodle {
class DOODLE_CORE_API authorization {
  class impl;
  std::unique_ptr<impl> p_i;

  bool is_build_near();

 public:
  explicit authorization(const std::string& in_data);

  /// 检查授权是否过期
  [[nodiscard]] bool is_expire() const;
  void load_authorization_data(const std::string& in_str);

  time_point_wrap::time_duration get_expire_time() const;

  static void generate_token(const FSys::path& in_path);
};
}  // namespace doodle
