//
// 协程用例支持：让 Boost.Test 的用例能直接 co_await。
//
// 为什么需要它：
//   1) 业务侧大量代码是 Boost.Asio 协程（awaitable<T>），但 BOOST_AUTO_TEST_CASE 的用例体
//      必须是普通 void() 函数 —— 协程函数的返回类型是 awaitable<...>，两者无法合并，
//      于是每个测试文件各自抄了一份 run_coro（ai_light_state.cpp / seedance2_task_pipeline.cpp）；
//   2) co_spawn + detached 会把协程里的异常吞掉（只记日志），单测需要看见失败；
//   3) 协程若 co_await 一个永远不会就绪的操作，run() 会一直阻塞（挂死 CI）；
//   4) app_base 的构造函数在 g_io_context() 上注册了 signal_set::async_wait（app_base.cpp:140-146），
//      该 context 永久持有 work，直接 g_io_context().run() 永远不返回。
//
// 这个头文件提供：
//   * run_coro(ctx, co|callable[, timeout])   —— 在自建 context 上把协程跑到结束；
//   * run_coro_on_app(co|callable[, timeout]) —— 在 g_io_context() 上跑，协程结束即 stop()；
//   * coro_fixture_t                          —— 自带 io_context 的用例 fixture 基类；
//   * DOODLE_TEST_CASE_CORO / DOODLE_FIXTURE_TEST_CASE_CORO —— 用例体直接写成协程。
//
// 约定：
//   * 协程里的异常原样重抛到用例体，由 Boost.Test 记账。BOOST_REQUIRE 抛出的
//     boost::execution_aborted 同样会被原样重抛，表现为「用例中止」而不是「未预期异常」；
//   * run() 返回时协程还没结束 → 判为失败，而不是静默通过；
//   * timeout > 0 时用 io_context::run_for 给这次 run 封顶，到点还没跑完就判失败（防挂死）；
//     缺省不限时，与改造前手写 run_coro 的行为一致 —— 即「把 context 抽干」；
//   * 判为超时 / 未完成时，协程仍挂在原来的异步操作上（比如一个没到期的 steady_timer），
//     那个操作还留在 context 里：不要再拿同一个 context 跑下一段（见 run_coro 的注释）。
//
// 使用示例：
//   DOODLE_TEST_CASE_CORO(some_case_name) {     // 自带 l_ctx
//     co_await something_async();
//     BOOST_CHECK_EQUAL(...);
//   }
//
//   BOOST_AUTO_TEST_CASE(一个用例里跑多段协程) {
//     fixture_t l_fx{...};                       // fixture 自带 ctx_，或者继承 coro_fixture_t
//     run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> { co_await 第一步(); });
//     run_coro(l_fx.ctx_, [&]() -> boost::asio::awaitable<void> { co_await 第二步(); });
//   }
//
#pragma once
#include <doodle_lib/core/global_function.h>  // g_io_context()

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_awaitable.hpp>  // 顺手带上，用例里 co_await post(...) 常用
#include <boost/test/unit_test.hpp>

#include <chrono>
#include <concepts>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace doodle_test {
namespace detail {

/// 协程的结束状态。放在堆上由协程帧与调用方共享：
/// 协程若一直挂起，它比 run_awaitable 的栈帧活得更久（直到 context 析构），栈上的对象不能给它写。
template <typename T>
struct slot_t {
  std::optional<T> value_{};
  std::exception_ptr error_{};
  bool finished_{false};
};

template <>
struct slot_t<void> {
  std::exception_ptr error_{};
  bool finished_{false};
};

template <typename T>
struct is_awaitable : std::false_type {};
template <typename T, typename E>
struct is_awaitable<boost::asio::awaitable<T, E>> : std::true_type {};

/// 毫秒数 → 字符串，只为把「等了多久」写进失败信息。
inline std::string ms_str(std::chrono::steady_clock::duration in_d) {
  return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(in_d).count()) + "ms";
}

