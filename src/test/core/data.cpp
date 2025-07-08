//
// Created by TD on 24-9-24.
//
#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/core/scan_win_service.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
BOOST_AUTO_TEST_SUITE(data)
using namespace doodle;
BOOST_AUTO_TEST_CASE(test_url_capture) {
  http::url_route_t l_route;
  struct data_t {
    uuid id;
    uuid id2;
    chrono::year_month ym;
    chrono::year_month_day ymd;
  };
  l_route / "api" / http::url_route_t::make_component(http::url_route_t::g_uuid_regex, &data_t::id) /
      http::url_route_t::make_component(http::url_route_t::g_uuid_regex, &data_t::id2) /
      http::url_route_t::make_component(http::url_route_t::g_year_month_regex, &data_t::ym) /
      http::url_route_t::make_component(http::url_route_t::g_year_month_day_regex, &data_t::ymd);

}

BOOST_AUTO_TEST_SUITE_END()