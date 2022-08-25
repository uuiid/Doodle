//
// Created by TD on 2022/6/17.
//
//
// Created by TD on 2022/6/14.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/time_tool/work_clock.h>

#include <catch.hpp>
#include <catch2/catch_approx.hpp>

namespace detail {

// Special derived service id type to keep classes header-file only.
template <typename Type>
class service_id
    : public boost::asio::execution_context::id {
};

// Special service base class to keep classes header-file only.
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

///=================================================================
class strand_executor_service
    : public execution_context_service_base<strand_executor_service> {
 public:
  class strand_impl {
   public:
    ~strand_impl();

   private:
    friend class strand_executor_service;

    // Mutex to protect access to internal data.
    std::recursive_mutex* mutex_;

    // Indicates whether the strand is currently "locked" by a handler. This
    // means that there is a handler upcall in progress, or that the strand
    // itself has been scheduled in order to invoke some pending handlers.
    bool locked_;

    // Indicates that the strand has been shut down and will accept no further
    // handlers.
    bool shutdown_;

    // 正在链上等待但在下次调度链之前不应运行的处理程序。只有在锁定互斥锁时，才能修改此队列
    //    boost::asio::op_queue<scheduler_operation> waiting_queue_;

    // 准备运行的处理程序。从逻辑上讲，这些是持有strand锁的处理程序。就绪队列仅从链中修改，因此可以在不锁定互斥锁的情况下访问
    //    boost::asio::op_queue<scheduler_operation> ready_queue_;

    // Pointers to adjacent handle implementations in linked list.
    strand_impl* next_;
    strand_impl* prev_;

    // The strand service in where the implementation is held.
    strand_executor_service* service_;
  };
  using implementation_type = std::shared_ptr<strand_impl>;

  /// \brief 初始化函数
  /// \param context 上下文
  explicit strand_executor_service(boost::asio::execution_context& context);
  /// \brief 关机
  void shutdown(){};
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
  static void post(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a){};

  // Request invocation of the given function and return immediately.
  template <typename Executor, typename Function, typename Allocator>
  static void defer(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a){};

  // Determine whether the strand is running in the current thread.
  static bool running_in_this_thread(
      const implementation_type& impl
  ){};

 private:
  friend class strand_impl;
  template <typename F, typename Allocator>
  class allocator_binder;
  template <typename Executor, typename = void>
  class invoker;

  // 向链添加函数。如果获取锁，则返回true
  //  static bool enqueue(const implementation_type& impl,
  //                      scheduler_operation* op);

  // 将等待处理程序传输到就绪队列。如果传输了一个或多个处理程序，则返回true。
  //  static bool push_waiting_to_ready(implementation_type& impl);

  // 调用所有准备运行的处理程序
  //  static void run_ready_handlers(implementation_type& impl);

  // Helper函数请求调用给定函数
  template <typename Executor, typename Function, typename Allocator>
  static void do_execute(const implementation_type& impl, Executor& ex, BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a);

  // Mutex to protect access to the service-wide state
  std::recursive_mutex mutex_;

  // The head of a linked list of all implementations.
  strand_impl* impl_list_;
};
class std_fenced_block
    : private boost::noncopyable {
 public:
  enum half_t { half };
  enum full_t { full };

  // Constructor for a half fenced block.
  explicit std_fenced_block(half_t) {
  }

  // Constructor for a full fenced block.
  explicit std_fenced_block(full_t) {
    std::atomic_thread_fence(std::memory_order_acquire);
  }

  // Destructor.
  ~std_fenced_block() {
    std::atomic_thread_fence(std::memory_order_release);
  }
};
using fenced_block = std_fenced_block;

strand_executor_service::strand_executor_service(boost::asio::execution_context& context)
    : execution_context_service_base<strand_executor_service>(context) {
}

template <typename Executor, typename Function>
void strand_executor_service::execute(
    const strand_executor_service::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    typename std::enable_if_t<
        boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value>
) {
  //  return do_execute(
  //      impl,
  //      ex,
  //      BOOST_ASIO_MOVE_CAST(Function)(function),
  //      std::allocator<void>());
  return strand_executor_service::do_execute(
      impl, ex,
      BOOST_ASIO_MOVE_CAST(Function)(function),
      boost::asio::query(ex, boost::asio::execution::allocator)
  );
}

template <typename Executor, typename Function>
void strand_executor_service::execute(
    const strand_executor_service::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    typename std::enable_if_t<
        !boost::asio::can_query<Executor, boost::asio::execution::allocator_t<void>>::value>
) {
  return strand_executor_service::do_execute(
      impl, ex,
      BOOST_ASIO_MOVE_CAST(Function)(function),
      std::allocator<void>()
  );
}

template <typename Executor, typename Function, typename Allocator>
void strand_executor_service::dispatch(
    const strand_executor_service::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a
) {
  using function_type = typename std::decay_t<Function>;
  // 如果我们已经在链中，那么函数可以立即运行。
  if (running_in_this_thread(impl)) {
    // 制作函数的本地非常量副本。
    function_type tmp(BOOST_ASIO_MOVE_CAST(Function)(function));

    fenced_block b(fenced_block::full);
    boost_asio_handler_invoke_helpers::invoke(tmp, tmp);
    return;
  }
}

strand_executor_service::strand_impl::~strand_impl() {
  std::lock_guard l_g{service_->mutex_};
}

strand_executor_service::implementation_type
strand_executor_service::create_implementation() {
  implementation_type new_impl = std::make_shared<strand_impl>();
  new_impl->locked_            = false;
  new_impl->shutdown_          = false;

  std::lock_guard l_g{mutex_};

  new_impl->mutex_   = &this->mutex_;

  // Insert implementation into linked list of all implementations.
  new_impl->next_    = impl_list_;
  new_impl->prev_    = 0;
  //  if (impl_list_)
  //    impl_list_->prev_ = new_impl.get();
  //  impl_list_         = new_impl.get();
  new_impl->service_ = this;

  return new_impl;
}

}  // namespace detail

