
#include "doodle_lib/core/scan_assets/character_scan_category.h"
#include <doodle_lib/core/scan_assets/scene_scan_category.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
BOOST_AUTO_TEST_SUITE(scan_category)

BOOST_AUTO_TEST_CASE(ZM_scene) {
  doodle::details::scene_scan_category_t l_s{};
  l_s.logger_ = spdlog::default_logger();
  // l_s.scan(doodle::details::scan_category_data_t::project_root_t{
  //     "宗门里除了我都是卧底", R"(//192.168.10.240/public/ZMLCLWDSWD)", "ZMLCLWDSWD", "ZM", R"(C:\sy\ZMLCLWDSWD)", ""
  // });
}
BOOST_AUTO_TEST_CASE(ZM_character) {
  doodle::details::character_scan_category_t l_s{};
  l_s.logger_ = spdlog::default_logger();
  // l_s.scan(doodle::details::scan_category_data_t::project_root_t{
  //     "宗门里除了我都是卧底", R"(//192.168.10.240/public/ZMLCLWDSWD)", "ZMLCLWDSWD", "ZM", R"(C:\sy\ZMLCLWDSWD)", ""
  // });
}
BOOST_AUTO_TEST_SUITE_END()