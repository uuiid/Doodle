//
// Created by TD on 24-12-26.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
enum class person_role_type {
  user,
  admin,
  supervisor,
  manager,
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
// enum class contract_types {
//   open_ended,
//   fixed_term,
//   short_term,
//   freelance,
//   apprentice,
//   internship,
// };

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
  std::string contract_type_;

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





  // from json
  template <typename BasicJsonType>
  friend void from_json(const BasicJsonType& j, person& p) {
    if (j.contains("password")) j.at("password").get_to(p.password_);
    j.at("first_name").get_to(p.first_name_);
    j.at("last_name").get_to(p.last_name_);
    j.at("email").get_to(p.email_);
    j.at("phone").get_to(p.phone_);
    j.at("contract_type").get_to(p.contract_type_);
    j.at("active").get_to(p.active_);
    j.at("archived").get_to(p.archived_);
    j.at("last_presence").get_to(p.last_presence_);
    j.at("desktop_login").get_to(p.desktop_login_);
    j.at("login_failed_attemps").get_to(p.login_failed_attemps_);
    j.at("last_login_failed").get_to(p.last_login_failed_);
    j.at("totp_enabled").get_to(p.totp_enabled_);
    j.at("totp_secret").get_to(p.totp_secret_);
    j.at("email_otp_enabled").get_to(p.email_otp_enabled_);
    if (j.contains("email_otp_secret")) j.at("email_otp_secret").get_to(p.email_otp_secret_);
    j.at("fido_enabled").get_to(p.fido_enabled_);
    if (j.contains("fido_credentials")) j.at("fido_credentials").get_to(p.fido_credentials_);
    if (j.contains("otp_recovery_codes")) j.at("otp_recovery_codes").get_to(p.otp_recovery_codes_);
    j.at("preferred_two_factor_authentication").get_to(p.preferred_two_factor_authentication_);
    j.at("shotgun_id").get_to(p.shotgun_id_);
    j.at("timezone").get_to(p.timezone_);
    j.at("locale").get_to(p.locale_);
    j.at("data").get_to(p.data_);
    j.at("role").get_to(p.role_);
    j.at("has_avatar").get_to(p.has_avatar_);
    j.at("notifications_enabled").get_to(p.notifications_enabled_);
    j.at("notifications_slack_enabled").get_to(p.notifications_slack_enabled_);
    j.at("notifications_slack_userid").get_to(p.notifications_slack_userid_);
    j.at("notifications_mattermost_enabled").get_to(p.notifications_mattermost_enabled_);
    j.at("notifications_mattermost_userid").get_to(p.notifications_mattermost_userid_);
    j.at("notifications_discord_enabled").get_to(p.notifications_discord_enabled_);
    j.at("notifications_discord_userid").get_to(p.notifications_discord_userid_);
    j.at("is_bot").get_to(p.is_bot_);
    j.at("expiration_date").get_to(p.expiration_date_);
    j.at("studio_id").get_to(p.studio_id_);
    j.at("is_generated_from_ldap").get_to(p.is_generated_from_ldap_);
    j.at("ldap_uid").get_to(p.ldap_uid_);
  }
  // from json
  template <typename BasicJsonType>
  friend void to_json(BasicJsonType& j, const person& p) {
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
    j["totp_secret"]                         = p.totp_secret_;
    j["email_otp_enabled"]                   = p.email_otp_enabled_;
    j["email_otp_secret"]                    = p.email_otp_secret_;
    j["fido_enabled"]                        = p.fido_enabled_;
    j["fido_credentials"]                    = p.fido_credentials_;
    j["otp_recovery_codes"]                  = p.otp_recovery_codes_;
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
  }
};
}  // namespace doodle