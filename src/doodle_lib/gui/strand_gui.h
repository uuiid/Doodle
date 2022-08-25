//
// Created by TD on 2022/6/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/gui_template/gui_process.h>
#include <boost/asio.hpp>

namespace doodle {

namespace detail {

// Special derived service id type to keep classes header-file only.
template <typename Type>
class gui_service_id
    : public boost::asio::execution_context::id {
};

// Special service base class to keep classes header-file only.
template <typename Type>
class gui_execution_context_service_base
    : public boost::asio::execution_context::service {
  using base_type_t = boost::asio::execution_context::service;

 public:
  static gui_service_id<Type> id;

  // Constructor.
  gui_execution_context_service_base(boost::asio::execution_context& e)
      : base_type_t(e) {
  }
};

template <typename Type>
gui_service_id<Type> gui_execution_context_service_base<Type>::id;

///=================================================================
/**
 * @brief 此处开始事件循环
 */
class strand_gui_executor_service
    : public gui_execution_context_service_base<strand_gui_executor_service> {
 public:
  class strand_impl {
   public:
    template <typename Executor_T>
    explicit strand_impl(const Executor_T& in_executor)
        : timer_(in_executor) {
      ready_start();
    }
    virtual ~strand_impl();
    void ready_start();

   private:
    friend class strand_gui_executor_service;

    // Mutex to protect access to internal data.
    std::recursive_mutex* mutex_{};

    // 正在链上等待但在下次调度链之前不应运行的处理程序。只有在锁定互斥锁时，才能修改此队列
    //    boost::asio::op_queue<scheduler_operation> waiting_queue_;

    // 准备运行的处理程序。从逻辑上讲，这些是持有strand锁的处理程序。就绪队列仅从链中修改，因此可以在不锁定互斥锁的情况下访问
    std::vector<std::function<void()>> handlers{};
    std::vector<std::function<void()>> handlers_next{};

    // The strand service in where the implementation is held.
    strand_gui_executor_service* service_{};
    //    boost::asio::strand<boost::asio::any_io_executor> strand{};
    boost::asio::high_resolution_timer timer_;
  };
  using implementation_type = std::shared_ptr<strand_impl>;

  /// \brief 初始化函数
  /// \param context 上下文
  explicit strand_gui_executor_service(boost::asio::execution_context& context);
  /// \brief 关机
  void shutdown() override;
  /// \brief 创建
  template <typename Executor_T>
  implementation_type create_implementation(const Executor_T& in_executor) {
    std::lock_guard l_g{mutex_};
    if (!impl_list_) {
      impl_list_ = std::make_shared<strand_impl>(in_executor);
      boost::asio::get_associated_executor(this->context());
      impl_list_->service_ = this;
      impl_list_->mutex_   = &this->mutex_;
    }
    return impl_list_;
  };

  // 调用给定的函数
  template <typename Executor, typename Function>
  static void execute(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, typename std::enable_if_t<boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value, bool> = 0);

  template <typename Executor, typename Function>
  static void execute(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, typename std::enable_if_t<!boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value, bool> = 0);

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
  void render_begin();
  void render_end();

  // Helper函数请求调用给定函数
  template <typename Executor, typename Function, typename Allocator>
  static void do_execute(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a);

  // Mutex to protect access to the service-wide state
  std::recursive_mutex mutex_;
  // The head of a linked list of all implementations.
  std::shared_ptr<strand_impl> impl_list_;
};

template <typename Executor, typename Function>
void strand_gui_executor_service::execute(
    const strand_gui_executor_service::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    typename std::enable_if_t<
        boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value,
        bool>
) {
  return strand_gui_executor_service::do_execute(
      impl, ex,
      BOOST_ASIO_MOVE_CAST(Function)(function),
      std::allocator<void>()
  );
}

template <typename Executor, typename Function>
void strand_gui_executor_service::execute(
    const strand_gui_executor_service::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    typename std::enable_if_t<
        !boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value,
        bool>
) {
  return strand_gui_executor_service::do_execute(
      impl, ex,
      BOOST_ASIO_MOVE_CAST(Function)(function),
      std::allocator<void>()
  );
}

template <typename Executor, typename Function, typename Allocator>
void strand_gui_executor_service::do_execute(
    const implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  std::lock_guard l_k{*(impl->mutex_)};
  using function_type = typename std::decay_t<Function>;
  auto l_ptr          = std::make_shared<function_type>(std::move(function));
  impl->handlers_next.emplace_back([fun = l_ptr]() { (*fun)(); });
}
// Request invocation of the given function.
template <typename Executor, typename Function, typename Allocator>
void strand_gui_executor_service::dispatch(
    const implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  std::lock_guard l_k{*(impl->mutex_)};
  using function_type = typename std::decay_t<Function>;
  auto l_ptr          = std::make_shared<function_type>(std::move(function));
  impl->handlers_next.emplace_back([fun = l_ptr]() { (*fun)(); });
}

// Request invocation of the given function and return immediately.
template <typename Executor, typename Function, typename Allocator>
void strand_gui_executor_service::post(
    const implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  std::lock_guard l_k{*(impl->mutex_)};
  using function_type = typename std::decay_t<Function>;
  auto l_ptr          = std::make_shared<function_type>(std::move(function));
  impl->handlers_next.emplace_back([fun = l_ptr]() { (*fun)(); });
}

// Request invocation of the given function and return immediately.
template <typename Executor, typename Function, typename Allocator>
void strand_gui_executor_service::defer(
    const implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  std::lock_guard l_k{*(impl->mutex_)};
  using function_type = typename std::decay_t<Function>;
  auto l_ptr          = std::make_shared<function_type>(std::move(function));
  impl->handlers_next.emplace_back([fun = l_ptr]() { (*fun)(); });
}
}  // namespace detail

class strand_gui {
 public:
  typedef boost::asio::any_io_executor inner_executor_type;
  using Executor      = boost::asio::any_io_executor;
  using executor_type = boost::asio::any_io_executor;

