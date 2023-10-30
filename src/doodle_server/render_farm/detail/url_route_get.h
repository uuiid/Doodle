//
// Created by td_main on 2023/8/9.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/render_farm/basic_json_body.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>

#include "core/http_session.h"
#include "core/url_route_base.h"

namespace doodle::render_farm::detail {

struct get_root_type {
  std::vector<std::string> url_{"v1", "render_farm"};
  void operator()(
      const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
  ) const;
};

struct get_log_type_get {
  std::vector<std::string> url_{"v1", "render_farm", "log", "{handle}"};
  void operator()(
      const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
  ) const;
};
struct get_err_type_get {
  std::vector<std::string> url_{"v1", "render_farm", "err", "{handle}"};
  void operator()(
      const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
  ) const;
};
struct render_job_type_get {
  std::vector<std::string> url_{"v1", "render_farm", "render_job"};
  void operator()(
      const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
  ) const;
};
struct computer_reg_type_get {
  std::vector<std::string> url_{"v1", "render_farm", "computer"};
  void operator()(
      const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
  ) const;
};
struct repository_type_get {
  std::vector<std::string> url_{"v1", "render_farm", "repository"};
  void operator()(
      const entt::handle& in_handle, const boost::beast::http::request<boost::beast::http::empty_body>& in_req
  ) const;
};
}  // namespace doodle::render_farm::detail
