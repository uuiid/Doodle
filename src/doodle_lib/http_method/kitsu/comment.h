//
// Created by TD on 25-4-29.
//
#pragma once
#include <doodle_core/metadata/attachment_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>

namespace doodle::http {

struct create_comment_result : comment {
  task_status task_status_;
  person person_;
  std::vector<attachment_file> attachment_file_;
  explicit create_comment_result(
      const comment& in_comment, const task_status& in_task_status, const person& in_person,
      const std::vector<attachment_file>& in_attachment_file
  )
      : comment(in_comment),
        task_status_(in_task_status),
        person_(in_person),
        attachment_file_(in_attachment_file)

  {}

  // to json
  friend void to_json(nlohmann::json& j, const create_comment_result& p) {
    to_json(j, static_cast<const comment&>(p));
    j["task_status"]      = p.task_status_;
    j["person"]           = p.person_;
    j["attachment_files"] = p.attachment_file_;
  }
};
boost::asio::awaitable<create_comment_result> create_comment(
    std::shared_ptr<comment> in_comment, const http_jwt_fun::http_jwt_t* in_person, uuid in_task_id,
    std::vector<FSys::path> in_files, std::shared_ptr<task> in_task = nullptr
);
}  // namespace doodle::http