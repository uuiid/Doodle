//
// Created by TD on 2022/9/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <entt/entt.hpp>
namespace doodle::gui {
namespace details {

class DOODLE_CORE_API windows_tick_interface_impl : public entt::type_list<bool()> {
 public:
  template <typename Base>
  struct type : public Base {
    bool render() { return entt::poly_call<0>(*this); }
    const std::string& title() const { return entt::poly_call<1>(*this); }
  };

  template <typename Type>
  using impl = entt::value_list<&Type::render>;
};
using windows_tick_interface = entt::poly<windows_tick_interface_impl>;

}  // namespace details
using windows = details::windows_tick_interface;
}  // namespace doodle::gui
