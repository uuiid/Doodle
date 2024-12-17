//
// Created by TD on 2023/12/21.
//

#include "connect_video.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>

#include "opencv2/core.hpp"
#include <opencv2/freetype.hpp>
#include <opencv2/opencv.hpp>
namespace doodle::detail {

boost::system::error_code connect_video(
    const FSys::path &in_out_path, doodle::logger_ptr in_logger, const std::vector<FSys::path> &in_vector,
    const image_size &in_size
) {
  in_logger->log(log_loc(), level::info, "开始创建视频 {}", in_out_path);
  in_logger->log(log_loc(), level::info, "获得视屏路径 {}", in_vector);
  std::atomic_bool l_stop{};
  boost::system::error_code l_ec{};

  if (FSys::exists(in_out_path)) {
    FSys::remove(in_out_path, l_ec);
    if (l_ec) {
      l_ec.assign(error_enum::file_exists, doodle_category::get());
      in_logger->log(log_loc(), level::warn, "合成视频主动删除失败 {} ", in_out_path);
      BOOST_ASIO_ASSERT(l_ec);
      return l_ec;
    }
  }

  const cv::Size k_size{in_size.width, in_size.height};
  auto video = cv::VideoWriter{in_out_path.generic_string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 25, k_size};
  auto l_video_cap       = cv::VideoCapture{};
  const auto &k_size_len = in_vector.size();
  cv::Mat l_image{};
  for (auto &l_video : in_vector) {
    if (l_stop) {
      in_logger->log(log_loc(), level::warn, "连接视频被主动结束 合成视频文件将被主动删除");

      FSys::remove(in_out_path, l_ec);
      if (l_ec) {
        l_ec.assign(error_enum::file_exists, doodle_category::get());
        in_logger->log(log_loc(), level::warn, "合成视频主动删除失败 , 无法删除文件 {} ", in_out_path);
        BOOST_ASIO_ASSERT(l_ec);
      } else {
        l_ec.assign(boost::system::errc::operation_canceled, boost::system::generic_category());
        BOOST_ASIO_ASSERT(l_ec);
      }
      return l_ec;
    }

    in_logger->log(log_loc(), level::info, "开始读取视屏 {}", l_video);

    l_video_cap.open(l_video.generic_string());
    if (!l_video_cap.isOpened()) {
      in_logger->log(log_loc(), level::warn, "视屏读取失败 跳过 {}", l_video);
      continue;
    }
    auto l_video_count = l_video_cap.get(cv::CAP_PROP_FRAME_COUNT);
    in_logger->log(log_loc(), level::info, "开始将{}写入行视屏", l_video);
    while (l_video_cap.read(l_image)) {
      if (l_image.empty()) {
        in_logger->log(log_loc(), level::warn, "视屏读取失败 跳过 {}", l_video);
        continue;
      }
      if (l_image.cols != k_size.width || l_image.rows != k_size.height) cv::resize(l_image, l_image, k_size);

      in_logger->log(log_loc(), level::info, "progress 1/{}", l_video_count * k_size_len + k_size_len / 10);
      video << l_image;
    }
  }

  in_logger->log(log_loc(), level::info, "成功完成任务");
  return l_ec;
}

}  // namespace doodle::detail