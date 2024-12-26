//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {

struct DOODLE_CORE_API task_status {
  DOODLE_BASE_FIELDS();

  std::string name_;
  bool archived_;
  std::string short_name_;
  std::string description_;
  std::string color_;
  std::int32_t priority_;
  bool is_done_;
  bool is_artist_allowed_;
  bool is_client_allowed_;
  bool is_retake_;
  bool is_feedback_request_;
  bool is_default_;
  std::int32_t shotgun_id_;
  bool for_concept_;
};
}  // namespace doodle