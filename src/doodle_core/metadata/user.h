//
// Created by TD on 2021/5/7.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
class user;

class DOODLE_CORE_API user : boost::equality_comparable<user> {
 private:
  std::string p_string_;
  std::string p_ENUS;
  class user_cache;

 public:
  user();

  explicit user(const std::string& in_string);

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& in_string);

  [[nodiscard]] const std::string& get_enus() const;

  bool operator==(const user& in_rhs) const;
  bool operator<(const user& in_rhs) const;

  /**
   * @brief 按名称寻找user
   * @param in_name 用户名称
   * @return 句柄(可能无效)
   */
  static entt::handle find_by_user_name(const std::string& in_name);
  /**
   * @brief 在打开数据库后, 注册表中保存的所有用户中寻找到当前用户,  如果未寻找到将创建一个新段用户
   */
  class DOODLE_CORE_API current_user {
   public:
    current_user();
    virtual ~current_user();
    entt::handle user_handle;
    boost::uuids::uuid uuid;

    explicit operator entt::handle();
    entt::handle get_handle();
    std::string user_name_attr();
    void user_name_attr(const std::string& in_name);

    void create_user();

    explicit operator bool() const;
  };

 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const user& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, user& p);
};

}  // namespace doodle
namespace fmt {
/**
 * @brief 集数格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::doodle::user> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::user& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<std::string>::format(in_.get_name(), ctx);
  }
};
}  // namespace fmt
