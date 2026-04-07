//
// Created by TD on 2023/12/21.
//

#include "connect_video.h"

#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/user.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/progress_data.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/logger/logger.h>

#include <opencv2/core.hpp>
#include <opencv2/freetype.hpp>
#include <opencv2/opencv.hpp>

namespace doodle::detail {

void connect_video(
    const FSys::path& in_out_path, doodle::logger_ptr in_logger, const std::vector<FSys::path>& in_vector,
    const image_size& in_size, const uuid& in_task_info_id, const progress_data_ptr& in_progress_data
) {
  in_logger->log(log_loc(), level::info, "开始创建视频 {}", in_out_path);
  in_logger->log(log_loc(), level::info, "获得视屏路径 {}", in_vector);

  if (FSys::exists(in_out_path)) {
    FSys::remove(in_out_path);
  }

  const cv::Size k_size{in_size.width, in_size.height};
  auto video = cv::VideoWriter{in_out_path.generic_string(), cv::VideoWriter::fourcc('a', 'v', 'c', '1'), 25, k_size};
  // 设置质量为 100
  video.set(cv::VIDEOWRITER_PROP_QUALITY, 100);
  DOODLE_CHICK(video.isOpened(), "无法创建视频文件: {} ", in_out_path.generic_string());
  auto l_video_cap = cv::VideoCapture{};
  cv::Mat l_image{};
  std::double_t l_index{0};
  if (in_progress_data) in_progress_data->set_current_steps(static_cast<std::int32_t>(in_vector.size()));
  for (auto& l_video : in_vector) {
    in_logger->log(log_loc(), level::info, "开始读取视屏 {}", l_video);
    if (!in_task_info_id.is_nil())
      socket_io::broadcast(
          socket_io::preview_file_progress_update_broadcast_t{.preview_file_id_ = in_task_info_id, .progress_ = l_index}
      );
    if (in_progress_data) ++(*in_progress_data);

    l_video_cap.open(l_video.generic_string());
    if (!l_video_cap.isOpened()) {
      in_logger->log(log_loc(), level::warn, "视屏读取失败 跳过 {}", l_video);
      continue;
    }
    in_logger->log(log_loc(), level::info, "开始将{}写入行视屏", l_video);
    while (l_video_cap.read(l_image)) {
      if (l_image.empty()) {
        in_logger->log(log_loc(), level::warn, "视屏读取失败 跳过 {}", l_video);
        continue;
      }
      if (l_image.cols != k_size.width || l_image.rows != k_size.height) cv::resize(l_image, l_image, k_size);

      video << l_image;
    }
    ++l_index;
  }

  in_logger->log(log_loc(), level::info, "成功完成任务");
}
boost::asio::awaitable<void> connect_video_t::run() {
  file_list_ |=
      ranges::actions::sort([](const FSys::path& l_a, const FSys::path& l_b) { return l_a.stem() < l_b.stem(); });
  connect_video(out_path_, logger_ptr_, file_list_, image_size_, task_info_id_);
  co_return;
}

}  // namespace doodle::detail