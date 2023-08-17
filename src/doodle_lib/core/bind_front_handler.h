//
// Created by td_main on 2023/8/14.
//

#pragma once
#include <boost/asio.hpp>

#include <type_traits>
namespace doodle {
namespace detail {
template <class Handler, typename Entt_Handler, typename Component, class... Args>
class bind_front_wrapper {
  //  using Handler_Class = boost::callable_traits::class_of_t<Handler>;
  Handler h_;
  Entt_Handler entt_handler_;
  std::tuple<Args...> args_;

  template <class T, class Executor>
  friend struct boost::asio::associated_executor;

  template <class T, class Allocator>
  friend struct boost::asio::associated_allocator;

  template <class T, class CancellationSlot>
  friend struct boost::asio::associated_cancellation_slot;

  template <std::size_t... I, class... Ts>
  void invoke(std::false_type, boost::mp11::index_sequence<I...>, Ts&&... ts) {
    h_(std::get<I>(std::move(args_))..., std::forward<Ts>(ts)...);
  }

  template <std::size_t... I, class... Ts>
  void invoke(std::true_type, boost::mp11::index_sequence<I...>, Ts&&... ts) {
    std::mem_fn(h_)(entt_handler_.template get<Component>(), std::get<I>(std::move(args_))..., std::forward<Ts>(ts)...);
  }

 public:
  using result_type                             = void;  // asio needs this

  bind_front_wrapper(bind_front_wrapper&&)      = default;
  bind_front_wrapper(bind_front_wrapper const&) = default;

  template <class Handler_, typename Entt_Handler_, class... Args_>
  explicit bind_front_wrapper(Handler_&& handler, Entt_Handler_&& in_entt_handler, Args_&&... args)
      : h_(std::forward<Handler_>(handler)),
        entt_handler_(std::forward<Entt_Handler_>(in_entt_handler)),
        args_(std::forward<Args_>(args)...) {}

  template <class... Ts>
  void operator()(Ts&&... ts) {
    invoke(
        std::is_member_function_pointer<Handler>{}, boost::mp11::index_sequence_for<Args...>{}, std::forward<Ts>(ts)...
    );
  }

  //

  template <class Function>
  friend boost::asio::asio_handler_invoke_is_deprecated asio_handler_invoke(Function&& f, bind_front_wrapper* op) {
    using boost::asio::asio_handler_invoke;
    return asio_handler_invoke(f, std::addressof(op->h_));
  }

  friend bool asio_handler_is_continuation(bind_front_wrapper* op) {
    using boost::asio::asio_handler_is_continuation;
    return asio_handler_is_continuation(std::addressof(op->h_));
  }

  friend boost::asio::asio_handler_allocate_is_deprecated asio_handler_allocate(
      std::size_t size, bind_front_wrapper* op
  ) {
    using boost::asio::asio_handler_allocate;
    return asio_handler_allocate(size, std::addressof(op->h_));
  }

  friend boost::asio::asio_handler_deallocate_is_deprecated asio_handler_deallocate(
      void* p, std::size_t size, bind_front_wrapper* op
  ) {
    using boost::asio::asio_handler_deallocate;
    return asio_handler_deallocate(p, size, std::addressof(op->h_));
  }
};

}  // namespace detail

template <typename Handler, typename Reg_Ptr, typename Component, typename... Args>
auto bind_reg_handler(Handler&& handler, Reg_Ptr& in_reg_ptr, Component* in_instance, Args&&... args) {
  return detail::bind_front_wrapper<
      std::decay_t<Handler>, decltype(entt::handle{*in_reg_ptr, entt::to_entity(*in_reg_ptr, *in_instance)}),
      std::decay_t<Component>, std::decay_t<Args>...>(
      std::forward<Handler>(handler), entt::handle{*in_reg_ptr, entt::to_entity(*in_reg_ptr, *in_instance)},
      std::forward<Args>(args)...
  );
}

template <typename Handler, typename Reg_Ptr, typename Component, typename... Args>
auto bind_reg_handler(Handler&& handler, Reg_Ptr& in_reg_ptr, Component*, entt::handle in_handle, Args&&... args) {
  using Component_Ptr = std::shared_ptr<std::decay_t<Component>>;
  return detail::bind_front_wrapper<std::decay_t<Handler>, entt::handle, Component_Ptr, std::decay_t<Args>...>(
      std::forward<Handler>(handler), in_handle, std::forward<Args>(args)...
  );
}

}  // namespace doodle
