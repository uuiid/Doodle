//
// Created by TD on 25-3-11.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {

enum class notification_type {
  comment,  // 评论
  mention,  // 提到我
  assignation,
  reply,
  reply_mention,
};

struct DOODLE_CORE_API notification {
  DOODLE_BASE_FIELDS();
  bool read_{};
  bool change_{};  // 指向的task 状态是否发生了改变, 比如从完成到返修
  notification_type type_;
  uuid person_id_;  // 接收通知的人
  uuid author_id_;  // 评论的作者
  uuid comment_id_;
  uuid task_id_;
  uuid reply_id_;
  chrono::system_zoned_time created_at_{chrono::system_clock::now()};
  // to json
  friend void to_json(nlohmann::json& j, const notification& p) {
    j["id"]         = p.uuid_id_;
    j["read"]       = p.read_;
    j["change"]     = p.change_;
    j["type"]       = p.type_;
    j["person_id"]  = p.person_id_;
    j["author_id"]  = p.author_id_;
    j["comment_id"] = p.comment_id_;
    j["task_id"]    = p.task_id_;
    j["reply_id"]   = p.reply_id_;
    j["created_at"] = p.created_at_;
  }
};
}  // namespace doodle
