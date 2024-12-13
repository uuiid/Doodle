//
// Created by TD on 2022/10/8.
//

#include "doodle_core/core/app_base.h"
#include <doodle_core/doodle_core.h>

#include <doodle_lib/long_task/image_to_move.h>

#include <boost/test/unit_test.hpp>

#include <crtdbg.h>
#include <stdlib.h>
BOOST_AUTO_TEST_SUITE(move_test)
using namespace doodle;
BOOST_AUTO_TEST_CASE(create) {
  app_base l_app{};
  auto l_list     = FSys::list_files("E:/Doodle/build/EXR_24_12_13");
  auto l_out_file = "E:/Doodle/build/EXR_24_12_13.mp4";

  std::vector<movie::image_attr> l_image_list{};
  for (auto& it : l_list) {
    l_image_list.emplace_back(it).gamma_t = 0.9;
  }
  movie::image_attr::extract_num(l_image_list);

  detail::create_move(l_out_file, spdlog::default_logger(), l_image_list);
}

BOOST_AUTO_TEST_SUITE_END()