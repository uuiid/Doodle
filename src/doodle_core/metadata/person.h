//
// Created by TD on 24-12-26.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <magic_enum.hpp>
namespace doodle {
enum class person_role_type {
  user,        // 艺术家
  admin,       // 超级管理员
  supervisor,  // 组长
  manager,     // 项目经理
  client,
  vendor,
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    person_role_type, {{person_role_type::user, "user"},
                       {person_role_type::admin, "admin"},
                       {person_role_type::supervisor, "supervisor"},
                       {person_role_type::manager, "manager"},
                       {person_role_type::client, "client"},
                       {person_role_type::vendor, "vendor"}}
)
inline auto format_as(const person_role_type& in_role) { return magic_enum::enum_name(in_role); }
enum class contract_types {
  open_ended,
  fixed_term,
  short_term,
  freelance,
  apprentice,
  internship,
};

enum class two_factor_authentication_types {
  totp,
  email_otp,
  fido,
};

struct person_department_link {
  std::int64_t id_;
  uuid person_id_;
  uuid department_id_;
};

struct DOODLE_CORE_API person {
  DOODLE_BASE_FIELDS();

  std::string first_name_;
  std::string last_name_;
  std::string email_;
  std::string phone_;
  contract_types contract_type_;

  bool active_;
  bool archived_;
  std::optional<chrono::system_zoned_time> last_presence_;

  std::string password_;
  std::string desktop_login_;
  std::int32_t login_failed_attemps_;
  std::optional<chrono::system_zoned_time> last_login_failed_;
  bool totp_enabled_;
  std::string totp_secret_;
  bool email_otp_enabled_;
  std::string email_otp_secret_;
  bool fido_enabled_;
  std::string fido_credentials_;
  std::string otp_recovery_codes_;
  std::optional<two_factor_authentication_types> preferred_two_factor_authentication_;

  std::optional<std::int32_t> shotgun_id_;
  std::string timezone_;
  std::string locale_;
  std::string data_;
  person_role_type role_;
  bool has_avatar_;

  bool notifications_enabled_;
  bool notifications_slack_enabled_;
  std::string notifications_slack_userid_;
  bool notifications_mattermost_enabled_;
  std::string notifications_mattermost_userid_;
  bool notifications_discord_enabled_;
  std::string notifications_discord_userid_;

  bool is_bot_;
  std::string jti_;
  std::optional<chrono::system_zoned_time> expiration_date_;

  std::vector<uuid> departments_;
  uuid studio_id_;

  bool is_generated_from_ldap_;
  std::string ldap_uid_;

  // 自定义属性
  // 钉钉id
  std::string dingding_id_;
  // 钉钉对应公司的 uuid
  uuid dingding_company_id_;

  std::string get_full_name() const {
    return !first_name_.empty() && !last_name_.empty() ? first_name_ + ' ' + last_name_ : first_name_ + last_name_;
  };

  /// 控制 departments_ 是否写入json 中
  // bool write_departments_{true};

