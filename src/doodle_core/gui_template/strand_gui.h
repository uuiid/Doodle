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
        : timer_(in_executor) {}
    void ready_start();

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
  void shutdown() override;
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
                   gui_process_t&& in_gui);
  static void stop(const implementation_type& in_impl);
  void loop_one();

 private:
  // Mutex to protect access to the service-wide state
  std::recursive_mutex mutex_;
  std::atomic_bool stop_;
  // The head of a linked list of all implementations.
  std::shared_ptr<strand_impl> impl_list_;
};

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

  boost::asio::execution_context& context() const BOOST_ASIO_NOEXCEPT;

  void on_work_started() const BOOST_ASIO_NOEXCEPT;

  void on_work_finished() const BOOST_ASIO_NOEXCEPT ;
  void stop() ;

  void show(gui_process_t&& in_fun);

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
