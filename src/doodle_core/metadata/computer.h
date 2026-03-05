//
// Created by TD on 2024/2/26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
enum class computer_status { online, busy, free, offline, unknown };
NLOHMANN_JSON_SERIALIZE_ENUM(
    computer_status, {{computer_status::unknown, "unknown"},
                      {computer_status::online, "online"},
                      {computer_status::busy, "busy"},
                      {computer_status::free, "free"},
                      {computer_status::offline, "offline"}}
);
class computer {
 public:
  std::int32_t id_{};
  uuid uuid_id_{};

  std::string name_;
  std::string ip_;

  computer_status status_ = computer_status::online;
  chrono::system_zoned_time last_heartbeat_time_{chrono::current_zone(), chrono::system_clock::now()};

 private:
  // to json
  friend void to_json(nlohmann::json& j, const computer& p) {
    j["name"]   = p.name_;
    j["ip"]     = p.ip_;
    j["status"] = p.status_;
    j["id"]     = p.uuid_id_;
    j["last_heartbeat_time"] = p.last_heartbeat_time_;
  }
};

}  // namespace doodle