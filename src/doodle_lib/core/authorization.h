//
// Created by TD on 2022/3/23.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API authorization {
  class impl;
  std::unique_ptr<impl> p_i;

  void load_authorization_data(const std::string& in_str);

 public:
  authorization();
  explicit authorization(const std::string& in_data);
  virtual ~authorization();

  [[nodiscard]] bool is_expire() const;
  void save(const FSys::path& in_path) const;
  void save() const;

  static void generate_token(const FSys::path& in_path);
};
}  // namespace doodle
