//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
struct attachment_file {
  DOODLE_BASE_FIELDS();

  std::string name_;
  std::int64_t size_;
  std::string extension_;
  std::string mimetype_;
  uuid comment_id_;
  uuid chat_message_id_;
  // to_json
  friend void to_json(nlohmann::json& j, const attachment_file& p) {
    j["id"]        = p.uuid_id_;
    j["name"]      = p.name_;
    j["size"]      = p.size_;
    j["extension"] = p.extension_;
  }
};
}  // namespace doodle