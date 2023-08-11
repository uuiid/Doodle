//
// Created by td_main on 2023/8/10.
//

#pragma once
#include <boost/asio.hpp>

#include <nlohmann/json.hpp>
namespace doodle::render_farm {

enum class computer_status : std::int32_t {
  idle,   // 空转
  busy,   // 忙碌
  lost,   // 失联
  error,  // 错误
};

class computer {
 public:
  computer()  = default;
  ~computer() = default;

  [[nodiscard]] inline std::string name() const { return name_; }
  inline void set_name(std::string in_name) { name_ = std::move(in_name); }

  [[nodiscard]] inline computer_status status() const { return status_; }
  inline void set_status(computer_status in_status) { status_ = in_status; }

  // 延期
  void delay();

  // to_json
  friend void to_json(nlohmann::json& j, const computer& p) { j["name"] = p.name_; }
  // from_json
  friend void from_json(const nlohmann::json& j, computer& p) { j.at("name").get_to(p.name_); }

 private:
  std::string name_;
  computer_status status_{computer_status::lost};
  std::shared_ptr<boost::asio::system_timer> timer_;
};

}  // namespace doodle::render_farm
