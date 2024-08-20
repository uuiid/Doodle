//
// Created by TD on 24-8-20.
//

#pragma once
#include <doodle_core/core/http_client_core.h>
namespace doodle::http {

struct kitsu_data_t {
  std::shared_ptr<detail::http_client_data_base> http_kitsu_{};
};

struct kitsu_ctx_t {
  std::string url_{};
  std::string access_token_{};
};

}  // namespace doodle::http