//
// Created by td_main on 2023/10/19.
//

#pragma once
#include "boost/beast/http/message.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system.hpp>

#include <entt/entt.hpp>
#include <string>
#include <vector>
namespace doodle::http::project {

struct get_type {
  const std::vector<std::string> url_{"v1", "render_farm", "project"};
  void operator()(const entt::handle& in_handle) const;
};

struct post_type {
  const std::vector<std::string> url_{"v1", "render_farm", "project", "{name}"};
  void operator()(
      boost::system::error_code ec, const entt::handle& in_handle,
      const boost::beast::http::request<boost::beast::http::string_body>& in_request
  ) const;
};

// struct put_type {
//   const std::vector<std::string> url_{"v1", "render_farm", "project", "{name}"};
//   void operator()(boost::system::error_code ec, const entt::handle& in_handle) const;
// };

struct delete_type {
  const std::vector<std::string> url_{"v1", "render_farm", "project", "{name}"};
  void operator()(boost::system::error_code ec, const entt::handle& in_handle) const;
};

}  // namespace doodle::http::project
