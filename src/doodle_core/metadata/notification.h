//
// Created by TD on 25-3-11.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {

enum class notification_type {
  comment,
  mention,
  assignation,
  reply,
  reply_mention,
};

struct DOODLE_CORE_API notification {
  DOODLE_BASE_FIELDS();
  bool read_{};
  bool change_{};  // 指向的task 状态是否发生了改变, 比如从完成到返修
  notification_type type_;
  uuid person_id_;
  uuid author_id_;
  uuid comment_id_;
  uuid task_id_;
  uuid reply_id_;
};
}  // namespace doodle
