//
// Created by TD on 2022/10/24.
//
#pragma once

#include <doodle_dingding/doodle_dingding_fwd.h>
#include <string>
namespace doodle {
namespace dingding {

class DOODLE_DINGDING_API user {
 public:
  std::string phone_number{};
  std::string user_id{};

 public:
  user()  = default;
  ~user() = default;
};

}  // namespace dingding
}  // namespace doodle
