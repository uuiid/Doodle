//
// Created by TD on 2021/5/7.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
class user {
 private:
  std::string p_string_;
  std::string p_ENUS;

 public:
  user();
  DOODLE_MOVE(user);
  explicit user(std::string in_string);
  explicit user(std::string in_string, std::string in_ENUS);

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& in_string);

  [[nodiscard]] const std::string& get_enus() const;


 private:

  friend void to_json(nlohmann::json& j, const user& p) {
    j["string_"]   = p.p_string_;
    j["ENUS"]      = p.p_ENUS;
  }
  friend void from_json(const nlohmann::json& j, user& p) {
    j.at("string_").get_to(p.p_string_);
    j.at("ENUS").get_to(p.p_ENUS);
  }
};


}  // namespace doodle
