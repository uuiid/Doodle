//
// Created by TD on 25-7-2.
//
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/metadata/rules.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/time_tool/work_clock.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/http_method/seed_email.h>

#include <boost/system/detail/error_code.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

BOOST_AUTO_TEST_SUITE(email)

BOOST_AUTO_TEST_CASE(seed) {
  doodle::email::seed_email l_seed_email{"smtp.163.com", 25, "19975298467@163.com", "WIGOMKIGPOKFJKEB"};
  l_seed_email("19975298467@163.com", "957714080@qq.com", "test");
}

BOOST_AUTO_TEST_SUITE_END()