//
// Created by TD on 24-9-24.
//
#include "doodle_lib/core/http/http_function.h"
#include "doodle_lib/http_method/model_library/model_library.h"
#include <doodle_lib/core/scan_win_service.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
BOOST_AUTO_TEST_SUITE(data)
using namespace doodle;
BOOST_AUTO_TEST_CASE(test_url_capture) {
  http::url_route_component_t l_route;
  struct data_t {
    uuid id;
    uuid id2;
    chrono::year_month ym;
    chrono::year_month_day ymd;
  };
  l_route / "api" / http::url_route_component_t::make_cap(http::url_route_component_t::g_uuid_regex, &data_t::id) /
      http::url_route_component_t::make_cap(http::url_route_component_t::g_uuid_regex, &data_t::id2) /
      http::url_route_component_t::make_cap(http::url_route_component_t::g_year_month_regex, &data_t::ym) /
      http::url_route_component_t::make_cap(http::url_route_component_t::g_year_month_day_regex, &data_t::ymd);
  std::vector<std::string> l_path = {
      "api", "12345678-1234-1234-1234-123456789012", "12345678-1234-1234-1234-123456789012", "2020-01", "2020-01-01"
  };
  auto l_ptr = std::make_shared<data_t>();
  for (const auto& [l_str, l_item] : ranges::zip_view(l_path, l_route.component_vector())) {
    BOOST_TEST(l_item->match(l_str));
    BOOST_TEST(l_item->set(l_str, l_ptr));
  }
}

BOOST_AUTO_TEST_CASE(test_check) { auto l_func = std::make_shared<http::model_library::assets_put>(); }

BOOST_AUTO_TEST_SUITE_END()