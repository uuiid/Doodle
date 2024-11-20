//
// Created by TD on 2024/2/26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
enum class computer_status { online, busy, free, unknown };
NLOHMANN_JSON_SERIALIZE_ENUM(
    computer_status, {{computer_status::unknown, "unknown"},
                      {computer_status::online, "online"},
                      {computer_status::busy, "busy"},
                      {computer_status::free, "free"}}
);
class computer {
 public:
  std::int32_t id_{};
  uuid uuid_id_{};

  std::string name_;
  std::string ip_;

  computer_status server_status_ = computer_status::online;
  computer_status client_status_ = computer_status::online;

 private:
  // to json
  friend void to_json(nlohmann::json& j, const computer& p) {
    j["name"]   = p.name_;
    j["ip"]     = p.ip_;
    j["status"] = p.server_status_;
    j["id"]     = p.uuid_id_;
  }
};

}  // namespace doodle