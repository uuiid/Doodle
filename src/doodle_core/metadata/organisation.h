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


};
}  // namespace doodle