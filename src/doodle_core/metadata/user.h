//
// Created by TD on 2021/5/7.
//

#pragma once
#include "doodle_core/configure/doodle_core_export.h"
#include <doodle_core/doodle_core_fwd.h>

#include <cstdint>
#include <entt/entity/fwd.hpp>

namespace doodle {
class user;
/**
 * @brief 用户权限配置
 *
 */
enum class power_enum : std::uint32_t {
  none       = 0,
  user       = 1,
  supervisor = 2,
  manager    = 3,
  client     = 4,
  vendor     = 5,
  admin      = 6,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    power_enum,
    {
        {power_enum::none, "none"},
        {power_enum::admin, "admin"},
        {power_enum::manager, "manager"},
        {power_enum::supervisor, "supervisor"},
        {power_enum::client, "client"},
        {power_enum::vendor, "vendor"},
        {power_enum::user, "user"},
    }
)

class DOODLE_CORE_API user : boost::equality_comparable<user> {
 private:
  std::string p_string_;
  std::string p_ENUS;

  template <typename T1, typename Char, typename Enable>
  friend struct fmt::formatter;

 public:
  // 对应 kitsu 中的 user
  boost::uuids::uuid id_;
  // 手机号
  std::string mobile_;
  // 钉钉id
  std::string dingding_id_;

  user();

  power_enum power{power_enum::none};

  explicit user(const std::string& in_string);

  [[nodiscard]] const std::string& get_name() const;
  inline std::string& get_name() { return p_string_; };
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

 private:
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const user& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, user& p);
};

namespace user_helper {
struct database_t {
  std::int32_t id_{};
  uuid uuid_id_{};

  // 手机号
  std::string mobile_;
  // 权限
  power_enum power_{power_enum::none};
  // 钉钉id
  std::string dingding_id_;
  // 钉钉对应公司的 uuid
  uuid dingding_company_id_;

  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const database_t& p) {
    j["id"]          = fmt::to_string(p.uuid_id_);
    j["mobile"]      = p.mobile_;
    j["dingding_id"] = p.dingding_id_;
    j["company"]     = p.dingding_company_id_;
  }
};
}  // namespace user_helper

}  // namespace doodle
namespace fmt {
/**
 * @brief 集数格式化程序
 *
 */
template <>
struct formatter<::doodle::user> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::user& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<std::string>::format(in_.get_name(), ctx);
  }
};
}  // namespace fmt
