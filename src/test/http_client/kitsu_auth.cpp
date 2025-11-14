#include "doodle_core/core/core_set.h"
#include "doodle_core/core/global_function.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>

#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/beast/http/impl/error.ipp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

using namespace doodle;
BOOST_AUTO_TEST_SUITE(kitsu)

BOOST_AUTO_TEST_CASE(get_ue_plugin) {
  app_base l_app_base{};
  auto l_f = boost::asio::co_spawn(
      g_io_context(),
      []() -> boost::asio::awaitable<void> {
        auto l_c = std::make_shared<::doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
        auto l_r = co_await l_c->get_ue_plugin("5.5");
        BOOST_TEST_MESSAGE(l_r);
        BOOST_TEST(!l_r.empty());
        app_base::Get().on_cancel.emit();
      }(),
      boost::asio::use_future
  );
  l_app_base.run();
  l_f.get();
}

BOOST_AUTO_TEST_CASE(up_file) {
  app_base l_app_base{};
  auto l_f = boost::asio::co_spawn(
      g_io_context(),
      []() -> boost::asio::awaitable<void> {
        auto l_c = std::make_shared<::doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
        co_await l_c->upload_asset_file_maya(
            from_uuid_str("87ddbc98-61ef-4cf7-b23c-619c646685bc"), "D:/test_files/test_xgen/scenes/ce.ma"
        );
        app_base::Get().on_cancel.emit();
      }(),
      boost::asio::use_future
  );
  l_app_base.run();
  l_f.get();
}
BOOST_AUTO_TEST_CASE(up_file_ue) {
  app_base l_app_base{};
  auto l_f = boost::asio::co_spawn(
      g_io_context(),
      []() -> boost::asio::awaitable<void> {
        auto l_c = std::make_shared<::doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
        co_await l_c->upload_asset_file_ue(
            from_uuid_str("87ddbc98-61ef-4cf7-b23c-619c646685bc"),
            std::make_shared<std::vector<FSys::path>>(std::vector<FSys::path>{"E:/doodle_plug_dev_5/Content/test"})
        );
        app_base::Get().on_cancel.emit();
      }(),
      boost::asio::use_future
  );
  l_app_base.run();
  l_f.get();
}
BOOST_AUTO_TEST_CASE(create_comment_task) {
  app_base l_app_base{};
  auto l_f = boost::asio::co_spawn(
      g_io_context(),
      []() -> boost::asio::awaitable<void> {
        auto l_c = std::make_shared<::doodle::kitsu::kitsu_client>(core_set::get_set().server_ip);
        co_await l_c->comment_task(
            from_uuid_str("d7210472-de23-4b76-8332-3470e0442190"), "测试通过客户端创建评论",
            "D:/test_files/test_xgen/scenes/ce.ma"
        );
        app_base::Get().on_cancel.emit();
      }(),
      boost::asio::use_future
  );
  l_app_base.run();
  l_f.get();
}
BOOST_AUTO_TEST_SUITE_END()