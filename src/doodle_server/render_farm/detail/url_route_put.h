//
// Created by td_main on 2023/8/17.
//

#pragma once
#include <doodle_core/render_farm/basic_json_body.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>

#include "core/http_session.h"
#include <doodle_server/render_farm/detail/url_route_base.h>
#include <nlohmann/json.hpp>
#include <utility>
namespace doodle::render_farm {
namespace detail {
struct render_job_type_put {
  std::vector<std::string> url_{"v1", "render_farm", "render_job", "{handle}"};
  boost::beast::http::verb verb_{boost::beast::http::verb::put};
  void operator()(
      boost::system::error_code ec, const entt::handle &in_handle,
      boost::beast::http::request<boost::beast::http::string_body> in_request
  ) const;
};
}  // namespace detail
}  // namespace doodle::render_farm