template <typename Executor>
class strand_gui {
 public:
  typedef Executor inner_executor_type;

  strand_gui()
      : executor_(),
        impl_(strand_gui::create_implementation(executor_)) {
  }

  template <typename Executor1>
  explicit strand_gui(const Executor1& in_e, std::enable_if_t<std::conditional<!std::is_same_v<Executor1, strand_gui>, std::is_convertible<Executor1, Executor>, std::false_type>::type::value, bool> = false)
      : executor_(in_e),
        impl_(strand_gui::create_implementation(executor_)) {
  }

#pragma region "复制移动函数"
  /// \brief 复制构造
  strand_gui(const strand_gui& other) BOOST_ASIO_NOEXCEPT
      : executor_(other.executor_),
        impl_(other.impl_) {
  }

  template <class OtherExecutor>
  strand_gui(
      const strand_gui<OtherExecutor>& other
  ) BOOST_ASIO_NOEXCEPT
      : executor_(other.executor_),
        impl_(other.impl_) {
  }

  strand_gui& operator=(const strand_gui& other) BOOST_ASIO_NOEXCEPT {
    executor_ = other.executor_;
    impl_     = other.impl_;
    return *this;
  }

  template <class OtherExecutor>
  strand_gui& operator=(
      const strand_gui<OtherExecutor>& other
  ) BOOST_ASIO_NOEXCEPT {
    executor_ = other.executor_;
    impl_     = other.impl_;
    return *this;
  }

  /// 移动构造

  strand_gui(strand_gui&& other) BOOST_ASIO_NOEXCEPT
      : executor_(BOOST_ASIO_MOVE_CAST(Executor)(other.executor_)),
        impl_(BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_)) {
  }

  template <class OtherExecutor>
  strand_gui(strand_gui<OtherExecutor>&& other) BOOST_ASIO_NOEXCEPT
      : executor_(BOOST_ASIO_MOVE_CAST(OtherExecutor)(other.executor_)),
        impl_(BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_)) {
  }

  strand_gui& operator=(strand_gui&& other) BOOST_ASIO_NOEXCEPT {
    executor_ = BOOST_ASIO_MOVE_CAST(Executor)(other.executor_);
    impl_     = BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_);
    return *this;
  }

  template <class OtherExecutor>
  strand_gui& operator=(strand_gui<OtherExecutor>&& other) BOOST_ASIO_NOEXCEPT {
    executor_ = BOOST_ASIO_MOVE_CAST(OtherExecutor)(other.executor_);
    impl_     = BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_);
    return *this;
  }

  ~strand_gui() BOOST_ASIO_NOEXCEPT {
  }
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
        std::is_convertible_v<Property, boost::asio::execution::blocking_t>(), p
    );
  }

  template <typename Property>
  typename std::enable_if_t<
      boost::asio::can_require<const Executor&, Property>::value && !std::is_convertible_v<Property, boost::asio::execution::blocking_t::always_t>,
      strand_gui<typename std::decay_t<
          typename boost::asio::require_result<const Executor&, Property>::type>>>
  require(const Property& p) const
      BOOST_ASIO_NOEXCEPT_IF((
          boost::asio::is_nothrow_require<const Executor&, Property>::value
      )) {
    return strand_gui<typename std::decay_t<
        typename boost::asio::require_result<const Executor&, Property>::type>>(boost::asio::require(executor_, p), impl_);
  }

  template <typename Property>
  typename std::enable_if_t<
      boost::asio::can_prefer<
          const Executor&, Property>::value &&
          !std::is_convertible_v<
              Property,
              boost::asio::execution::blocking_t::always_t>,
      strand_gui<typename std::decay_t<
          typename boost::asio::prefer_result<const Executor&, Property>::type>>>
  prefer(const Property& p) const
      BOOST_ASIO_NOEXCEPT_IF((
          boost::asio::is_nothrow_prefer<const Executor&, Property>::value
      )) {
    return strand_gui<typename std::decay_t<
        typename boost::asio::prefer_result<
            const Executor&, Property>::type>>(
        boost::asio::prefer(executor_, p), impl_
    );
  }
