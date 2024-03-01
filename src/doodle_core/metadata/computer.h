//
// Created by TD on 2024/2/26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
enum class computer_status { online, busy, free, unknown };
NLOHMANN_JSON_SERIALIZE_ENUM(
    computer_status, {{computer_status::online, "online"},
                      {computer_status::busy, "busy"},
                      {computer_status::free, "free"},
                      {computer_status::unknown, "unknown"}}
);
class computer {
 public:
  computer() = default;
  explicit computer(std::string in_name, std::string in_ip) : name_(std::move(in_name)), ip_(std::move(in_ip)) {}
  ~computer() = default;

  std::string name_;
  std::string ip_;

  computer_status server_status_ = computer_status::online;
  computer_status client_status_ = computer_status::online;

 private:
  // to json
  friend void to_json(nlohmann::json& j, const computer& p) {
    j["name"] = p.name_;
    j["ip"]   = p.ip_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, computer& p) {
    j.at("name").get_to(p.name_);
    j.at("ip").get_to(p.ip_);
  }
};

}  // namespace doodle