//
// Created by TD on 2022/9/14.
//
#pragma once
#include <doodle_dingding/doodle_dingding_fwd.h>
#include <nlohmann/json_fwd.hpp>
#include <nlohmann/json.hpp>
#include <utility>

#include <doodle_dingding/metadata/access_token.h>
#include <doodle_dingding/metadata/department.h>
#include <doodle_dingding/metadata/user_dd.h>

namespace doodle::dingding {

namespace detail {
class DOODLE_DINGDING_API request_base {
 public:
  std::int32_t errcode;
  std::string errmsg;
  nlohmann::json json_attr{};
  explicit request_base(std::int32_t in_code, std::string in_msg, nlohmann::json in_json)
      : errcode(in_code),
        errmsg(std::move(in_msg)),
        json_attr(std::move(in_json)){};
  virtual ~request_base() = default;
  [[nodiscard("")]] doodle_error get_error() const {
    return doodle_error{"code: {} {}", errcode, errmsg};
  }
  [[nodiscard("s")]] explicit operator bool() const {
    return errcode != 0;
  }
};
}  // namespace detail

template <bool is_result, typename Result_Type>
class request_base;

template <typename Result_Type>
class request_base<true, Result_Type> : public detail::request_base {
 public:
  Result_Type result_type() {
    return json_attr.at("result").get<Result_Type>();
  };

 public:
  explicit request_base(const nlohmann::json& in_json)
      : detail::request_base(
            in_json.at("errcode").get<std::int32_t>(),
            in_json.at("errmsg").get<std::string>(),
            in_json
        ) {}
};
template <typename Result_Type>
class request_base<false, Result_Type> : public detail::request_base {
 public:
  Result_Type result_type() {
    return json_attr.get<Result_Type>();
  };

 public:
  explicit request_base(const nlohmann::json& in_json)
      : detail::request_base(
            in_json.at("errcode").get<std::int32_t>(),
            in_json.at("errmsg").get<std::string>(),
            in_json
        ) {}
};
namespace detail {
template <typename Result_Type>
class cursor {
 public:
  bool has_more;
  std::size_t next_cursor;
  Result_Type list;
  friend void to_json(nlohmann::json& nlohmann_json_j, const cursor& nlohmann_json_t) {
    nlohmann_json_j["has_more"]    = nlohmann_json_t.has_more;
    nlohmann_json_j["next_cursor"] = nlohmann_json_t.next_cursor;
    nlohmann_json_j["list"]        = nlohmann_json_t.list;
  }
  friend void from_json(const nlohmann::json& nlohmann_json_j, cursor& nlohmann_json_t) {
    nlohmann_json_j.at("has_more").get_to(nlohmann_json_t.has_more);
    if (nlohmann_json_j.contains("next_cursor"))
      nlohmann_json_j.at("next_cursor").get_to(nlohmann_json_t.next_cursor);
    if (nlohmann_json_j.contains("list"))
      nlohmann_json_j.at("list").get_to(nlohmann_json_t.list);
  }
};
}  // namespace detail

using access_token_body = request_base<false, access_token>;
using department_body   = request_base<true, std::vector<department>>;
using user_dd_body      = request_base<true, detail::cursor<std::vector<user_dd>>>;

}  // namespace doodle::dingding
