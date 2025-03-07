//
// Created by TD on 25-3-7.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
struct DOODLE_CORE_API organisation {
  std::string name_;
  std::int32_t hours_by_day_;
  bool has_avatar_;
  bool use_original_file_name_;
  bool timesheets_locked_;
  bool format_duration_in_hours_;
  bool hd_by_default_;
  std::string chat_token_slack_;
  std::string chat_webhook_mattermost_;
  std::string chat_token_discord_;
  std::string dark_theme_by_default_;

  // from json
  template <typename BasicJsonType>
  friend void from_json(const BasicJsonType& j, organisation& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("hours_by_day")) j.at("hours_by_day").get_to(p.hours_by_day_);
    if (j.contains("has_avatar")) j.at("has_avatar").get_to(p.has_avatar_);
    if (j.contains("use_original_file_name")) j.at("use_original_file_name").get_to(p.use_original_file_name_);
    if (j.contains("timesheets_locked")) j.at("timesheets_locked").get_to(p.timesheets_locked_);
    if (j.contains("format_duration_in_hours")) j.at("format_duration_in_hours").get_to(p.format_duration_in_hours_);
    if (j.contains("hd_by_default")) j.at("hd_by_default").get_to(p.hd_by_default_);
    if (j.contains("chat_token_slack")) j.at("chat_token_slack").get_to(p.chat_token_slack_);
    if (j.contains("chat_webhook_mattermost")) j.at("chat_webhook_mattermost").get_to(p.chat_webhook_mattermost_);
    if (j.contains("chat_token_discord")) j.at("chat_token_discord").get_to(p.chat_token_discord_);
    if (j.contains("dark_theme_by_default")) j.at("dark_theme_by_default").get_to(p.dark_theme_by_default_);
  }
  // to json
  template <typename BasicJsonType>
  friend void to_json(BasicJsonType& j, const organisation& p) {
    j["name"]                     = p.name_;
    j["hours_by_day"]             = p.hours_by_day_;
    j["has_avatar"]               = p.has_avatar_;
    j["use_original_file_name"]   = p.use_original_file_name_;
    j["timesheets_locked"]        = p.timesheets_locked_;
    j["format_duration_in_hours"] = p.format_duration_in_hours_;
    j["hd_by_default"]            = p.hd_by_default_;
    j["chat_token_slack"]         = p.chat_token_slack_;
    j["chat_webhook_mattermost"]  = p.chat_webhook_mattermost_;
    j["chat_token_discord"]       = p.chat_token_discord_;
    j["dark_theme_by_default"]    = p.dark_theme_by_default_;
  }
};

}  // namespace doodle