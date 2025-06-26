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
