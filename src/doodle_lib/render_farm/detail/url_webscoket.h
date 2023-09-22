//
// Created by td_main on 2023/9/14.
//

#pragma once
#include <doodle_lib/render_farm/http_session.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::render_farm {
namespace detail {


struct reg_work_websocket {
  reg_work_websocket() = default;

  void operator()();
};

struct reg_server_websocket {
  reg_server_websocket() = default;

  void operator()();
};

}  // namespace detail
}  // namespace doodle::render_farm