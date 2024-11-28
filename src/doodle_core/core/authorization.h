//
// Created by TD on 2022/3/23.
//
#pragma once

#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/configure/doodle_core_export.h>
namespace doodle {
class DOODLE_CORE_API authorization {
  class impl;
  std::unique_ptr<impl> p_i;

  void load_authorization_data(std::istream& in_apth);

  bool is_build_near();

 public:
  authorization();
  explicit authorization(const std::string& in_data);
  virtual ~authorization();

  [[nodiscard]] bool is_expire() const;
  void save(const FSys::path& in_path) const;
  void save() const;
  void load_authorization_data(const std::string& in_str);

  time_point_wrap::time_duration get_expire_time() const;

  static void generate_token(const FSys::path& in_path);
};
}  // namespace doodle
