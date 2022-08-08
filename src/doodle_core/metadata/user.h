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

  explicit user(const std::string& in_string);

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& in_string);

  [[nodiscard]] const std::string& get_enus() const;

  bool operator==(const user& in_rhs) const;
  bool operator<(const user& in_rhs) const;

  /**
   * @brief 在打开数据库后, 注册表中保存的所有用户中寻找到当前用户,  如果未寻找到将创建一个新段用户
   * @param in_reg 传入段注册表引用
   */
  static void reg_to_ctx(entt::registry& in_reg);
  static entt::handle get_user();

 private:
  friend void to_json(nlohmann::json& j, const user& p);
  friend void from_json(const nlohmann::json& j, user& p);
};

}  // namespace doodle
