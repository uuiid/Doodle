//
// Created by TD on 2022/9/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include "entt/core/type_traits.hpp"
#include <entt/entt.hpp>
#include <utility>
namespace doodle::gui {
namespace details {
namespace windows_impl {
template <typename... Type>
entt::type_list<Type...> as_type_list(const entt::type_list<Type...> &);
}
class DOODLE_CORE_API windows_tick_interface_impl : public entt::type_list<bool()> {
 public:
  template <typename base_type>
  struct type : public base_type {
    bool render() { return entt::poly_call<0>(*this); }
    //    const std::string& title() const { return entt::poly_call<1>(*this); }
  };

  template <typename type_t>
  using impl = entt::value_list<&type_t::render>;
};
using windows_tick_interface = entt::poly<windows_tick_interface_impl>;

class DOODLE_CORE_API windows_layout_interface_impl
    : public entt::type_list_cat_t<
          decltype(windows_impl::as_type_list(std::declval<windows_tick_interface_impl>())), entt::type_list<void()>> {
 public:
  template <typename base_type>
  struct type : windows_tick_interface_impl::template type<base_type> {
    static constexpr auto base = windows_tick_interface_impl::size;
    void set_show() { entt::poly_call<base + 0>(*this); }
  };

  template <typename type_t>
  using impl =
      entt::value_list_cat_t<typename windows_tick_interface_impl::impl<type_t>, entt::value_list<&type_t::set_show>>;
};
using windows_layout_interface = entt::poly<windows_layout_interface_impl>;
}  // namespace details
using windows        = details::windows_tick_interface;
using windows_layout = details::windows_layout_interface;
}  // namespace doodle::gui
