//
// Created by TD on 24-8-6.
//

#pragma once
#include <doodle_core/core/core_sql.h>

#include <doodle_lib/core/http/http_route.h>

#include "entt/entity/fwd.hpp"
namespace doodle::http {

struct model_base_t {
  entt::registry registry_;
  database_info database_info_;
  std::any any_;
  model_base_t()                                 = default;
  ~model_base_t()                                = default;
  model_base_t(const model_base_t&)            = delete;
  model_base_t(model_base_t&&)                 = delete;
  model_base_t& operator=(const model_base_t&) = delete;
  model_base_t& operator=(model_base_t&&)      = delete;
};

void model_base_reg(http_route& in_route);
}  // namespace doodle::http