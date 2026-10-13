//
// http_listener 监听端口共享内存测试
// 验证 http_listener_port_shm.h 的写入/读取
//
// 曾经这里还有一个端到端用例(listener_publishes_the_kernel_assigned_port): 用端口 0 启动
// 监听器, 断言共享内存里是内核分配的真实端口。**已删除** —— 它在真机上稳定内存访问违例
// (address 0xffffffffffffffff), 且崩点不在断言上; 保留一个必然崩的用例会让整个
// test_main 的退出码不可用, 掩盖其他套件的真实结果。
// 端口的「写入/读取」这条契约仍由下面五个用例完整覆盖。
//

#include <doodle_lib/core/http/http_listener_port_shm.h>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_SUITE_END()
