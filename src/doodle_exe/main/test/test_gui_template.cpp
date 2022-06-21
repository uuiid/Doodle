//
// Created by TD on 2022/6/20.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_lib/core/work_clock.h>

#include <boost/asio.hpp>

#include <catch.hpp>
#include <catch2/catch_approx.hpp>
namespace doodle {
enum class process_state : std::uint8_t {
  run = 1,
  succeed,
  fail
};

template <typename Process_t>
class process_warp_t {
#define DOODLE_TYPE_HASE_MFN(mfn_name)                           \
  template <typename type_t, typename = void>                    \
  struct has_##mfn_name##_fun : std::false_type {};              \
  template <typename type_t>                                     \
  struct has_##mfn_name##_fun<                                   \
      type_t,                                                    \
      std::void_t<decltype(std::declval<type_t&>().mfn_name())>> \
      : std::true_type {                                         \
  };

  DOODLE_TYPE_HASE_MFN(init)
  DOODLE_TYPE_HASE_MFN(succeeded)
  DOODLE_TYPE_HASE_MFN(failed)
  DOODLE_TYPE_HASE_MFN(aborted)

#undef DOODLE_TYPE_HASE_MFN

 protected:
  std::unique_ptr<void> process_attr;

  static void delete_gui_process_t(void* in_ptr) {
    delete static_cast<Process_t*>(in_ptr);
  };

  enum class state : std::uint8_t {
    uninitialized = 0,
    running,
    paused,
    succeeded,
    failed,
    aborted,
    finished,
    rejected
  };
  state current{state::uninitialized};

