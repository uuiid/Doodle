//
// 协程用例支持自测：<test/test_coro.h> 里的 run_coro / run_coro_on_app 与两个宏。
//
// 这是「测试基建自己的测试」：改过 test_coro.h 之后先跑它，再跑业务用例。
// 覆盖：值返回（含 move-only）、void、异常（含 BOOST_REQUIRE 用的 execution_aborted）、
//       「协程没跑完」的识别、超时、在 g_io_context() 上跑、两个宏。
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/global_function.h>

#include <test/test_coro.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

BOOST_AUTO_TEST_SUITE(coro_test_support)

namespace {

using namespace doodle;

/// 跑一段「应该失败」的协程，返回 run_coro 抛出的 runtime_error 文本。
/// 用它而不是 BOOST_CHECK_THROW：失败路径有两种（超时 / 没跑完），只看「抛没抛」分不出是哪种。
template <typename F>
std::string failure_message(F&& in_f) {
  try {
    std::forward<F>(in_f)();
  } catch (const std::runtime_error& l_e) {
    return std::string{l_e.what()};
  } catch (const std::exception& l_e) {
    BOOST_FAIL(std::string{"抛的不是 runtime_error："} + l_e.what());
  }
  BOOST_FAIL("应该抛异常，但没有");
  return {};
}

/// 计时用：失败路径的判据之一是「没有真的等到那么久」。
std::chrono::milliseconds elapsed_since(const std::chrono::steady_clock::time_point& in_start) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - in_start);
}

/// 带 fixture 的宏要用的 fixture：继承 coro_fixture_t 就有了 ctx_ 与 run()。
struct counter_fixture_t : doodle_test::coro_fixture_t {
  int value_{0};
};

}  // namespace

// ─── 两个宏 ─────────────────────────────────────────────────────────────────

DOODLE_TEST_CASE_CORO(macro_generated_case_body_can_co_await) {
  bool l_resumed{false};
  // 先挂起一次再往下走：证明用例体真的是协程，而不是被同步跑完的
  co_await boost::asio::post(l_ctx, boost::asio::use_awaitable);
  l_resumed = true;
  BOOST_CHECK(l_resumed);
}

DOODLE_FIXTURE_TEST_CASE_CORO(macro_generated_fixture_case_can_co_await, counter_fixture_t) {
  l_fx.value_ += 1;
  boost::asio::steady_timer l_timer{l_fx.ctx_, std::chrono::milliseconds{1}};
  co_await l_timer.async_wait(boost::asio::use_awaitable);
  l_fx.value_ += 1;
  BOOST_CHECK_EQUAL(l_fx.value_, 2);
}

// ─── run_coro ───────────────────────────────────────────────────────────────

BOOST_AUTO_TEST_CASE(value_is_returned) {
  boost::asio::io_context l_ctx{};
  BOOST_CHECK_EQUAL(doodle_test::run_coro(l_ctx, []() -> boost::asio::awaitable<int> { co_return 42; }), 42);
}

BOOST_AUTO_TEST_CASE(move_only_value_is_returned) {
  boost::asio::io_context l_ctx{};
  auto l_p = doodle_test::run_coro(l_ctx, []() -> boost::asio::awaitable<std::unique_ptr<int>> {
    co_return std::make_unique<int>(7);
  });
  BOOST_REQUIRE(l_p != nullptr);
  BOOST_CHECK_EQUAL(*l_p, 7);
}

BOOST_AUTO_TEST_CASE(legacy_std_function_signature_still_works) {
  boost::asio::io_context l_ctx{};
  bool l_ran{false};
  // ai_light_state.cpp / seedance2_task_pipeline.cpp 当初抄的就是这一份签名，改造后必须无痛兼容
  std::function<boost::asio::awaitable<void>()> l_fun = [&]() -> boost::asio::awaitable<void> {
    co_await boost::asio::post(l_ctx, boost::asio::use_awaitable);
    l_ran = true;
  };
  doodle_test::run_coro(l_ctx, l_fun);
  BOOST_CHECK(l_ran);
}

BOOST_AUTO_TEST_CASE(exception_from_coroutine_comes_back_to_the_case) {
  boost::asio::io_context l_ctx{};
  BOOST_CHECK_THROW(
      doodle_test::run_coro(l_ctx, [&]() -> boost::asio::awaitable<void> {
        // 先挂起一次再抛：确认「异常跨过挂起点」也能带回来
        co_await boost::asio::post(l_ctx, boost::asio::use_awaitable);
        throw std::runtime_error{"编排层抛了 501"};
      }),
      std::runtime_error
  );
  // 失败之后 context 还能继续用（旧实现里那句 restart() 的语义）
  BOOST_CHECK_EQUAL(doodle_test::run_coro(l_ctx, []() -> boost::asio::awaitable<int> { co_return 1; }), 1);
}

