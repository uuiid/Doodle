//
// Created by td_main on 2023/9/14.
//

#pragma once
#include <doodle_lib/render_farm/working_machine_session.h>

#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::render_farm {
namespace detail {

struct computer_reg_type_websocket {
  std::vector<std::string> url_{"v1", "render_farm", "computer"};
  void operator()(const entt::handle &in_handle, const std::map<std::string, std::string> &in_cap) const;
};

struct reg_websocket {
  reg_websocket() = default;

  void operator()();
};

}  // namespace detail
}  // namespace doodle::render_farm