  strand_gui()
      : executor_(),
        impl_(strand_gui::create_implementation(executor_)) {
  }

  template <typename Executor1>
  explicit strand_gui(const Executor1& in_e, std::enable_if_t<std::conditional<!std::is_same_v<Executor1, strand_gui>, std::is_convertible<Executor1, Executor>, std::false_type>::type::value, bool> = false)
      : executor_(in_e),
        impl_(strand_gui::create_implementation(executor_)) {
  }
  template <typename Executor1>
  explicit strand_gui(Executor1& in_e, std::enable_if_t<!std::is_same_v<Executor1, strand_gui> && !std::is_convertible_v<Executor1, Executor>, bool> = false)
      : executor_(in_e.get_executor()),
        impl_(strand_gui::create_implementation(executor_)) {
  }

#pragma region "复制移动函数"
  /// \brief 复制构造
  strand_gui(const strand_gui& other) BOOST_ASIO_NOEXCEPT
      : executor_(other.executor_),
        impl_(other.impl_) {
  }

  strand_gui& operator=(const strand_gui& other) BOOST_ASIO_NOEXCEPT {
    executor_ = other.executor_;
    impl_     = other.impl_;

    return *this;
  }

  /// 移动构造

  strand_gui(strand_gui&& other) BOOST_ASIO_NOEXCEPT
      : executor_(BOOST_ASIO_MOVE_CAST(Executor)(other.executor_)),
        impl_(BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_)) {
  }

  strand_gui& operator=(strand_gui&& other) BOOST_ASIO_NOEXCEPT {
    executor_ = BOOST_ASIO_MOVE_CAST(Executor)(other.executor_);
    impl_     = BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_);
    return *this;
  }

  ~strand_gui() BOOST_ASIO_NOEXCEPT = default;
#pragma endregion

#pragma region "自定义点"
  template <typename Property>
  typename std::enable_if_t<
      boost::asio::can_query<const Executor&, Property>::value,
      typename std::conditional_t<
          std::is_convertible_v<Property, boost::asio::execution::blocking_t>,
          boost::asio::execution::blocking_t,
          typename boost::asio::query_result<const Executor&, Property>::type>>
  query(const Property& p) const
      BOOST_ASIO_NOEXCEPT_IF((
          boost::asio::is_nothrow_query<const Executor&, Property>::value
      )) {
    return this->query_helper(
        std::is_convertible<Property, boost::asio::execution::blocking_t>(), p
    );
  }

  template <typename Property>
  typename std::enable_if_t<
      boost::asio::can_require<
          const Executor&, Property>::value &&
          !std::is_convertible_v<Property, boost::asio::execution::blocking_t::always_t>,
      strand_gui>
  require(const Property& p) const
      BOOST_ASIO_NOEXCEPT_IF((
          boost::asio::is_nothrow_require<const Executor&, Property>::value
      )) {
    return strand_gui(boost::asio::require(executor_, p), impl_);
  }

  template <typename Property>
  typename std::enable_if_t<
      boost::asio::can_prefer<
          const Executor&, Property>::value &&
          !std::is_convertible_v<
              Property,
              boost::asio::execution::blocking_t::always_t>,
      strand_gui>
  prefer(const Property& p) const
      BOOST_ASIO_NOEXCEPT_IF((
          boost::asio::is_nothrow_prefer<const Executor&, Property>::value
      )) {
    return strand_gui(
        boost::asio::prefer(executor_, p), impl_
    );
  }