BOOST_AUTO_TEST_CASE(execution_aborted_keeps_its_type) {
  // BOOST_REQUIRE 失败时抛的正是这个类型：类型必须原样保住，否则协程里的 BOOST_REQUIRE
  // 会被 Boost.Test 记成「未预期异常」，而不是「用例中止」。
  boost::asio::io_context l_ctx{};
  BOOST_CHECK_THROW(
      doodle_test::run_coro(l_ctx, []() -> boost::asio::awaitable<void> { throw boost::execution_aborted{}; }),
      boost::execution_aborted
  );
}

BOOST_AUTO_TEST_CASE(abandoned_coroutine_is_reported_as_failure) {
  boost::asio::io_context l_ctx{};
  bool l_body_finished{false};
  // 20ms 后把 context 停掉：run() 会返回，而协程还挂在 5 秒的定时器上。
  // 这正是「别处调用了 stop()」的场景 —— app_base::run() 收尾时就会把 io_context 停掉。
  boost::asio::steady_timer l_killer{l_ctx, std::chrono::milliseconds{20}};
  l_killer.async_wait([&](const boost::system::error_code&) { l_ctx.stop(); });

  const auto l_msg = failure_message([&] {
    doodle_test::run_coro(l_ctx, [&]() -> boost::asio::awaitable<void> {
      boost::asio::steady_timer l_slow{l_ctx, std::chrono::seconds{5}};
      co_await l_slow.async_wait(boost::asio::use_awaitable);
      l_body_finished = true;
    });
  });
  BOOST_CHECK(!l_body_finished);
  BOOST_CHECK_MESSAGE(l_msg.find("还没结束") != std::string::npos, "失败信息不对：" << l_msg);
}

BOOST_AUTO_TEST_CASE(timeout_is_reported_instead_of_hanging) {
  boost::asio::io_context l_ctx{};
  const auto l_start = std::chrono::steady_clock::now();
  const auto l_msg   = failure_message([&] {
    doodle_test::run_coro(
        l_ctx,
        [&]() -> boost::asio::awaitable<void> {
          // 30 秒才到期的定时器：不设超时的话 run() 会真的等满 30 秒
          boost::asio::steady_timer l_slow{l_ctx, std::chrono::seconds{30}};
          co_await l_slow.async_wait(boost::asio::use_awaitable);
        },
        std::chrono::milliseconds{50}
    );
  });
  BOOST_CHECK_MESSAGE(l_msg.find("超时") != std::string::npos, "失败信息不对：" << l_msg);
  BOOST_CHECK_LT(elapsed_since(l_start).count(), 5000);
}

BOOST_AUTO_TEST_CASE(timeout_that_is_not_hit_does_not_slow_the_case_down) {
  // 给了 30 秒超时但协程立刻就返回：不能因为「挂了定时器」而白等一场。
  // 这条是 io_context::run_for 方案相对「额外挂一个 steady_timer」的回归测试。
  boost::asio::io_context l_ctx{};
  const auto l_start = std::chrono::steady_clock::now();
  const auto l_v =
      doodle_test::run_coro(l_ctx, []() -> boost::asio::awaitable<int> { co_return 42; }, std::chrono::seconds{30});
  BOOST_CHECK_EQUAL(l_v, 42);
  BOOST_CHECK_LT(elapsed_since(l_start).count(), 5000);
}

// ─── run_coro_on_app ────────────────────────────────────────────────────────

BOOST_AUTO_TEST_CASE(runs_on_the_app_io_context) {
  app_base l_app{};  // g_io_context() 得先存在（app_base 的构造函数建 doodle_lib）
  // g_io_context() 上有常驻 work（signal_set::async_wait），所以这里靠「协程结束就 stop()」回来，
  // 而不是靠「没活了」—— 协程里那句 post 用来证明它确实跑在 g_io_context() 上。
  BOOST_CHECK_EQUAL(
      doodle_test::run_coro_on_app([]() -> boost::asio::awaitable<int> {
        co_await boost::asio::post(g_io_context(), boost::asio::use_awaitable);
        co_return 7;
      }),
      7
  );
  // 第二次照样能跑：说明拿到结果后 context 被 restart 回了可用状态
  BOOST_CHECK_EQUAL(
      doodle_test::run_coro_on_app([]() -> boost::asio::awaitable<int> {
        co_await boost::asio::post(g_io_context(), boost::asio::use_awaitable);
        co_return 8;
      }),
      8
  );
}

// ─── coro_fixture_t ─────────────────────────────────────────────────────────

BOOST_AUTO_TEST_CASE(fixture_run_returns_values) {
  struct fixture_t : doodle_test::coro_fixture_t {
    int base_{40};
  } l_fx{};
  const auto l_v = l_fx.run([&]() -> boost::asio::awaitable<int> {
    co_await boost::asio::post(l_fx.ctx_, boost::asio::use_awaitable);
    co_return l_fx.base_ + 2;
  });
  BOOST_CHECK_EQUAL(l_v, 42);
}

BOOST_AUTO_TEST_SUITE_END()
