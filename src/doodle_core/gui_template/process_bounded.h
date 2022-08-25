//
// Created by TD on 2022/7/11.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <boost/asio.hpp>
namespace doodle {
namespace process_bounded_ns {
template <typename Type>
class service_id
    : public boost::asio::execution_context::id {
};

template <typename Type>
class execution_context_service_base
    : public boost::asio::execution_context::service {
  using base_type_t = boost::asio::execution_context::service;

 public:
  static service_id<Type> id;

  // Constructor.
  execution_context_service_base(boost::asio::execution_context& e)
      : base_type_t(e) {
  }
};

template <typename Type>
service_id<Type> execution_context_service_base<Type>::id;
class process_bounded_server
    : public execution_context_service_base<process_bounded_server> {
 public:
  class strand_impl {
   public:
    virtual ~strand_impl() = default;

   private:
    friend class process_bounded_server;

    std::recursive_mutex* mutex_;

    std::vector<std::function<void()>> handlers{};
    std::vector<std::function<void()>> handlers_next{};
    std::atomic_int32_t size{(std::int32_t)std::thread::hardware_concurrency() - 2};

    // The strand service in where the implementation is held.
    process_bounded_server* service_{};
  };
  using implementation_type = std::shared_ptr<strand_impl>;

  /// \brief 初始化函数
  /// \param context 上下文
  explicit process_bounded_server(boost::asio::execution_context& context);
  /// \brief 关机
  void shutdown() override;
  /// \brief 创建

  implementation_type create_implementation();

  // 调用给定的函数
  template <typename Executor, typename Function>
  static void execute(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, typename std::enable_if_t<boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value> = 0);

  template <typename Executor, typename Function>
  static void execute(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, typename std::enable_if_t<!boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value> = 0);

  // Request invocation of the given function.
  template <typename Executor, typename Function, typename Allocator>
  static void dispatch(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a);

  // Request invocation of the given function and return immediately.
  template <typename Executor, typename Function, typename Allocator>
  static void post(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a);

  // Request invocation of the given function and return immediately.
  template <typename Executor, typename Function, typename Allocator>
  static void defer(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a);

  static void stop(const implementation_type& in_impl);
  void loop_one();

 private:
  // Helper函数请求调用给定函数
  template <typename Executor, typename Function, typename Allocator>
  static void do_execute(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a);

  // Mutex to protect access to the service-wide state
  std::recursive_mutex mutex_;

  // The head of a linked list of all implementations.
  std::shared_ptr<strand_impl> impl_list_;
};
template <typename Executor, typename Function>
void process_bounded_server::execute(
    const process_bounded_server::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    typename std::enable_if_t<
        boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value>
) {
  return process_bounded_server::do_execute(
      impl, ex,
      BOOST_ASIO_MOVE_CAST(Function)(function),
      boost::asio::query(ex, boost::asio::execution::allocator)
  );
}

template <typename Executor, typename Function>
void process_bounded_server::execute(
    const process_bounded_server::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    typename std::enable_if_t<
        !boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value>
) {
  return process_bounded_server::do_execute(
      impl, ex,
      BOOST_ASIO_MOVE_CAST(Function)(function),
      std::allocator<void>()
  );
}
template <typename Executor, typename Function, typename Allocator>
void process_bounded_server::do_execute(
    const implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  std::lock_guard l_k{*(impl->mutex_)};
  impl->handlers_next.emplace_back(std::move(function));
}
// Request invocation of the given function.
template <typename Executor, typename Function, typename Allocator>
void process_bounded_server::dispatch(
    const implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  std::lock_guard l_k{*(impl->mutex_)};
  impl->handlers_next.emplace_back(std::move(function));
}

// Request invocation of the given function and return immediately.
template <typename Executor, typename Function, typename Allocator>
void process_bounded_server::post(
    const implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  std::lock_guard l_k{*(impl->mutex_)};
  impl->handlers_next.emplace_back(std::move(function));
}

// Request invocation of the given function and return immediately.
template <typename Executor, typename Function, typename Allocator>
void process_bounded_server::defer(
    const implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  std::lock_guard l_k{*(impl->mutex_)};
  impl->handlers_next.emplace_back(std::move(function));
}

}  // namespace process_bounded_ns
class process_bounded {
};
}  // namespace doodle