/// 跑到底的实现。
/// @param in_stop_when_done 协程一结束就 stop() 掉 context。g_io_context() 上有常驻 work
///        （app_base 的 signal_set::async_wait），不 stop 就 run() 不返回。
/// @param in_timeout 0 表示不限时。> 0 时改用 io_context::run_for 封顶 —— 刻意不额外挂一个
///        定时器：那样会给 context 添加 work，让「跑完就抽干」的语义在没超时时也要等定时器到期。
template <typename T, typename E>
T run_awaitable(
    boost::asio::io_context& in_ctx, boost::asio::awaitable<T, E> in_co, bool in_stop_when_done,
    std::chrono::milliseconds in_timeout
) {
  auto l_slot = std::make_shared<slot_t<T>>();
  // 捕获裸指针而不是引用：协程帧里只存指针，避免「协程活得比这个函数长」时的悬垂引用。
  auto* l_ctx = &in_ctx;
  // 上一次 run() 结束后 context 处于 stopped 状态（例如 g_io_context() 被上一个用例 stop 过），
  // 不 restart 的话这里的 run() 会立刻返回。
  in_ctx.restart();

  boost::asio::co_spawn(
      in_ctx,
      [l_slot, l_ctx, in_stop_when_done, l_co = std::move(in_co)]() mutable -> boost::asio::awaitable<void> {
        try {
          if constexpr (std::is_void_v<T>) {
            co_await std::move(l_co);
          } else {
            l_slot->value_.emplace(co_await std::move(l_co));
          }
        } catch (...) {
          // 包括 BOOST_REQUIRE 抛出的 boost::execution_aborted：原样带回用例体。
          l_slot->error_ = std::current_exception();
        }
        l_slot->finished_ = true;
        if (in_stop_when_done) l_ctx->stop();
      },
      boost::asio::detached
  );

  const auto l_start = std::chrono::steady_clock::now();
  if (in_timeout.count() > 0) {
    in_ctx.run_for(in_timeout);
  } else {
    in_ctx.run();
  }
  const auto l_elapsed = std::chrono::steady_clock::now() - l_start;
  in_ctx.restart();

  if (!l_slot->finished_) {
    // 协程还挂在某个异步操作上，那个操作仍然留在 context 里 —— 这里只负责报错，不复用 context。
    if (in_timeout.count() > 0 && l_elapsed >= in_timeout)
      throw std::runtime_error{
          "run_coro 超时：协程没有在 " + std::to_string(in_timeout.count()) + "ms 内跑完（实际等了 " +
          ms_str(l_elapsed) + "）。它可能永远等不到结果，或者只是超时给得太短；" +
          "总之协程还挂在那个异步操作上，不要再复用这个 io_context"
      };
    throw std::runtime_error{
        "run_coro 失败：io_context 已经没有可跑的工作了，但协程还没结束（等了 " + ms_str(l_elapsed) +
        "）。多半是别处调用了 stop()（例如 app_base::run() 收尾时会把 io_context 停掉），" +
        "或者 co_await 了一个不注册 work 的操作"
    };
  }
  if (l_slot->error_) std::rethrow_exception(l_slot->error_);
  if constexpr (!std::is_void_v<T>) return std::move(*l_slot->value_);
}

}  // namespace detail

/// 在自建 io_context 上把协程跑到结束，异常原样重抛。
///
/// 兼容改造前手写的那个 run_coro（签名是 std::function<boost::asio::awaitable<void>()>）：
/// 用法一模一样，只是不必再在文件里抄一份实现。
///
/// @param in_timeout 0（缺省）表示不限时 —— 与旧实现一致，会把 context 里所有 work 抽干。
///                   若协程 await 了一个永不就绪但持有 work 的操作，缺省会一直阻塞在 run() 里；
///                   需要防挂死就显式给一个（例如 30s），到点没跑完就抛 std::runtime_error。
template <typename T, typename E>
T run_coro(
    boost::asio::io_context& in_ctx, boost::asio::awaitable<T, E> in_co,
    std::chrono::milliseconds in_timeout = std::chrono::milliseconds{0}
) {
  return detail::run_awaitable<T, E>(in_ctx, std::move(in_co), /*in_stop_when_done=*/false, in_timeout);
}

/// 便捷重载：直接传一个返回 awaitable 的 lambda / std::function（改造前手写 run_coro 的写法）。
template <typename F>
  requires std::invocable<F&> && detail::is_awaitable<std::invoke_result_t<F&>>::value
auto run_coro(
    boost::asio::io_context& in_ctx, F&& in_f, std::chrono::milliseconds in_timeout = std::chrono::milliseconds{0}
) {
  return run_coro(in_ctx, std::invoke(in_f), in_timeout);
}

