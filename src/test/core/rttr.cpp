//
// Created by TD on 2022/8/26.
//

#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/assets_file.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include <memory>
#include <rttr/rttr_enable.h>
#include <rttr/type.h>

BOOST_AUTO_TEST_CASE(test_rttr_base) {
  for (const auto& i : rttr::type::get_types()) {
    BOOST_TEST_MESSAGE(i.get_name().to_string());
  }

  auto l_as_var = rttr::type::get_by_name("doodle::assets_file").create();

  BOOST_TEST_MESSAGE(l_as_var.get_type().get_name().to_string());

  if (l_as_var.is_type<doodle::assets_file>()) {
    auto l_ass = l_as_var.get_value<doodle::assets_file>();
    BOOST_TEST_MESSAGE("as obj");
    l_ass.name_attr("test");
    BOOST_TEST_MESSAGE(l_ass.str());
  }
  if (l_as_var.is_type<doodle::assets_file*>()) {
    auto l_ass = l_as_var.get_value<doodle::assets_file*>();
    BOOST_TEST_MESSAGE("as raw obj");
    l_ass->name_attr("test");
    BOOST_TEST_MESSAGE(l_ass->str());
  }
  if (l_as_var.is_type<std::shared_ptr<doodle::assets_file>>()) {
    auto l_ass = l_as_var.get_value<std::shared_ptr<doodle::assets_file>>();
    BOOST_TEST_MESSAGE("as std::shared_ptr");
    l_ass->name_attr("test");
    BOOST_TEST_MESSAGE(l_ass->str());
  }
  // // l_ass.name_attr("tret");
}