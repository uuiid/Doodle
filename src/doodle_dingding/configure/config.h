//
// Created by TD on 2022/9/8.
//
#pragma once
#include <string>

namespace doodle::dingding {

class dingding_config {
 private:
  dingding_config();
  struct app_key_appsecret {
    std::string name;
    std::string app_key;
    std::string app_value;
  };

 public:
  virtual ~dingding_config() = default;

  static dingding_config& get();

  dingding_config(const dingding_config&) noexcept            = delete;
  dingding_config(dingding_config&&) noexcept                 = delete;
  dingding_config& operator=(const dingding_config&) noexcept = delete;
  dingding_config& operator=(dingding_config&&) noexcept      = delete;
  std::map<std::string, app_key_appsecret> app_keys;

  std::string app_key;
  std::string app_value;
};

}  // namespace doodle::dingding
