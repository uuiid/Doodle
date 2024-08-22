//
// Created by TD on 24-8-20.
//

#pragma once
#include <doodle_core/core/http_client_core.h>

#include <doodle_lib/core/http/http_session_data.h>

namespace doodle::http {
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;

struct kitsu_data_t {
  std::shared_ptr<detail::http_client_data_base> http_kitsu_;
  std::map<boost::uuids::uuid, std::string> task_types_;
};

struct kitsu_ctx_t {
  std::string url_;
  std::string access_token_;
  std::map<boost::uuids::uuid, std::string> task_types_;
};

http_route_ptr create_kitsu_route();

namespace kitsu {
http::detail::http_client_data_base_ptr create_kitsu_proxy(session_data_ptr in_handle);
}

}  // namespace doodle::http