#pragma endregion

  boost::asio::execution_context& context() const BOOST_ASIO_NOEXCEPT {
    return executor_.context();
  }

  void on_work_started() const BOOST_ASIO_NOEXCEPT {
    DOODLE_LOG_INFO("开始任务")
    executor_.on_work_started();
  }

  void on_work_finished() const BOOST_ASIO_NOEXCEPT {
    DOODLE_LOG_INFO("结束任务")
    executor_.on_work_finished();
  }

  template <typename Function>
  typename std::enable_if_t<
      boost::asio::execution::can_execute<const Executor&, Function>::value,
      void>
  execute(BOOST_ASIO_MOVE_ARG(Function) f) const {
    detail::strand_executor_service::execute(
        impl_,
        executor_, BOOST_ASIO_MOVE_CAST(Function)(f)
    );
  }

  template <typename Function, typename Allocator>
  void dispatch(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_executor_service::dispatch(
        impl_,
        executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a
    );
  }

  template <typename Function, typename Allocator>
  void post(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_executor_service::post(impl_, executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a);
  }

  template <typename Function, typename Allocator>
  void defer(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_executor_service::defer(impl_, executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a);
  }

  friend bool operator==(const strand_gui& a, const strand_gui& b) BOOST_ASIO_NOEXCEPT {
    return a.impl_ == b.impl_;
  }

  friend bool operator!=(const strand_gui& a, const strand_gui& b) BOOST_ASIO_NOEXCEPT {
    return a.impl_ != b.impl_;
  }

  inner_executor_type get_inner_executor() const BOOST_ASIO_NOEXCEPT {
    return executor_;
  }

  typedef detail::strand_executor_service::implementation_type
      implementation_type;

  template <typename InnerExecutor>
  static implementation_type create_implementation(
      const InnerExecutor& ex,
      typename std::enable_if<
          boost::asio::can_query<InnerExecutor, boost::asio::execution::context_t>::value,
          std::int32_t>::type = 0
  ) {
    return boost::asio::use_service<detail::strand_executor_service>(
               boost::asio::query(ex, boost::asio::execution::context)
    )
        .create_implementation();
  }

  template <typename InnerExecutor>
  static implementation_type create_implementation(
      const InnerExecutor& ex,
      typename std::enable_if<
          !boost::asio::can_query<InnerExecutor, boost::asio::execution::context_t>::value,
          std::int32_t>::type = 0
  ) {
    return boost::asio::use_service<detail::strand_executor_service>(
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

TEST_CASE("test boost gui strand") {
  boost::asio::io_context l_context{};
  strand_gui<decltype(l_context)::executor_type> l_gui{l_context.get_executor()};
  boost::asio::post(l_gui, []() -> bool {
    DOODLE_LOG_INFO("dasd");
    return false;
  });
  boost::asio::post(l_gui, std::packaged_task<bool()>{[]() -> bool {
                      DOODLE_LOG_INFO("dasd");
                      return false;
                    }});
  l_context.run();
}

TEST_CASE("test boost use service") {
  boost::asio::io_context l_context{};
  auto&& l_s = boost::asio::use_service<detail::strand_executor_service>(l_context);
}

template <typename CompletionToken>
auto async_wait_callback(CompletionToken&& token) {
  return boost::asio::async_initiate<CompletionToken, bool(void)>(
      [](auto&& completion_handler) {

      },
      token
  );
}

TEST_CASE("test boost bind_executor1") {
  boost::asio::io_context l_context{};
  strand_gui<decltype(l_context)::executor_type> l_gui{l_context.get_executor()};
  auto l_e = boost::asio::bind_executor(l_gui, []() {});
  boost::asio::post(l_e);
  boost::asio::post(l_gui, []() { DOODLE_LOG_INFO("test"); return true; });
}
TEST_CASE("test boost bind_executor3") {
  boost::asio::io_context l_context{};

  auto l_e = boost::asio::bind_executor(boost::asio::make_strand(l_context), []() {});
  async_wait_callback([](std::int32_t in) -> bool { return false; });
  async_wait_callback([]() {});
  //  boost::asio::async_initiate<std::packaged_task<bool()>, bool()>(
  //      [](auto&& in_handler) -> bool { return false; }, std::packaged_task<bool()>{});
  auto&& l_s = boost::asio::use_service<detail::strand_executor_service>(l_context);
}

TEST_CASE("test boost strand2") {
  boost::asio::io_context l_context{};

  {
    auto l_s = boost::asio::make_strand(l_context);

    boost::asio::post(l_s, []() {
      DOODLE_LOG_INFO("开始工作");
    });
  }
  l_context.run();
}

TEST_CASE("test boost can_exe") {
  std::cout << std::boolalpha << boost::asio::execution::can_execute<boost::asio::any_io_executor, void()>::value;
  std::cout << std::boolalpha << boost::asio::query(boost::asio::any_io_executor{}, boost::asio::execution::allocator);
}