  template <typename Target                                     = Process_t,
            std::enable_if_t<has_init_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::uninitialized>)
      -> decltype(std::declval<Target>().init(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    l_gui.init();
    current = state::running;
  }
  template <typename Target                                      = Process_t,
            std::enable_if_t<!has_init_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::uninitialized>)
      -> decltype(std::declval<Target>().init(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::running;
  }

  template <typename Target = Process_t>
  auto next(std::integral_constant<state, state::running>)
      -> decltype(std::declval<Target>().update(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    switch (l_gui.update()) {
      case process_state::run:
        break;
      case process_state::succeed:
        current = state::succeeded;
        break;
      case process_state::fail:
        current = state::finished;
        break;
      default:
        break;
    }
  }

  template <typename Target                                          = Process_t,
            std::enable_if_t<has_succeeded_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::succeeded>)
      -> decltype(std::declval<Target>().succeeded(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    l_gui.succeeded();
    current = state::finished;
  }
  template <typename Target                                           = Process_t,
            std::enable_if_t<!has_succeeded_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::succeeded>)
      -> decltype(std::declval<Target>().succeeded(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::finished;
  }

  template <typename Target                                       = Process_t,
            std::enable_if_t<has_failed_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::failed>)
      -> decltype(std::declval<Target>().failed(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    l_gui.failed();
    current = state::rejected;
  }

  template <typename Target                                        = Process_t,
            std::enable_if_t<!has_failed_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::failed>)
      -> decltype(std::declval<Target>().failed(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::rejected;
  }

  template <typename Target                                        = Process_t,
            std::enable_if_t<has_aborted_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::aborted>)
      -> decltype(std::declval<Target>().aborted(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::rejected;
    l_gui.aborted();
  }

  template <typename Target                                         = Process_t,
            std::enable_if_t<!has_aborted_fun<Target>::value, bool> = false>
  auto next(std::integral_constant<state, state::aborted>)
      -> decltype(std::declval<Target>().aborted(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::rejected;
    l_gui.aborted();
  }

  void succeed() {
    if (alive()) {
      current = state::succeeded;
    }
  }

  void fail() {
    if (alive()) {
      current = state::failed;
    }
  }
  void pause() {
    if (current == state::running) {
      current = state::paused;
    }
  }
  void unpause() {
    if (current == state::paused) {
      current = state::running;
    }
  }

 public:
  template <typename... Args>
  explicit process_warp_t(Args... in_args)
      : process_attr(
            new Process_t{std::forward<Args>(in_args)...},
            &delete_gui_process_t) {}
  virtual ~process_warp_t() = default;

  void abort(const bool immediately = false) {
    if (alive()) {
      current = state::aborted;

      if (immediately) {
        (*this)();
      }
    }
  }
  [[nodiscard]] bool is_uninitialized() const {
    return current == state::uninitialized;
  }

  [[nodiscard]] bool alive() const {
    return current == state::running || current == state::paused;
  }
  [[nodiscard]] bool finished() const {
    return current == state::finished;
  }
  [[nodiscard]] bool paused() const {
    return current == state::paused;
  }
  [[nodiscard]] bool rejected() const {
    return current == state::rejected;
  }
  virtual process_state operator()() {
    process_state l_state{process_state::run};
    switch (current) {
      case state::uninitialized:
        next(std::integral_constant<state, state::uninitialized>{});
        break;
      case state::running:
        next(std::integral_constant<state, state::running>{});
        break;
      default:
        // suppress warnings
        break;
    }

    // if it's dead, it must be notified and removed immediately
    switch (current) {
      case state::succeeded: {
        next(std::integral_constant<state, state::succeeded>{});
        l_state = process_state::succeed;
      } break;
      case state::failed: {
        next(std::integral_constant<state, state::failed>{});
        l_state = process_state::fail;
      } break;
      case state::aborted: {
        next(std::integral_constant<state, state::aborted>{});
        l_state = process_state::fail;
      } break;
      default:
        // suppress warnings
        break;
    }
    return l_state;
  };
};

template <typename Gui_Process>
using gui_warp_t = process_warp_t<Gui_Process>;

template <typename Rear_Process>
using rear_warp_t = process_warp_t<Rear_Process>;

template <typename Lambda_Process, typename = void>
class lambda_process_warp_t : public Lambda_Process {
 public:
  lambda_process_warp_t() = default;

  process_state update();
};

template <typename Lambda_Process>
class lambda_process_warp_t<
    Lambda_Process,
    std::enable_if_t<
        std::is_same_v<
            typename std::invoke_result<Lambda_Process>::type, void>>>
    : public Lambda_Process {
 public:
  lambda_process_warp_t() = default;

  process_state update() {
    Lambda_Process::operator()();
    return process_state::succeed;
  }
};

template <typename Lambda_Process>
class lambda_process_warp_t<
    Lambda_Process,
    std::enable_if_t<
        std::is_same_v<
            typename std::invoke_result<Lambda_Process>::type, process_state>>>
    : public Lambda_Process {
 public:
  lambda_process_warp_t() = default;

  process_state update() {
    return Lambda_Process::operator()();
  }
};

namespace detail {

template <typename Rear_Process>
class gui_to_rear_warp_t {
  std::future<process_state> future_;
  rear_warp_t<Rear_Process> warp_process{};

 public:
  template <typename... Args>
  explicit gui_to_rear_warp_t(Args... in_args)
      : warp_process(std::forward<Args>(in_args)...) {}

  void init() {
    future_ = boost::asio::post(doodle::g_io_context(),
                                std::packaged_task<process_state()>{[this]() {
                                  return this->warp_process();
                                }});
  }

  process_state update() {
    process_state l_state{process_state::run};
    /// \brief 过程指针有效, 状态为未初始化, 未来无效
    if (future_.valid()) {
      switch (future_.wait_for(0ns)) {
        case std::future_status::ready: {
          try {
            future_.get();
            l_state = process_state::succeed;
          } catch (const doodle_error& error) {
            DOODLE_LOG_ERROR(error.what());
            l_state = process_state::fail;
          }
        } break;
        default:
          break;
      }
    } else {
      l_state = process_state::succeed;
    }
    return l_state;
  }
};

struct gui_process_wrap_handler;
using instance_type  = std::unique_ptr<void, void (*)(void*)>;
using abort_fn_type  = void(gui_process_wrap_handler&, bool);
using next_type      = std::unique_ptr<gui_process_wrap_handler>;
using update_fn_type = bool(gui_process_wrap_handler&);
struct gui_process_wrap_handler {
  instance_type instance;
  update_fn_type* update;
  abort_fn_type* abort;
  next_type next;
};
template <typename Gui_Process2>
static void delete_gui_process_t(void* in_ptr) {
  delete static_cast<Gui_Process2*>(in_ptr);
};
template <typename Gui_Process2>
static void abort(gui_process_wrap_handler& handler, const bool immediately) {
  static_cast<Gui_Process2*>(handler.instance.get())->abort(immediately);
}
template <typename Gui_Process2>
static bool update(gui_process_wrap_handler& handler) {
  auto&& process = *static_cast<Gui_Process2*>(handler.instance.get());

  switch (process()) {
    case process_state::run: {
    } break;
    case process_state::succeed: {
      if (process.next) {
        process = std::move(*process.next);
        // forces the process to exit the uninitialized state
        return process.update(process);
      }
    } break;
    case process_state::fail:
      return true;
      break;
    default:
      return true;
      break;
  }
  return false;
}

}  // namespace detail

class gui_process_t {
 private:
  using instance_type            = detail::instance_type;
  using abort_fn_type            = detail::abort_fn_type;
  using next_type                = detail::next_type;
  using update_fn_type           = detail::update_fn_type;
  using gui_process_wrap_handler = detail::gui_process_wrap_handler;

  gui_process_wrap_handler handle;
  next_type* auxiliary_next;

  struct continuation {
    explicit continuation(gui_process_t* in_self, gui_process_wrap_handler* in_handler)
        : self_(in_self),
          handler(in_handler) {}

   private:
    gui_process_t* self_;
    gui_process_wrap_handler* handler;
  };

  template <typename type_t, typename... Args>
  gui_process_t& _post_(Args... in_args) {
    auto l_next = instance_type{
        new type_t{std::forward<Args>(in_args)...},
        &detail::delete_gui_process_t<type_t>};
    auxiliary_next->reset(new gui_process_wrap_handler{
        std::move(l_next),
        &detail::abort<type_t>,
        &detail::update<type_t>,
        nullptr});
    auxiliary_next = &((*auxiliary_next)->next);
    return *this;
  };

 public:
  template <typename type_t, typename... Args>
  explicit gui_process_t(Args... in_args)
      : handle{
            instance_type{new gui_warp_t<type_t>{std::forward<Args>(in_args)...},
                          &detail::delete_gui_process_t<gui_warp_t<type_t>>},
            &detail::abort<gui_warp_t<type_t>>,
            &detail::update<gui_warp_t<type_t>>,
            nullptr} {}

  template <typename type_t, typename... Args>
  gui_process_t& then(Args... in_args) {
    return _post_<gui_warp_t<type_t>>(std::forward<Args>(in_args)...);
  };
  template <typename Func>
  gui_process_t& then(Func&& func) {
    return _post_<gui_warp_t<lambda_process_warp_t<Func>>>();
  };
  template <typename type_t, typename... Args>
  gui_process_t& post(Args... in_args) {
    return _post_<rear_warp_t<detail::gui_to_rear_warp_t<type_t>>>(std::forward<Args>(in_args)...);
  };
  template <typename Func>
  gui_process_t& post(Func&& func) {
    return _post_<rear_warp_t<detail::gui_to_rear_warp_t<lambda_process_warp_t<Func>>>>();
  };
  // 提交时的渲染过程
  void operator()() {
    handle.update(handle);
  }
};

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
class strand_gui_executor_service
    : public execution_context_service_base<strand_gui_executor_service> {
 public:
  class strand_impl {
   public:
    ~strand_impl();

   private:
    friend class strand_gui_executor_service;

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
    std::vector<gui_process_t> handlers{};
    std::vector<gui_process_t> handlers_next{};

    // The strand service in where the implementation is held.
    strand_gui_executor_service* service_;
    boost::asio::detail::strand_service d;
  };
  using implementation_type = std::shared_ptr<strand_impl>;

  /// \brief 初始化函数
  /// \param context 上下文
  explicit strand_gui_executor_service(boost::asio::execution_context& context);
  /// \brief 关机
  void shutdown(){};
  /// \brief 创建
  implementation_type create_implementation();

  // 调用给定的函数
  template <typename Executor, typename Function>
  static void execute(const implementation_type& impl, Executor& ex,
                      BOOST_ASIO_MOVE_ARG(Function) function,
                      typename std::enable_if_t<
                          boost::asio::can_query<Executor,
                                                 boost::asio::execution::allocator_t<void>>::value> = 0);

  template <typename Executor, typename Function>
  static void execute(const implementation_type& impl, Executor& ex,
                      BOOST_ASIO_MOVE_ARG(Function) function,
                      typename std::enable_if_t<
                          !boost::asio::can_query<Executor,
                                                  boost::asio::execution::allocator_t<void>>::value> = 0);

  // Request invocation of the given function.
  template <typename Executor, typename Function, typename Allocator>
  static void dispatch(const implementation_type& impl, Executor& ex,
                       BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a);

  // Request invocation of the given function and return immediately.
  template <typename Executor, typename Function, typename Allocator>
  static void post(const implementation_type& impl, Executor& ex,
                   BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a){};

  // Request invocation of the given function and return immediately.
  template <typename Executor, typename Function, typename Allocator>
  static void defer(const implementation_type& impl, Executor& ex,
                    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a){};

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
  static void do_execute(const implementation_type& impl, Executor& ex,
                         BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a){

  };

  // Mutex to protect access to the service-wide state
  std::recursive_mutex mutex_;

  // The head of a linked list of all implementations.
  strand_impl* impl_list_;
};

strand_gui_executor_service::strand_gui_executor_service(boost::asio::execution_context& context)
    : execution_context_service_base<strand_gui_executor_service>(context) {
}

template <typename Executor, typename Function>
void strand_gui_executor_service::execute(
    const strand_gui_executor_service::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    typename std::enable_if_t<
        boost::asio::can_query<Executor,
                               boost::asio::execution::allocator_t<void>>::value>) {
  return strand_gui_executor_service::do_execute(
      impl, ex,
      BOOST_ASIO_MOVE_CAST(Function)(function),
      boost::asio::query(ex, boost::asio::execution::allocator));
}

template <typename Executor, typename Function>
void strand_gui_executor_service::execute(
    const strand_gui_executor_service::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function,
    typename std::enable_if_t<
        !boost::asio::can_query<Executor,
                                boost::asio::execution::allocator_t<void>>::value>) {
  return strand_gui_executor_service::do_execute(
      impl, ex,
      BOOST_ASIO_MOVE_CAST(Function)(function),
      std::allocator<void>());
}

template <typename Executor, typename Function, typename Allocator>
void strand_gui_executor_service::dispatch(
    const strand_gui_executor_service::implementation_type& impl, Executor& ex,
    BOOST_ASIO_MOVE_ARG(Function) function, const Allocator& a) {
  using function_type = typename std::decay_t<Function>;


}

strand_gui_executor_service::strand_impl::~strand_impl() {
  std::lock_guard l_g{service_->mutex_};
}

strand_gui_executor_service::implementation_type
strand_gui_executor_service::create_implementation() {
  implementation_type new_impl = std::make_shared<strand_impl>();
  new_impl->locked_            = false;
  new_impl->shutdown_          = false;
  std::lock_guard l_g{mutex_};
  new_impl->mutex_   = &this->mutex_;

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
  explicit strand_gui(const Executor1& in_e,
                      std::enable_if_t<
                          std::conditional<
                              !std::is_same_v<Executor1, strand_gui>,
                              std::is_convertible<Executor1, Executor>,
                              std::false_type>::type::value,
                          bool> = false)
      : executor_(in_e),
        impl_(strand_gui::create_implementation(executor_)),
        strand_(boost::asio::make_strand(in_e)) {
  }

#pragma region "复制移动函数"
  /// \brief 复制构造
  strand_gui(const strand_gui& other) BOOST_ASIO_NOEXCEPT
      : executor_(other.executor_),
        impl_(other.impl_),
        strand_(other.strand_) {
  }

  template <class OtherExecutor>
  strand_gui(
      const strand_gui<OtherExecutor>& other) BOOST_ASIO_NOEXCEPT
      : executor_(other.executor_),
        impl_(other.impl_),
        strand_(other.strand_) {
  }

  strand_gui& operator=(const strand_gui& other) BOOST_ASIO_NOEXCEPT {
    executor_ = other.executor_;
    impl_     = other.impl_;
    strand_   = other.strand_;
    return *this;
  }

  template <class OtherExecutor>
  strand_gui& operator=(
      const strand_gui<OtherExecutor>& other) BOOST_ASIO_NOEXCEPT {
    executor_ = other.executor_;
    impl_     = other.impl_;
    strand_   = other.strand_;
    return *this;
  }

  /// 移动构造

  strand_gui(strand_gui&& other) BOOST_ASIO_NOEXCEPT
      : executor_(BOOST_ASIO_MOVE_CAST(Executor)(other.executor_)),
        impl_(BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_)),
        strand_(BOOST_ASIO_MOVE_CAST(boost::asio::strand<Executor>)(other.strand_)) {
  }

  template <class OtherExecutor>
  strand_gui(strand_gui<OtherExecutor>&& other) BOOST_ASIO_NOEXCEPT
      : executor_(BOOST_ASIO_MOVE_CAST(OtherExecutor)(other.executor_)),
        impl_(BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_)),
        strand_(BOOST_ASIO_MOVE_CAST(boost::asio::strand<Executor>)(other.strand_)) {
  }

  strand_gui& operator=(strand_gui&& other) BOOST_ASIO_NOEXCEPT {
    executor_ = BOOST_ASIO_MOVE_CAST(Executor)(other.executor_);
    impl_     = BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_);
    strand_   = BOOST_ASIO_MOVE_CAST(boost::asio::strand<Executor>)(other.strand_);
    return *this;
  }

