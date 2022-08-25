//
// Created by TD on 2022/8/25.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/core/app_base.h>
#include <boost/asio.hpp>

namespace doodle {

template <typename T>
auto post_tick(T&& in_fun) {
  app_base::Get()._add_tick_(
      [fun = std::move(in_fun)](bool& in_r) {
        in_r = fun();
      }
  );
}

}  // namespace doodle
