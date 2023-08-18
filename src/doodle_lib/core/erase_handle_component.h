//
// Created by td_main on 2023/8/17.
//

#pragma once
#include <boost/asio.hpp>

#include <entt/entt.hpp>
#include <type_traits>

namespace doodle {
namespace detail {
template <class Handler, typename Component>
class erase_reg_component {
  //  using Handler_Class = boost::callable_traits::class_of_t<Handler>;
  Handler h_;

  template <class T, class Executor>
  friend struct boost::asio::associated_executor;

  template <class T, class Allocator>
  friend struct boost::asio::associated_allocator;

  template <class T, class CancellationSlot>
  friend struct boost::asio::associated_cancellation_slot;

 public:
  using result_type                               = void;  // asio needs this

  erase_reg_component(erase_reg_component&&)      = default;
  erase_reg_component(erase_reg_component const&) = default;

  template <class Handler_>
  explicit erase_reg_component(Handler_&& handler) : h_(std::forward<Handler_>(handler)) {}

  template <>
  void operator()() {
    h_.erase<Component>();
  }
  //  template <class Function>
  //  friend boost::asio::asio_handler_invoke_is_deprecated asio_handler_invoke(Function&& f, erase_reg_component* op) {
  //    using boost::asio::asio_handler_invoke;
  //    return asio_handler_invoke(f, std::addressof(op->h_));
  //  }
  //
  //  friend bool asio_handler_is_continuation(erase_reg_component* op) {
  //    using boost::asio::asio_handler_is_continuation;
  //    return asio_handler_is_continuation(std::addressof(op->h_));
  //  }
  //
  //  friend boost::asio::asio_handler_allocate_is_deprecated asio_handler_allocate(
  //      std::size_t size, erase_reg_component* op
  //  ) {
  //    using boost::asio::asio_handler_allocate;
  //    return asio_handler_allocate(size, std::addressof(op->h_));
  //  }
  //
  //  friend boost::asio::asio_handler_deallocate_is_deprecated asio_handler_deallocate(
  //      void* p, std::size_t size, erase_reg_component* op
  //  ) {
  //    using boost::asio::asio_handler_deallocate;
  //    return asio_handler_deallocate(p, size, std::addressof(op->h_));
  //  }
};

}  // namespace detail

// template <typename Handler, typename Component>
// auto erase_reg_component(Handler&& handler, Component* in_instance, const registry_ptr& in_reg_ptr = g_reg()) {
//   return detail::erase_reg_component<
//       decltype(entt::handle{*in_reg_ptr, entt::to_entity(*in_reg_ptr, *in_instance)}), std::decay_t<Component> >(
//       std::forward<Handler>(handler), entt::handle{*in_reg_ptr, entt::to_entity(*in_reg_ptr, *in_instance)}
//   );
// }
template <typename Component, typename IO_Context = boost::asio::io_context>
auto erase_reg_component(
    Component* in_instance, IO_Context& in_io_context = g_io_context(), const registry_ptr& in_reg_ptr = g_reg()
) {
  using Component_Type = std::decay_t<Component>;

  boost::asio::post(in_io_context, [l_h = entt::handle{*in_reg_ptr, entt::to_entity(*in_reg_ptr, *in_instance)}]() {
    l_h.erase<Component_Type>();
  });
}
template <typename Component, typename IO_Context = boost::asio::io_context>
auto destroy_reg_handle(
    Component* in_instance, IO_Context& in_io_context = g_io_context(), const registry_ptr& in_reg_ptr = g_reg()
) {
  boost::asio::post(
      in_io_context,
      [l_h = entt::handle{*in_reg_ptr, entt::to_entity(*in_reg_ptr, *in_instance)}]() mutable { l_h.destroy(); }
  );
}
}  // namespace doodle