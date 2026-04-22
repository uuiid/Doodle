#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {

enum class DOODLE_CORE_API task_status {
  standby,
  running,
  complete,
  canceled,
};

struct DOODLE_CORE_API task {
  DOODLE_BASE_FIELDS();
  uuid user_id_;
  task_status status_;
  std::string data_request_;
  std::string file_extension_;
  std::string data_response_;
  uuid studio_id_;
  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};

  // to json
  friend void to_json(nlohmann::json& j, const task& p) {
    j["id"]             = p.uuid_id_;
    j["user_id"]        = p.user_id_;
    j["status"]         = p.status_;
    j["data_request"]   = p.data_request_;
    j["file_extension"] = p.file_extension_;
    j["data_response"]  = p.data_response_;
    j["studio_id"]      = p.studio_id_;

    j["created_at"]     = p.created_at_;
  }
};

}  // namespace doodle::seedance2