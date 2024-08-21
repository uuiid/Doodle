//
// Created by TD on 24-8-20.
//

#pragma once
#include <doodle_core/core/http_client_core.h>
namespace doodle::http {
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;

struct kitsu_data_t {
  std::shared_ptr<detail::http_client_data_base> http_kitsu_{};
  std::map<boost::uuids::uuid, std::string> task_types_{};
};

struct kitsu_ctx_t {
  std::string url_{};
  std::string access_token_{};
};


http_route_ptr create_kitsu_route();

}  // namespace doodle::http