#pragma endregion

  [[nodiscard]] boost::asio::execution_context& context() const BOOST_ASIO_NOEXCEPT;

  void on_work_started() const BOOST_ASIO_NOEXCEPT;

  void on_work_finished() const BOOST_ASIO_NOEXCEPT;

  template <typename Function>
  //  typename std::enable_if_t<
  //      boost::asio::execution::can_execute<executor_type, Function>::value,
  //      void>
  void execute(BOOST_ASIO_MOVE_ARG(Function) f) const {
    return detail::strand_gui_executor_service::execute(
        impl_,
        executor_, BOOST_ASIO_MOVE_CAST(Function)(f)
    );
  }

  template <typename Function, typename Allocator>
  void dispatch(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_gui_executor_service::dispatch(
        impl_,
        executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a
    );
  }

  template <typename Function, typename Allocator>
  void post(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_gui_executor_service::post(impl_, executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a);
  }

  template <typename Function, typename Allocator>
  void defer(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_gui_executor_service::defer(impl_, executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a);
  }

  void stop();

  friend bool operator==(const strand_gui& a, const strand_gui& b) BOOST_ASIO_NOEXCEPT {
    return a.impl_ == b.impl_;
  }

  friend bool operator!=(const strand_gui& a, const strand_gui& b) BOOST_ASIO_NOEXCEPT {
    return a.impl_ != b.impl_;
  }

  inner_executor_type get_inner_executor() const BOOST_ASIO_NOEXCEPT;

  typedef detail::strand_gui_executor_service::implementation_type
      implementation_type;

  template <typename InnerExecutor>
  static implementation_type create_implementation(
      const InnerExecutor& ex,
      typename std::enable_if<
          boost::asio::can_query<InnerExecutor, boost::asio::execution::context_t>::value,
          std::int32_t>::type = 0
  ) {
    return boost::asio::use_service<detail::strand_gui_executor_service>(
               boost::asio::query(ex, boost::asio::execution::context)
    )
        .create_implementation(ex);
  }

  template <typename InnerExecutor>
  static implementation_type create_implementation(
      const InnerExecutor& ex,
      typename std::enable_if<
          !boost::asio::can_query<InnerExecutor, boost::asio::execution::context_t>::value,
          std::int32_t>::type = 0
  ) {
    return boost::asio::use_service<detail::strand_gui_executor_service>(
               ex.context()
    )
        .create_implementation();
  }

  strand_gui(const Executor& ex, const implementation_type& impl)
      : executor_(ex),
        impl_(impl) {
  }

  template <typename Property>
  typename boost::asio::query_result<const Executor&, Property>::type query_helper(
      std::false_type, const Property& property
  ) const {
    return boost::asio::query(executor_, property);
  }

  template <typename Property>
  boost::asio::execution::blocking_t query_helper(std::true_type, const Property& property) const {
    boost::asio::execution::blocking_t result = boost::asio::query(executor_, property);
    return result == boost::asio::execution::blocking.always
               ? boost::asio::execution::blocking.possibly
               : result;
  }

  Executor executor_;
  implementation_type impl_;
};
}  // namespace doodle

namespace boost::asio::traits {

#if !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

template <typename F>
struct execute_member<::doodle::strand_gui, F> {
  static constexpr bool is_valid    = true;
  static constexpr bool is_noexcept = false;
  typedef void result_type;
};

#endif  // !defined(BOOST_ASIO_HAS_DEDUCED_EXECUTE_MEMBER_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

template <>
struct equality_comparable<::doodle::strand_gui> {
  static constexpr bool is_valid    = true;
  static constexpr bool is_noexcept = true;
};

#endif  // !defined(BOOST_ASIO_HAS_DEDUCED_EQUALITY_COMPARABLE_TRAIT)

#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)

template <>
struct query_member<::doodle::strand_gui, boost::asio::execution::context_t> {
  static constexpr bool is_valid    = true;
  static constexpr bool is_noexcept = true;
  typedef boost::asio::execution_context& result_type;
};

#endif  // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_MEMBER_TRAIT)
#if !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

template <typename Property>
struct query_static_constexpr_member<::doodle::strand_gui, Property, typename enable_if<std::is_convertible<Property, boost::asio::execution::blocking_t>::value>::type> {
  static constexpr bool is_valid    = true;
  static constexpr bool is_noexcept = true;
  typedef boost::asio::execution::blocking_t::never_t result_type;
  static constexpr result_type value() noexcept { return result_type(); }
};

#endif  // !defined(BOOST_ASIO_HAS_DEDUCED_QUERY_STATIC_CONSTEXPR_MEMBER_TRAIT)

}  // namespace boost::asio::traits
