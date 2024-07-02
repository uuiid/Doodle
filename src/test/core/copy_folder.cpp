#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/thread_copy_io.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

using namespace doodle;
BOOST_AUTO_TEST_CASE(copy_folder) {
  app_base l_app_base{};

  thread_copy_io_service l_copy{};

  l_copy.async_delete_remote_not_exit_and_copy_old(
      {{R"(D:\job)", R"(D:\job2)"}, {R"(D:\test_files)", R"(D:\job2)"}}, {"ModernCSV-Win-v2.0.7"},
      FSys::copy_options::recursive, spdlog::default_logger(),
      [](const boost::system::error_code& in_ec) {
        if (in_ec) {
          BOOST_TEST_MESSAGE(in_ec.message());
        }
      }
  );
  g_io_context().run();
}