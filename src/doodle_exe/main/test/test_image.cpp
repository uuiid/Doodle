//
// Created by TD on 2021/12/28.
//

#include <catch2/catch.hpp>
#include <doodle_lib/doodle_lib_all.h>

using namespace doodle;
TEST_CASE("image to vide", "[core]") {
  FSys::path k_image_path{R"(E:\tmp\image_test_ep002_sc002)"};

  std::vector<entt::handle> l_vector{};
  for (auto& k_path : FSys::list_files(k_image_path)) {
    auto l_h      = make_handle();
    auto& k_image = l_h.emplace<image_file_attribute>(k_path);
    k_image.watermarks.push_back(image_watermark{"test"s, 0.5, 0.5, {25, 220, 2, 0}});
    l_vector.push_back(l_h);
  }
  auto l_msg = make_handle();
  l_msg.emplace<process_message>();
  l_msg.emplace<episodes>().analysis(k_image_path);
  l_msg.emplace<shot>().analysis(k_image_path);
  l_msg.emplace<FSys::path>(R"(E:\tmp\)");

  g_main_loop().attach<details::image_to_move>(l_msg, l_vector);
  while (!g_main_loop().empty())
    g_main_loop().update({}, nullptr);
}
