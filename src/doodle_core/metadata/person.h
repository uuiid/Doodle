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
  chrono::system_zoned_time last_presence_;

  std::string password_;
  std::string desktop_login_;
  std::int32_t login_failed_attemps_;
  chrono::system_zoned_time last_login_failed_;
  bool totp_enabled_;
  std::string totp_secret_;
  bool email_otp_enabled_;
  std::string email_otp_secret_;
  bool fido_enabled_;
  std::string fido_credentials_;
  std::string otp_recovery_codes_;
  two_factor_authentication_types preferred_two_factor_authentication_;

  std::int32_t shotgun_id_;
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
  chrono::system_zoned_time expiration_date_;

  std::vector<uuid> departments_;
  uuid studio_id_;

  bool is_generated_from_ldap_;
  std::string ldap_uid_;
};
}  // namespace doodle