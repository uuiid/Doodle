//
// Created by td_main on 2023/8/10.
//

#pragma once
#include <nlohmann/json.hpp>

namespace doodle::render_farm {

class computer {
 public:
  computer()  = default;
  ~computer() = default;

  // to_json
  friend void to_json(nlohmann::json& j, const computer& p) { j["name"] = p.name_; }
  // from_json
  friend void from_json(const nlohmann::json& j, computer& p) { j.at("name").get_to(p.name_); }

  [[nodiscard]] std::string name() const { return name_; }
  void set_name(std::string in_name) { name_ = std::move(in_name); }

 private:
  std::string name_;
};

}  // namespace doodle::render_farm