  // from json
  friend void from_json(const nlohmann::json& j, person& p) {
    if (j.contains("password")) j.at("password").get_to(p.password_);
    if (j.contains("first_name")) j.at("first_name").get_to(p.first_name_);
    if (j.contains("last_name")) j.at("last_name").get_to(p.last_name_);
    if (j.contains("email")) j.at("email").get_to(p.email_);
    if (j.contains("phone")) j.at("phone").get_to(p.phone_);
    if (j.contains("contract_type")) j.at("contract_type").get_to(p.contract_type_);
    if (j.contains("active")) j.at("active").get_to(p.active_);
    if (j.contains("archived")) j.at("archived").get_to(p.archived_);
    if (j.contains("last_presence")) j.at("last_presence").get_to(p.last_presence_);
    if (j.contains("desktop_login")) j.at("desktop_login").get_to(p.desktop_login_);
    if (j.contains("login_failed_attemps")) j.at("login_failed_attemps").get_to(p.login_failed_attemps_);
    if (j.contains("last_login_failed")) j.at("last_login_failed").get_to(p.last_login_failed_);
    if (j.contains("totp_enabled")) j.at("totp_enabled").get_to(p.totp_enabled_);
    if (j.contains("totp_secret")) j.at("totp_secret").get_to(p.totp_secret_);
    if (j.contains("email_otp_enabled")) j.at("email_otp_enabled").get_to(p.email_otp_enabled_);
    if (j.contains("email_otp_secret")) j.at("email_otp_secret").get_to(p.email_otp_secret_);
    if (j.contains("fido_enabled")) j.at("fido_enabled").get_to(p.fido_enabled_);
    if (j.contains("fido_credentials")) j.at("fido_credentials").get_to(p.fido_credentials_);
    if (j.contains("otp_recovery_codes")) j.at("otp_recovery_codes").get_to(p.otp_recovery_codes_);
    if (j.contains("preferred_two_factor_authentication"))
      j.at("preferred_two_factor_authentication").get_to(p.preferred_two_factor_authentication_);
    if (j.contains("shotgun_id")) j.at("shotgun_id").get_to(p.shotgun_id_);
    if (j.contains("timezone")) j.at("timezone").get_to(p.timezone_);
    if (j.contains("locale")) j.at("locale").get_to(p.locale_);
    if (j.contains("data")) j.at("data").get_to(p.data_);
    if (j.contains("role")) j.at("role").get_to(p.role_);
    if (j.contains("has_avatar")) j.at("has_avatar").get_to(p.has_avatar_);
    if (j.contains("notifications_enabled")) j.at("notifications_enabled").get_to(p.notifications_enabled_);
    if (j.contains("notifications_slack_enabled"))
      j.at("notifications_slack_enabled").get_to(p.notifications_slack_enabled_);
    if (j.contains("notifications_slack_userid"))
      j.at("notifications_slack_userid").get_to(p.notifications_slack_userid_);
    if (j.contains("notifications_mattermost_enabled"))
      j.at("notifications_mattermost_enabled").get_to(p.notifications_mattermost_enabled_);
    if (j.contains("notifications_mattermost_userid"))
      j.at("notifications_mattermost_userid").get_to(p.notifications_mattermost_userid_);
    if (j.contains("notifications_discord_enabled"))
      j.at("notifications_discord_enabled").get_to(p.notifications_discord_enabled_);
    if (j.contains("notifications_discord_userid"))
      j.at("notifications_discord_userid").get_to(p.notifications_discord_userid_);
    if (j.contains("is_bot")) j.at("is_bot").get_to(p.is_bot_);
    if (j.contains("expiration_date")) j.at("expiration_date").get_to(p.expiration_date_);
    if (j.contains("studio_id")) j.at("studio_id").get_to(p.studio_id_);
    if (j.contains("is_generated_from_ldap")) j.at("is_generated_from_ldap").get_to(p.is_generated_from_ldap_);
    if (j.contains("ldap_uid")) j.at("ldap_uid").get_to(p.ldap_uid_);

    if (j.contains("dingding_company_id")) j.at("dingding_company_id").get_to(p.dingding_company_id_);
  }
  // from json
  friend void to_json(nlohmann::json& j, const person& p) {
    j["first_name"]                          = p.first_name_;
    j["last_name"]                           = p.last_name_;
    j["email"]                               = p.email_;
    j["phone"]                               = p.phone_;
    j["contract_type"]                       = p.contract_type_;
    j["active"]                              = p.active_;
    j["archived"]                            = p.archived_;
    j["last_presence"]                       = p.last_presence_;
    j["desktop_login"]                       = p.desktop_login_;
    j["login_failed_attemps"]                = p.login_failed_attemps_;
    j["last_login_failed"]                   = p.last_login_failed_;
    j["totp_enabled"]                        = p.totp_enabled_;
    j["email_otp_enabled"]                   = p.email_otp_enabled_;
    j["fido_enabled"]                        = p.fido_enabled_;
    j["preferred_two_factor_authentication"] = p.preferred_two_factor_authentication_;
    j["shotgun_id"]                          = p.shotgun_id_;
    j["timezone"]                            = p.timezone_;
    j["locale"]                              = p.locale_;
    j["data"]                                = p.data_;
    j["role"]                                = p.role_;
    j["has_avatar"]                          = p.has_avatar_;
    j["notifications_enabled"]               = p.notifications_enabled_;
    j["notifications_slack_enabled"]         = p.notifications_slack_enabled_;
    j["notifications_slack_userid"]          = p.notifications_slack_userid_;
    j["notifications_mattermost_enabled"]    = p.notifications_mattermost_enabled_;
    j["notifications_mattermost_userid"]     = p.notifications_mattermost_userid_;
    j["notifications_discord_enabled"]       = p.notifications_discord_enabled_;
    j["notifications_discord_userid"]        = p.notifications_discord_userid_;
    j["is_bot"]                              = p.is_bot_;
    j["expiration_date"]                     = p.expiration_date_;
    j["studio_id"]                           = p.studio_id_;
    j["is_generated_from_ldap"]              = p.is_generated_from_ldap_;
    j["ldap_uid"]                            = p.ldap_uid_;
    j["departments"]                         = p.departments_;
    j["dingding_company_id"]                 = p.dingding_company_id_;
    j["fido_devices"]                        = nlohmann::json::array();
    j["full_name"]                           = p.first_name_ + " " + p.last_name_;
    j["id"]                                  = p.uuid_id_;
  }
};
}  // namespace doodle