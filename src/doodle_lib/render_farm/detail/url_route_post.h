//
// Created by td_main on 2023/8/9.
//

#pragma once
#include <doodle_lib/render_farm/detail/basic_json_body.h>
#include <doodle_lib/render_farm/detail/url_route_base.h>
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>

#include <nlohmann/json.hpp>
#include <utility>
namespace doodle::render_farm {
namespace detail {

struct render_job_type_post {
  std::vector<std::string> url_{"v1", "render_farm", "render_job"};
  boost::beast::http::verb verb_{boost::beast::http::verb::post};
  void operator()(const entt::handle &in_handle, const std::map<std::string, std::string> &in_cap) const;
};

struct computer_reg_type_post {
  std::vector<std::string> url_{"v1", "render_farm", "computer"};
  boost::beast::http::verb verb_{boost::beast::http::verb::post};
  void operator()(const entt::handle &in_handle, const std::map<std::string, std::string> &in_cap) const;
};

struct get_log_type_post {
  std::vector<std::string> url_{"v1", "render_farm", "log", "{handle}"};
  boost::beast::http::verb verb_{boost::beast::http::verb::get};
  void operator()(const entt::handle &in_handle, const std::map<std::string, std::string> &in_cap) const;
};
struct get_err_type_post {
  std::vector<std::string> url_{"v1", "render_farm", "err", "{handle}"};
  boost::beast::http::verb verb_{boost::beast::http::verb::get};
  void operator()(const entt::handle &in_handle, const std::map<std::string, std::string> &in_cap) const;
};
}  // namespace detail
}  // namespace doodle::render_farm
