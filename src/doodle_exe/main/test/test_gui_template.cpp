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
}

class test_1 {
  std::int32_t p_{};
  std::int32_t p_max{10};

 public:
  explicit test_1(std::int32_t in_int) : p_(in_int){};

  void init() {
    DOODLE_LOG_INFO(" init {}", p_);
  }
  void succeeded() {
    DOODLE_LOG_INFO(" init {}", p_);
  };
  void failed() {
    DOODLE_LOG_INFO(" init {}", p_);
  };
  void aborted() {
    DOODLE_LOG_INFO(" init {}", p_);
  };
  doodle::process_state update() {
    DOODLE_LOG_INFO(" init {}", p_);
    if (p_ < p_max) {
      ++p_;
      return doodle::process_state::run;
    } else {
      return doodle::process_state::succeed;
    }
  };
};

namespace doodle {

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
  std::unique_ptr<void, void (*)(void*)> process_attr;

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
      -> decltype(void(), void()) {
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
      -> decltype(void(), void()) {
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
      -> decltype(void(), void()) {
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
      -> decltype(void(), void()) {
    auto&& l_gui = *static_cast<Process_t*>(process_attr.get());
    current      = state::rejected;
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
  explicit process_warp_t(Args&&... in_args)
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
  template <typename... Args>
  lambda_process_warp_t(Args&&... in_args){};

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
  template <typename... Args>
  lambda_process_warp_t(Args&&... in_args){};

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
  template <typename... Args>
  lambda_process_warp_t(Args&&... in_args){};

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
  explicit gui_to_rear_warp_t(Args&&... in_args)
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
using next_type      = std::unique_ptr<gui_process_wrap_handler>;
using abort_fn_type  = void(gui_process_wrap_handler&, bool);
using update_fn_type = bool(gui_process_wrap_handler&);
struct gui_process_wrap_handler {
  instance_type instance;
  abort_fn_type* abort;
  update_fn_type* update;
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
  auto&& l_process = *static_cast<Gui_Process2*>(handler.instance.get());

  switch (l_process()) {
    case process_state::run: {
    } break;
    case process_state::succeed: {
      if (handler.next) {
        handler = std::move(*handler.next);
      }
    } break;
    case process_state::fail:
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
  using next_type                = detail::next_type;
  using gui_process_wrap_handler = detail::gui_process_wrap_handler;

  next_type handle;
  next_type* auxiliary_next;

  template <typename type_t, typename... Args>
  gui_process_t& _post_(Args&&... in_args) {
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
  explicit gui_process_t()
      : handle{nullptr},
        auxiliary_next(&(handle)) {}

  template <typename type_t, typename... Args>
  gui_process_t& then(Args&&... in_args) {
    return _post_<gui_warp_t<type_t>>(std::forward<Args>(in_args)...);
  };
  template <typename Func>
  gui_process_t& then(Func&& func) {
    return _post_<gui_warp_t<lambda_process_warp_t<Func>>>();
  };
  template <typename type_t, typename... Args>
  gui_process_t& post(Args&&... in_args) {
    return _post_<rear_warp_t<detail::gui_to_rear_warp_t<type_t>>>(std::forward<Args>(in_args)...);
  };
  template <typename Func>
  gui_process_t& post(Func&& func) {
    return _post_<rear_warp_t<detail::gui_to_rear_warp_t<lambda_process_warp_t<Func>>>>();
  };
  // 提交时的渲染过程
  bool operator()() {
    return handle->update(*handle);
  }
  void abort(bool in_abort = false) {
    handle->abort(*handle, in_abort);
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
    template <typename Executor_T>
    explicit strand_impl(const Executor_T& in_executor)
        : timer_(in_executor) {
      static std::function<void(const boost::system::error_code& in_code)> s_fun{};
      s_fun = [&](const boost::system::error_code& in_code) {
        if (in_code == boost::asio::error::operation_aborted)
          return;
        if (!service_->stop_) {
          service_->loop_one();
        }
        timer_.expires_after(doodle::chrono::seconds{1} / 60);
        timer_.async_wait(s_fun);
      };
      timer_.expires_after(doodle::chrono::seconds{1} / 60);
      timer_.async_wait(s_fun);
    }

   private:
    friend class strand_gui_executor_service;

    // Mutex to protect access to internal data.
    std::recursive_mutex* mutex_;

    // 正在链上等待但在下次调度链之前不应运行的处理程序。只有在锁定互斥锁时，才能修改此队列
    //    boost::asio::op_queue<scheduler_operation> waiting_queue_;

    // 准备运行的处理程序。从逻辑上讲，这些是持有strand锁的处理程序。就绪队列仅从链中修改，因此可以在不锁定互斥锁的情况下访问
    std::vector<gui_process_t> handlers{};
    std::vector<gui_process_t> handlers_next{};

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
  void shutdown() {
    std::lock_guard l_g{mutex_};
    stop_ = true;
    for (auto&& i : impl_list_->handlers) {
      i.abort(true);
      i();
    }
    for (auto&& i : impl_list_->handlers_next) {
      i.abort(true);
      i();
    }
  };
  /// \brief 创建
  template <typename Executor_T>
  implementation_type create_implementation(const Executor_T& in_executor) {
    std::lock_guard l_g{mutex_};
    if (!impl_list_) {
      impl_list_           = std::make_shared<strand_impl>(in_executor);
      impl_list_->service_ = this;
    }
    return impl_list_;
  };

  static void show(const implementation_type& in_impl,
                   gui_process_t&& in_gui) {
    std::lock_guard l_g{in_impl->service_->mutex_};
    in_impl->handlers_next.emplace_back(std::move(in_gui));
  };

  void loop_one() {
    stop_ = true;
    std::lock_guard l_g{mutex_};
    std::move(impl_list_->handlers_next.begin(),
              impl_list_->handlers_next.end(), std::back_inserter(impl_list_->handlers));
    impl_list_->handlers_next.clear();
    if (impl_list_->handlers.empty())
      return;
    auto l_erase_benin = std::remove_if(
        impl_list_->handlers.begin(),
        impl_list_->handlers.end(),
        [&](typename decltype(this->impl_list_->handlers)::value_type& handler) -> bool {
          return handler();
        });
    if (l_erase_benin != impl_list_->handlers.end())
      impl_list_->handlers.erase(l_erase_benin,
                                 impl_list_->handlers.end());
  }

 private:
  // Mutex to protect access to the service-wide state
  std::recursive_mutex mutex_;
  std::atomic_bool stop_;
  // The head of a linked list of all implementations.
  std::shared_ptr<strand_impl> impl_list_;
};

strand_gui_executor_service::strand_gui_executor_service(boost::asio::execution_context& context)
    : execution_context_service_base<strand_gui_executor_service>(context),
      mutex_(),
      stop_(false) {
}

strand_gui_executor_service::strand_impl::~strand_impl() = default;

}  // namespace detail

class strand_gui {
 public:
  typedef boost::asio::any_io_executor inner_executor_type;
  using Executor = boost::asio::any_io_executor;

  //  strand_gui()
  //      : executor_(),
  //        impl_(strand_gui::create_implementation(executor_)) {
  //  }

  template <typename Executor1>
  explicit strand_gui(const Executor1& in_e,
                      std::enable_if_t<
                          std::conditional<
                              !std::is_same_v<Executor1, strand_gui>,
                              std::is_convertible<Executor1, Executor>,
                              std::false_type>::type::value,
                          bool> = false)
      : executor_(in_e),
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
      boost::asio::can_require<
          const Executor&, Property>::value &&
          !std::is_convertible_v<Property, boost::asio::execution::blocking_t::always_t>,
      strand_gui>
  require(const Property& p) const
      BOOST_ASIO_NOEXCEPT_IF((
          boost::asio::is_nothrow_require<const Executor&, Property>::value)) {
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
          boost::asio::is_nothrow_prefer<const Executor&, Property>::value)) {
    return strand_gui(
        boost::asio::prefer(executor_, p), impl_);
  }
#pragma endregion

  boost::asio::execution_context& context() const BOOST_ASIO_NOEXCEPT {
    return executor_.context();
  }

  void on_work_started() const BOOST_ASIO_NOEXCEPT {
    DOODLE_LOG_INFO("开始工作")
  }

  void on_work_finished() const BOOST_ASIO_NOEXCEPT {
    DOODLE_LOG_INFO("结束工作工作")
  }

  void show(gui_process_t&& in_fun) {
    detail::strand_gui_executor_service::show(impl_, std::move(in_fun));
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
        .create_implementation(ex);
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
  implementation_type impl_;
};

}  // namespace doodle

TEST_CASE("test gui strand") {
  boost::asio::io_context l_context{};
  doodle::strand_gui l_gui{l_context.get_executor()};
  doodle::gui_process_t l_process{};
  l_process.then<test_1>(1).post<test_1>(2).then([]() {
    DOODLE_LOG_INFO("end")
  });
  l_gui.show(std::move(l_process));
  //  boost::asio::post(l_gui, []() -> bool {
  //    DOODLE_LOG_INFO("dasd");
  //    return false;
  //  });
  //  boost::asio::post(l_gui, std::packaged_task<bool()>{[]() -> bool {
  //                      DOODLE_LOG_INFO("dasd");
  //                      return false;
  //                    }});
  //  l_context.run();
}