/// 在 g_io_context() 上跑一段协程（被测代码内部用 g_io_context() 时用这个）。
///
/// 与 run_coro 的差别：协程一结束就 stop() 掉 context。因为 app_base 注册的 signal_set
/// 让 g_io_context() 永远有待处理工作，普通 run() 不会自己返回；拿到结果后 restart()，
/// 把 context 还原成「可以再次 run」的状态。
///
/// 前置条件：app_base（或 doodle_lib）已经构造出来，否则 g_io_context() 是空指针解引用。
/// 代价：stop() 会丢弃当时所有待处理的 handler —— 真跑起了 HTTP 服务 / websocket 的进程不适合用它。
template <typename T, typename E>
T run_coro_on_app(
    boost::asio::awaitable<T, E> in_co, std::chrono::milliseconds in_timeout = std::chrono::milliseconds{0}
) {
  return detail::run_awaitable<T, E>(::doodle::g_io_context(), std::move(in_co), /*in_stop_when_done=*/true, in_timeout);
}

/// 便捷重载：直接传一个返回 awaitable 的 lambda / std::function。
template <typename F>
  requires std::invocable<F&> && detail::is_awaitable<std::invoke_result_t<F&>>::value
auto run_coro_on_app(F&& in_f, std::chrono::milliseconds in_timeout = std::chrono::milliseconds{0}) {
  return run_coro_on_app(std::invoke(in_f), in_timeout);
}

/// 协程用例的 fixture 基类：拥有一个自建 io_context，并提供一个把协程跑到底的 run()。
///
///   struct my_fixture_t : doodle_test::coro_fixture_t { ... };
///   DOODLE_FIXTURE_TEST_CASE_CORO(某用例, my_fixture_t) { co_await ...; }
///
/// 注意：派生类不要再自己声明 ctx_，否则会遮蔽基类的同名成员（run() 用的是基类那个）。
struct coro_fixture_t {
  boost::asio::io_context ctx_{};

  coro_fixture_t()                                 = default;
  ~coro_fixture_t()                                = default;
  coro_fixture_t(const coro_fixture_t&)            = delete;
  coro_fixture_t& operator=(const coro_fixture_t&) = delete;
  coro_fixture_t(coro_fixture_t&&)                 = delete;
  coro_fixture_t& operator=(coro_fixture_t&&)      = delete;

  template <typename T, typename E>
  T run(
      boost::asio::awaitable<T, E> in_co, std::chrono::milliseconds in_timeout = std::chrono::milliseconds{0}
  ) {
    return run_coro(ctx_, std::move(in_co), in_timeout);
  }

  template <typename F>
    requires std::invocable<F&> && detail::is_awaitable<std::invoke_result_t<F&>>::value
  auto run(F&& in_f, std::chrono::milliseconds in_timeout = std::chrono::milliseconds{0}) {
    return run_coro(ctx_, std::forward<F>(in_f), in_timeout);
  }
};

/// 协程用例体的函数名（内部使用，宏要靠 ## 拼接出唯一名字）。
#define DOODLE_CORO_BODY_(name) doodle_coro_body_##name

/// 用例体直接写成协程，自带一个自建 io_context（协程体里叫 l_ctx）：
///
///   DOODLE_TEST_CASE_CORO(some_case_name) {
///     co_await boost::asio::post(l_ctx, boost::asio::use_awaitable);
///     BOOST_CHECK(...);
///   }
///
/// 限制：用例名必须是单个标识符（宏要拿它拼协程体函数名），不能用 "套件/用例" 那种写法。
/// 另外协程体是普通函数，fixture 成员要用参数访问；需要 fixture 就用下面那个宏。
#define DOODLE_TEST_CASE_CORO(name)                                                                \
  static ::boost::asio::awaitable<void> DOODLE_CORO_BODY_(name)(::boost::asio::io_context& l_ctx); \
  BOOST_AUTO_TEST_CASE(name) {                                                                     \
    ::boost::asio::io_context l_ctx{};                                                             \
    ::doodle_test::run_coro(l_ctx, DOODLE_CORO_BODY_(name)(l_ctx));                                \
  }                                                                                                \
  static ::boost::asio::awaitable<void> DOODLE_CORO_BODY_(name)(::boost::asio::io_context& l_ctx)

/// 带 fixture 的协程用例：fixture 需要提供 run(awaitable)（继承 coro_fixture_t 即可），
/// 协程体里通过参数 l_fx 访问 fixture（不是用 *this）。
#define DOODLE_FIXTURE_TEST_CASE_CORO(name, fixture)                             \
  static ::boost::asio::awaitable<void> DOODLE_CORO_BODY_(name)(fixture& l_fx);  \
  BOOST_FIXTURE_TEST_CASE(name, fixture) {                                       \
    auto& l_fx = *this;                                                          \
    l_fx.run(DOODLE_CORO_BODY_(name)(l_fx));                                     \
  }                                                                              \
  static ::boost::asio::awaitable<void> DOODLE_CORO_BODY_(name)(fixture& l_fx)

}  // namespace doodle_test
