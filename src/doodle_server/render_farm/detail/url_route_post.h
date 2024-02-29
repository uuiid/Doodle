//
// Created by td_main on 2023/8/9.
//

#pragma once
#include <doodle_core/render_farm/basic_json_body.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>

#include "doodle_server/core/http_session.h"
#include "doodle_server/core/url_route_base.h"
#include <nlohmann/json.hpp>
#include <utility>
namespace doodle::render_farm {
namespace detail {

struct render_job_type_post {
  std::vector<std::string> url_{"v1", "render_farm", "render_job"};
  boost::beast::http::verb verb_{boost::beast::http::verb::post};
  void operator()(
      boost::system::error_code ec, const entt::handle &in_handle,
      boost::beast::http::request<boost::beast::http::string_body> in_request
  ) const;
};

struct get_log_type_post {
  std::vector<std::string> url_{"v1", "render_farm", "log", "{handle}"};
  boost::beast::http::verb verb_{boost::beast::http::verb::get};
  void operator()(
      boost::system::error_code ec, const entt::handle &in_handle,
      boost::beast::http::request<boost::beast::http::string_body> in_request
  ) const;
};
struct get_err_type_post {
  std::vector<std::string> url_{"v1", "render_farm", "err", "{handle}"};
  boost::beast::http::verb verb_{boost::beast::http::verb::get};
  void operator()(
      boost::system::error_code ec, const entt::handle &in_handle,
      boost::beast::http::request<boost::beast::http::string_body> in_request
  ) const;
};
}  // namespace detail
}  // namespace doodle::render_farm