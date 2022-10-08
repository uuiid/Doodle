//
// Created by TD on 2022/9/29.
//
#include <main_fixtures/lib_fixtures.h>

#include <boost/test/unit_test.hpp>

#include <doodle_core/doodle_core.h>
#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_lib/app/doodle_main_app.h>
#include <doodle_lib/app/rpc_server_facet.h>
#include <doodle_lib/long_task/image_to_move.h>

namespace doodle {
class test_app2 : public doodle_main_app {
 public:
  test_app2() : doodle::doodle_main_app() {
    run_facet = std::make_shared<facet::rpc_server_facet>();
    add_facet(run_facet);
  }
};
}  // namespace doodle

struct loop_rpc {
  ::doodle::test_app2 main_app_attr{};
  void setup() {
    for (int l = 0; l < 500; ++l) {
      main_app_attr.poll_one();
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(rpc, loop_rpc)
BOOST_AUTO_TEST_CASE(base) {
  main_app_attr.run();
}

BOOST_AUTO_TEST_SUITE_END()
