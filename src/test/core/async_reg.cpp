#include <doodle_core/core/app_base.h>
#include <doodle_core/core/async_reg.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <entt/entt.hpp>

using namespace doodle;

BOOST_AUTO_TEST_SUITE(coroutine)

BOOST_AUTO_TEST_CASE(async_reg_1) {
  app_base l_app_base{};
  l_app_base.use_multithread();

  std::vector<entt::entity> l_entts{};
  l_entts.resize(std::size_t{100000000}, entt::null);
  g_reg()->create(l_entts.begin(), l_entts.end());
  // auto l_f1 = std::async(std::launch::async, [&]() -> boost::asio::awaitable<void> {
  //   std::vector<std::int32_t> l_values{};
  //   l_values.resize(l_entts.size(), 0);
  //   // auto l_exe = boost::asio::this_coro::executor;
  //   co_await async_reg::async_emplace_or_replace(*g_reg(), l_entts, l_values, boost::asio::use_awaitable);
  //   co_return;
  // });
  boost::asio::thread_pool l_pool;
  boost::asio::co_spawn(
      l_pool,
      [&]() -> boost::asio::awaitable<void> {
        // auto l_value = co_await async_get2<std::int32_t>(*g_reg(), l_entts, boost::asio::use_awaitable);
        auto l_value = co_await async_reg::async_get<std::int32_t>(*g_reg(), l_entts, boost::asio::use_awaitable);
        co_return;
      },
      boost::asio::detached
  );

  // auto l_f3 = std::async(std::launch::async, [&]() -> boost::asio::awaitable<void> {
  //   std::vector<std::int32_t> l_values{};
  //   l_values.resize(l_entts.size(), 0);
  //   co_await async_reg::async_destroy(*g_reg(), l_entts, boost::asio::use_awaitable);
  //   co_return;
  // });
  l_app_base.run();
}
BOOST_AUTO_TEST_SUITE_END()