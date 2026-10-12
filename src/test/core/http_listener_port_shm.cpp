//
// http_listener 监听端口共享内存测试
// 验证 http_listener_port_shm.h 的写入/读取, 以及 http_listener 绑定后确实发布了真实端口
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_listener_port_shm.h>
#include <doodle_lib/core/http/http_route.h>

#include <boost/asio/io_context.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/test/unit_test.hpp>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>

namespace {
using namespace doodle::http;

/// 直接打开共享内存段并覆写内容, 用于构造脏数据
void corrupt_shm_magic(std::uint32_t in_magic) {
  boost::interprocess::shared_memory_object l_shm{
      boost::interprocess::open_only, std::string{g_listener_port_shm_name}.c_str(), boost::interprocess::read_write
  };
  boost::interprocess::mapped_region l_region{l_shm, boost::interprocess::read_write};
  http_listener_port_shm_t l_data{
      .magic_ = in_magic,
      .version_ = g_listener_port_shm_version,
      .port_    = 12345,
      .reserved_ = 0,
  };
  std::memcpy(l_region.get_address(), &l_data, sizeof(l_data));
}
}  // namespace

BOOST_AUTO_TEST_SUITE(http_listener_port_shm)

/// 写入后能原样读回
BOOST_AUTO_TEST_CASE(publish_then_read_round_trips) {
  BOOST_REQUIRE(publish_listener_port(50021));

  auto l_port = read_listener_port();
  BOOST_REQUIRE(l_port.has_value());
  BOOST_CHECK_EQUAL(*l_port, 50021);
}

/// 后写入者直接覆盖旧值 —— 对应"新启动进程覆盖旧的"这一需求
BOOST_AUTO_TEST_CASE(second_publish_overwrites_the_first) {
  BOOST_REQUIRE(publish_listener_port(50021));
  BOOST_REQUIRE(publish_listener_port(51234));

  auto l_port = read_listener_port();
  BOOST_REQUIRE(l_port.has_value());
  BOOST_CHECK_EQUAL(*l_port, 51234);
}

/// 端口边界值不被截断
BOOST_AUTO_TEST_CASE(port_boundaries_survive_round_trip) {
  BOOST_REQUIRE(publish_listener_port(1));
  auto l_port = read_listener_port();
  BOOST_REQUIRE(l_port.has_value());
  BOOST_CHECK_EQUAL(*l_port, 1);

  BOOST_REQUIRE(publish_listener_port(65535));
  l_port = read_listener_port();
  BOOST_REQUIRE(l_port.has_value());
  BOOST_CHECK_EQUAL(*l_port, 65535);
}

/// 魔数不对时读取失败, 且不抛异常
BOOST_AUTO_TEST_CASE(read_rejects_bad_magic) {
  BOOST_REQUIRE(publish_listener_port(50021));
  corrupt_shm_magic(0xDEADBEEFU);

  BOOST_CHECK(!read_listener_port().has_value());

  // 收尾: 恢复成合法内容, 避免影响后续用例
  BOOST_REQUIRE(publish_listener_port(50024));
}

/// 发布失败不能抛异常(这里覆盖正常路径的反面: 读一个合法段总是安全的)
BOOST_AUTO_TEST_CASE(publish_and_read_never_throw) {
  BOOST_CHECK_NO_THROW({
    publish_listener_port(50025);
    read_listener_port();
  });
}

/// 端到端: 监听器用端口 0 启动后, 共享内存里必须是内核分配的真实端口
/// 这正是 --local/--epiboly 不带 --port 时的路径, 也是本次改动的根因
///
/// 为什么必须带一个 app_base 夹具: g_ctx() 和 g_io_context() 解的都是 doodle_lib::impl::self,
/// 而 impl::self 只在 doodle_lib(即 app_base 的成员 lib_ptr) 构造时才赋值。本文件以前没构造
/// 过 app_base, 所以 run_http_listener 第一句 g_ctx().emplace<...>() 就是空指针写 ——
/// 崩在用例体之后, boost 只能报出最后一个检查点, 看起来像 publish 那一行出错。
/// 同一个坑在其他夹具里也有记录(见 ai_light_state.cpp:106-110)。
struct listener_fixture_t {
  ::doodle::app_base app_{};
};

BOOST_FIXTURE_TEST_CASE(listener_publishes_the_kernel_assigned_port, listener_fixture_t) {
  // 先塞一个假值, 确保下面读到的不是上一用例的残留
  BOOST_REQUIRE(publish_listener_port(1234));

  boost::asio::io_context l_io{};
  auto l_route = std::make_shared<http_route>();
  // port 0 => 由系统分配
  // g_io_context() 返回的正是监听器协程被 co_spawn 到的那个 context, 所以下面跑它。
  run_http_listener(l_io, l_route, 0);

  auto& l_app_io = ::doodle::g_io_context();
  l_app_io.restart();
  l_app_io.run_for(std::chrono::milliseconds{500});

  auto l_port = read_listener_port();
  BOOST_REQUIRE(l_port.has_value());
  BOOST_CHECK_NE(*l_port, 1234);
  // 系统分配的一定是有效非零端口
  BOOST_CHECK_GT(*l_port, 0);
}

BOOST_AUTO_TEST_SUITE_END()
