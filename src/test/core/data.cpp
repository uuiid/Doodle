//
// Created by TD on 24-9-24.
//

#include "doodle_lib/core/global_function.h"
#include "doodle_lib/http_client/kitsu_client.h"
#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
BOOST_AUTO_TEST_SUITE(data)
BOOST_AUTO_TEST_CASE(http_client) {
  using namespace doodle;
  app_base l_app{};
  kitsu::kitsu_client l_client{"127.0.0.1:50025"};

  boost::asio::co_spawn(
      g_io_context(),
      [&]() -> boost::asio::awaitable<void> {
        auto l_uuid = from_uuid_str("96a1f1d5-e37d-4f22-90e0-1817468c9c3e");
        co_await l_client.upload_asset_file_maya(
            l_uuid, "D:\\test_db\\public\\DYX\\6_moxing\\CFX\\Ch014B_rig_jxh_cloth.ma"
        );
      },
      boost::asio::detached
  );
  l_app.run();
  ;
}

BOOST_AUTO_TEST_SUITE_END()