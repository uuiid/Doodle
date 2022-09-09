//
// Created by TD on 2022/9/9.
//
#pragma once

#include <doodle_dingding/client/client.h>
#include <nlohmann/json_fwd.hpp>

namespace doodle {
namespace dingding {

class access_token {
 public:
  std::string token;
  std::int32_t expires_in;
  //  friend void DOODLE_DINGDING_API to_json(nlohmann::json& j, const access_token& in){
  //
  //  };
  friend void DOODLE_DINGDING_API from_json(const nlohmann::json& j, access_token& p);
};
using read_access_token_fun = std::function<void(const access_token& in)>;

}  // namespace dingding
}  // namespace doodle
