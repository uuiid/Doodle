//
// Created by TD on 2022/9/8.
//
#pragma once
#include <string>

namespace doodle::dingding {

class dingding_config {
 private:
  dingding_config();

 public:
  ~dingding_config() = default;

  static dingding_config& get();

  dingding_config(const dingding_config&) noexcept            = delete;
  dingding_config(dingding_config&&) noexcept                 = delete;
  dingding_config& operator=(const dingding_config&) noexcept = delete;
  dingding_config& operator=(dingding_config&&) noexcept      = delete;

  std::string app_key;
  std::string app_value;
};

}  // namespace doodle::dingding
