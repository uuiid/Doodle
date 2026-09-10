//
// Created by TD on 2024/2/26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/server_task_info_type.h>

#include <set>

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
  // 电脑硬件id, 唯一识别码, 由计算机生成, 用于区分不同的计算机
  uuid hardware_id_{};

  std::string name_;
  std::string ip_;

  computer_status status_ = computer_status::online;
  chrono::system_zoned_time last_heartbeat_time_{chrono::current_zone(), chrono::system_clock::now()};
  uuid bot_uuid_;
  // 允许执行的任务类型（会话级，由工作机连接时上报，不持久化）
  std::set<server_task_info_type> allowed_task_types_{};

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"name", &computer::name_},      //
        std::pair{"status", &computer::status_},  //
    };
  }

 private:
  // to json
  friend void to_json(nlohmann::json& j, const computer& p) {
    j["name"]                = p.name_;
    j["ip"]                  = p.ip_;
    j["status"]              = p.status_;
    j["id"]                  = p.uuid_id_;
    j["hardware_id"]         = p.hardware_id_;
    j["last_heartbeat_time"] = p.last_heartbeat_time_;
    j["allowed_task_types"]  = p.allowed_task_types_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, computer& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("status")) j.at("status").get_to(p.status_);
    j.at("hardware_id").get_to(p.hardware_id_);
    if (j.contains("last_heartbeat_time")) j.at("last_heartbeat_time").get_to(p.last_heartbeat_time_);
    if (j.contains("allowed_task_types")) j.at("allowed_task_types").get_to(p.allowed_task_types_);
  }
};

}  // namespace doodle