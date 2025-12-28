//
// Created by TD on 2022/10/8.
//

#include "doodle_core/core/app_base.h"
#include <doodle_core/doodle_core.h>

#include <doodle_lib/core/ffmpeg_video.h>
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

  detail::create_move(l_out_file, spdlog::default_logger(), l_image_list, {1920, 1080});
}

BOOST_AUTO_TEST_CASE(conv_audio) {
  app_base l_app{};
  auto l_out_file = "D:/test_files/calp_test/v2_audio.wav.aac";
  auto l_audio    = "D:/test_files/calp_test/v1.wav";
  ffmpeg_video::preprocess_wav_to_aac(l_audio, l_out_file);
}

BOOST_AUTO_TEST_CASE(add_audio) {
  app_base l_app{};
  auto l_in_file  = "D:/test_files/calp_test/v2.mp4";
  auto l_out_file = "D:/test_files/calp_test/v2_audio.mp4";
  auto l_audio    = "D:/test_files/calp_test/v1.wav";
  ffmpeg_video l_video{l_in_file, l_out_file};
  l_video.set_audio(l_audio);
  l_video.process();
}

BOOST_AUTO_TEST_SUITE_END()