  template <class OtherExecutor>
  strand_gui& operator=(strand_gui<OtherExecutor>&& other) BOOST_ASIO_NOEXCEPT {
    executor_ = BOOST_ASIO_MOVE_CAST(OtherExecutor)(other.executor_);
    impl_     = BOOST_ASIO_MOVE_CAST(implementation_type)(other.impl_);
    strand_   = BOOST_ASIO_MOVE_CAST(boost::asio::strand<Executor>)(other.strand_);
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
          boost::asio::is_nothrow_query<const Executor&, Property>::value)) {
    return this->query_helper(
        std::is_convertible_v<Property, boost::asio::execution::blocking_t>(), p);
  }

  template <typename Property>
  typename std::enable_if_t<
      boost::asio::can_require<const Executor&, Property>::value && !std::is_convertible_v<Property, boost::asio::execution::blocking_t::always_t>,
      strand_gui<typename std::decay_t<
          typename boost::asio::require_result<const Executor&, Property>::type>>>
  require(const Property& p) const
      BOOST_ASIO_NOEXCEPT_IF((
          boost::asio::is_nothrow_require<const Executor&, Property>::value)) {
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
          boost::asio::is_nothrow_prefer<const Executor&, Property>::value)) {
    return strand_gui<typename std::decay_t<
        typename boost::asio::prefer_result<
            const Executor&, Property>::type>>(
        boost::asio::prefer(executor_, p), impl_);
  }
