//
// Created by TD on 2022/8/26.
//

#include "doodle_core/logger/logger.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/test/unit_test.hpp>

#include <rttr/rttr_enable.h>
#include <rttr/type.h>

BOOST_AUTO_TEST_CASE(test_rttr_base) {
  for (const auto& i : rttr::type::get_types()) {
    DOODLE_LOG_INFO(i.get_name().to_string());
  }
}