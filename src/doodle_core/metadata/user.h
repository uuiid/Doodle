//
// Created by TD on 2021/5/7.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
class user;
void to_json(nlohmann::json& j, const user& p);
void from_json(const nlohmann::json& j, user& p);
class user : boost::equality_comparable<user> {
 private:
  std::string p_string_;
  std::string p_ENUS;

 public:
  user();
  DOODLE_MOVE(user);
  explicit user(const std::string& in_string);

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& in_string);

  [[nodiscard]] const std::string& get_enus() const;

  bool operator==(const user& in_rhs) const;
  bool operator<(const user& in_rhs) const;

  static void set_user_ctx(entt::registry& in_reg);
  static void has_user_in_ctx(entt::registry& in_reg);
  static void set_user_entt(entt::registry& in_reg);

 private:
  friend void to_json(nlohmann::json& j, const user& p);
  friend void from_json(const nlohmann::json& j, user& p);
};

}  // namespace doodle
