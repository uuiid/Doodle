//
// Created by TD on 2022/9/8.
//
#pragma once
#include <map>
#include <string>
namespace doodle::dingding {

class dingding_config {
 private:
  dingding_config();
  struct app_key_appsecret {
    std::string name{};
    std::string app_key{};
    std::string app_value{};
  };

 public:
  const static constexpr std::string_view suoyi   = {"索以文化"};
  const static constexpr std::string_view congxin = {"从心动漫"};
  virtual ~dingding_config()                      = default;

  static dingding_config& get();

  dingding_config(const dingding_config&) noexcept            = delete;
  dingding_config(dingding_config&&) noexcept                 = delete;
  dingding_config& operator=(const dingding_config&) noexcept = delete;
  dingding_config& operator=(dingding_config&&) noexcept      = delete;
  std::map<std::string, app_key_appsecret> app_keys;

  void switch_key(const std::string& in_key);
  std::string key_name;
  std::string app_key;
  std::string app_value;
};

}  // namespace doodle::dingding
