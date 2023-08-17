//
// Created by td_main on 2023/8/17.
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
struct render_job_type_put {
  std::vector<std::string> url_{"v1", "render_farm", "render_job", "{handle}"};
  boost::beast::http::verb verb_{boost::beast::http::verb::put};
  void operator()(const entt::handle &in_handle, const std::map<std::string, std::string> &in_cap) const;
};
}  // namespace detail
}  // namespace doodle::render_farm