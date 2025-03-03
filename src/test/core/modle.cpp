//
// Created by TD on 24-8-6.
//

#include <doodle_core/core/app_base.h>

#include <doodle_lib/core/alembic_file.h>
#include <doodle_lib/core/fbx_file.h>
#include <doodle_lib/core/http/http_listener.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
BOOST_AUTO_TEST_CASE(alembic_get_mats) {
  auto l_str = doodle::alembic::get_all_materials(
      R"(D:\test_files\test_ue_auto_main\test_auto_vfx\DBXY_EP360_SC003_AN\DBXY_EP360_SC003_AN_YeYuYJB_Rig_cyh_1001-1151.abc)"
  );
  BOOST_TEST(!l_str.empty());
  auto l_ss = fmt::format("{}", l_str);
  BOOST_TEST_MESSAGE(l_ss);
}
BOOST_AUTO_TEST_CASE(fbx_get_mats) {
  auto l_str = doodle::fbx::get_all_materials(
      R"(D:\test_files\test_ue_auto_main\test_auto_vfx\DBXY_EP360_SC003_AN\DBXY_EP360_SC003_AN_YeYuYJB_Rig_cyh_1001-1151.fbx)"
  );
  BOOST_TEST(!l_str.empty());
  auto l_ss = fmt::format("{}", l_str);
  BOOST_TEST_MESSAGE(l_ss);
}