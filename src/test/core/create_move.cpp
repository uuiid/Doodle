//
// Created by TD on 2022/10/8.
//

#include "doodle_core/core/app_base.h"
#include "doodle_core/metadata/image_size.h"
#include <doodle_core/doodle_core.h>

#include <doodle_lib/core/ffmpeg_video.h>
#include <doodle_lib/core/generate_text_video.hpp>
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

  detail::create_move(l_out_file, spdlog::default_logger(), l_image_list, image_size{1920, 1080});
}

BOOST_AUTO_TEST_CASE(add_audio) {
  app_base l_app{};
  auto l_in_file  = "D:/test_files/calp_test/v2.mp4";
  auto l_out_file = "D:/test_files/calp_test/v2_out.mp4";
  auto l_audio    = "D:/test_files/calp_test/v2.wav";
  auto l_subtitle = "D:/test_files/calp_test/v2.srt";
  auto l_intro    = "D:/test_files/calp_test/intro.mp4";
  auto l_outro    = "D:/test_files/calp_test/outro.mp4";

  ffmpeg_video l_video{l_in_file, l_out_file};
  l_video.set_audio(l_audio);
  l_video.set_subtitle(l_subtitle);
  l_video.set_input_video(l_intro);
  l_video.set_outro(l_outro);
  l_video.set_time_code(true);
  l_video.set_watermark("测试水印");
  l_video.process();
}

BOOST_AUTO_TEST_CASE(text_video) {
  app_base l_app{};
  auto l_out_file = "D:/test_files/calp_test/v2_text.mp4";
  generate_text_video l_video{};
  l_video.add_font_attr(
      generate_text_video::font_attr_t{
          .font_path_   = "D:/test/No.21-上首传奇书法体.ttf",
          .font_height_ = 80,
          .font_point_  = cv::Point{0, 395},
          .text_        = "第一百二十三集"
      }
  );
  l_video.add_font_attr(
      generate_text_video::font_attr_t{
          .font_path_   = "D:/test/No.21-上首传奇书法体.ttf",
          .font_height_ = 110,
          .font_point_  = cv::Point{0, 395 + 80 + 100},
          .text_        = "风起云涌"
      }
  );
  l_video.set_out_path(l_out_file);
  l_video.run();
}

BOOST_AUTO_TEST_SUITE_END()