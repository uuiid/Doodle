//
// Created by TD on 2022/8/26.
//

#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/assets_file.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <rttr/rttr_enable.h>
#include <rttr/type.h>

BOOST_AUTO_TEST_CASE(test_rttr_base) {
  for (const auto& i : rttr::type::get_types()) {
    DOODLE_LOG_INFO(i.get_name().to_string());
  }

  auto l_as_var = rttr::type::get_by_name("doodle::assets_file").create();

  DOODLE_LOG_INFO(l_as_var.get_type().get_name().to_string());

  if (l_as_var.is_type<doodle::assets_file>()) {
    auto l_ass = l_as_var.get_value<doodle::assets_file>();
    DOODLE_LOG_INFO("as obj");
    l_ass.name_attr("test");
    DOODLE_LOG_ERROR(l_ass.str());
  }
  if (l_as_var.is_type<doodle::assets_file*>()) {
    auto l_ass = l_as_var.get_value<doodle::assets_file*>();
    DOODLE_LOG_INFO("as raw obj");
    l_ass->name_attr("test");
    DOODLE_LOG_ERROR(l_ass->str());
  }
  if (l_as_var.is_type<std::shared_ptr<doodle::assets_file>>()) {
    auto l_ass = l_as_var.get_value<std::shared_ptr<doodle::assets_file>>();
    DOODLE_LOG_INFO("as std::shared_ptr");
    l_ass->name_attr("test");
    DOODLE_LOG_ERROR(l_ass->str());
  }
  // // l_ass.name_attr("tret");
}