//
// Created by TD on 2022/9/14.
//
#pragma once
#include <doodle_dingding/doodle_dingding_fwd.h>
#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>

#include <doodle_dingding/metadata/access_token.h>
#include <doodle_dingding/metadata/department.h>

namespace doodle::dingding {

namespace detail {
class request_base {
 public:
  std::int32_t errcode;
  std::string errmsg;

  explicit operator std::error_code() {
    return std::error_code{};
  }
};
}  // namespace detail

template <bool is_result, typename Result_Type>
class request_base;

template <typename Result_Type>
class request_base<true, Result_Type> {
 public:
  std::int32_t errcode;
  std::string errmsg;
  Result_Type result_type;

 public:
  explicit request_base(const nlohmann::json& in_json)
      : errcode(in_json.at("errcode").get<std::int32_t>()),
        errmsg(in_json.at("errmsg").get<std::string>()),
        result_type(in_json.at("result").get<Result_Type>()) {}
};
template <typename Result_Type>
class request_base<false, Result_Type> {
 public:
  std::int32_t errcode;
  std::string errmsg;
  Result_Type result_type;

 public:
  explicit request_base(const nlohmann::json& in_json)
      : errcode(in_json.at("errcode").get<std::int32_t>()),
        errmsg(in_json.at("errmsg").get<std::string>()),
        result_type(in_json.get<Result_Type>()) {}
};
using access_token_body = request_base<false, access_token>;
using department_body   = request_base<true, std::vector<department>>;

}  // namespace doodle::dingding
