//
// Created by TD on 25-3-11.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API notification {
  DOODLE_BASE_FIELDS();
  bool read_{};
  bool change_{};
  std::string type_;
  uuid person_id_;
  uuid author_id_;
  uuid comment_id_;
  uuid task_id_;
  uuid reply_id_;
};
}
