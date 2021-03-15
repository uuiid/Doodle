#include <gtest/gtest.h>

#include <opencv2/video.hpp>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <fstream>
TEST(ffmpeg, imageSequeTovideo) {
  const std::filesystem::path path{R"(D:\tmp\tmp)"};
  std::fstream file{};
  int i = 0;

  auto video = cv::VideoWriter("D:/test.mp4", cv::VideoWriter::fourcc('D', 'I', 'V', 'X'), (double)25, cv::Size{1280, 720});
  cv::Mat image{};
  for (auto&& it_p : std::filesystem::directory_iterator(path)) {
    if (it_p.is_regular_file()) {
      image = cv::imread(it_p.path().string());
      video << image;
    }
  }
}