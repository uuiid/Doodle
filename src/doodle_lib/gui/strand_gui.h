//
// Created by TD on 2022/6/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/core/app_base.h>
#include <doodle_core/gui_template/gui_process.h>
#include <boost/asio.hpp>

namespace doodle {

class strand_gui {
 public:
  typedef boost::asio::any_io_executor inner_executor_type;
  using Executor      = boost::asio::any_io_executor;
  using executor_type = boost::asio::any_io_executor;

  strand_gui()
      : executor_() {
  }

  template <typename Executor1>
  explicit strand_gui(const Executor1& in_e, std::enable_if_t<std::conditional<!std::is_same_v<Executor1, strand_gui>, std::is_convertible<Executor1, Executor>, std::false_type>::type::value, bool> = false)
      : executor_(in_e) {
  }
  template <typename Executor1>
  explicit strand_gui(Executor1& in_e, std::enable_if_t<!std::is_same_v<Executor1, strand_gui> && !std::is_convertible_v<Executor1, Executor>, bool> = false)
      : executor_(in_e.get_executor()) {
  }

#pragma region "复制移动函数"
  /// \brief 复制构造
  strand_gui(const strand_gui& other) BOOST_ASIO_NOEXCEPT
      : executor_(other.executor_) {
  }

  strand_gui& operator=(const strand_gui& other) BOOST_ASIO_NOEXCEPT {
    executor_ = other.executor_;

    return *this;
  }

  /// 移动构造

  strand_gui(strand_gui&& other) BOOST_ASIO_NOEXCEPT
      : executor_(BOOST_ASIO_MOVE_CAST(Executor)(other.executor_)) {
  }

  strand_gui& operator=(strand_gui&& other) BOOST_ASIO_NOEXCEPT {
    executor_ = BOOST_ASIO_MOVE_CAST(Executor)(other.executor_);
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
    using function_type = typename std::decay_t<Function>;
    auto l_ptr          = std::make_shared<function_type>(std::move(f));
    app_base::Get()._add_tick_([l_ptr](bool& in_b) {
      (*l_ptr)();
      in_b = true;
    });
  }

  template <typename Function, typename Allocator>
  void dispatch(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    using function_type = typename std::decay_t<Function>;
    auto l_ptr          = std::make_shared<function_type>(std::move(f));
    app_base::Get()._add_tick_([l_ptr](bool& in_b) {
      (*l_ptr)();
      in_b = true;
    });
  }

  template <typename Function, typename Allocator>
  void post(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    using function_type = typename std::decay_t<Function>;
    auto l_ptr          = std::make_shared<function_type>(std::move(f));
    app_base::Get()._add_tick_([l_ptr](bool& in_b) {
      (*l_ptr)();
      in_b = true;
    });
  }

  template <typename Function, typename Allocator>
  void defer(BOOST_ASIO_MOVE_ARG(Function) f, const Allocator& a) const {
    using function_type = typename std::decay_t<Function>;
    auto l_ptr          = std::make_shared<function_type>(std::move(f));
    app_base::Get()._add_tick_([l_ptr](bool& in_b) {
      (*l_ptr)();
      in_b = true;
    });
  }

  void stop();

  friend bool operator==(const strand_gui& a, const strand_gui& b) BOOST_ASIO_NOEXCEPT {
    return a.executor_ == b.executor_;
  }

  friend bool operator!=(const strand_gui& a, const strand_gui& b) BOOST_ASIO_NOEXCEPT {
    return a.executor_ != b.executor_;
  }

  inner_executor_type get_inner_executor() const BOOST_ASIO_NOEXCEPT;

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