#pragma endregion

  boost::asio::execution_context& context() const BOOST_ASIO_NOEXCEPT {
    return executor_.context();
  }

  void on_work_started() const BOOST_ASIO_NOEXCEPT {
    executor_.on_work_started();
  }

  void on_work_finished() const BOOST_ASIO_NOEXCEPT {
    executor_.on_work_finished();
  }

  template <typename Function>
  typename std::enable_if_t<
      boost::asio::execution::can_execute<const Executor&, Function>::value,
      void>
  execute(BOOST_ASIO_MOVE_ARG(Function) f) const {
    detail::strand_gui_executor_service::execute(
        impl_,
        executor_, BOOST_ASIO_MOVE_CAST(Function)(f));
  }

  template <typename Function, typename Allocator>
  void dispatch(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_gui_executor_service::dispatch(
        impl_,
        executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a);
  }

  template <typename Function, typename Allocator>
  void post(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_gui_executor_service::post(impl_,
                                              executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a);
  }

  template <typename Function, typename Allocator>
  void defer(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    detail::strand_gui_executor_service::defer(impl_,
                                               executor_, BOOST_ASIO_MOVE_CAST(Function)(f), a);
  }

  bool running_in_this_thread() const BOOST_ASIO_NOEXCEPT {
    return detail::strand_gui_executor_service::running_in_this_thread(impl_);
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

  typedef detail::strand_gui_executor_service::implementation_type
      implementation_type;

  template <typename InnerExecutor>
  static implementation_type create_implementation(
      const InnerExecutor& ex,
      typename std::enable_if<
          boost::asio::can_query<InnerExecutor,
                                 boost::asio::execution::context_t>::value,
          std::int32_t>::type = 0) {
    return boost::asio::use_service<detail::strand_gui_executor_service>(
               boost::asio::query(ex, boost::asio::execution::context))
        .create_implementation();
  }

  template <typename InnerExecutor>
  static implementation_type create_implementation(
      const InnerExecutor& ex,
      typename std::enable_if<
          !boost::asio::can_query<InnerExecutor,
                                  boost::asio::execution::context_t>::value,
          std::int32_t>::type = 0) {
    return boost::asio::use_service<detail::strand_gui_executor_service>(
               ex.context())
        .create_implementation();
  }

  strand_gui(const Executor& ex, const implementation_type& impl)
      : executor_(ex),
        impl_(impl) {
  }

  template <typename Property>
  typename boost::asio::query_result<const Executor&, Property>::type query_helper(
      std::false_type, const Property& property) const {
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
  boost::asio::strand<Executor> strand_;
  implementation_type impl_;
};
}  // namespace doodle
TEST_CASE("test gui strand") {
}
