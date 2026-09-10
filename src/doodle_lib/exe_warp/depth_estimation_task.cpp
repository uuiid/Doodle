//
// Created by TD on 25-7-12.
//
// 深度估计分布式任务 — 在工作机上执行

#include "depth_estimation_task.h"

#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_lib/ai/depth_anything/doodle_depth_estimation.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/asio/consign.hpp>
#include <boost/asio/post.hpp>

#include "core/global_function.h"
#include <opencv2/opencv.hpp>

namespace doodle::http {

boost::asio::awaitable<void> depth_estimation_distributed::run() {
  auto l_logger       = create_logger();
  auto l_kitsu_client = create_kitsu_client();
  l_kitsu_client->set_logger(l_logger);

  // 从 command_ 反序列化参数
  task_info_.command_.at("preview_id").get_to(args_.preview_id_);

  SPDLOG_LOGGER_INFO(l_logger, "开始深度估计任务, preview_id: {}", args_.preview_id_);

  try {
    // 1. 下载输入视频
    auto l_input_path = co_await l_kitsu_client->download_depth_file(args_.preview_id_);
    SPDLOG_LOGGER_INFO(l_logger, "下载输入视频完成: {}", l_input_path);

    // 2. 加载模型并执行深度估计
    auto& l_ctx      = g_ctx().get<kitsu_ctx_t>();
    auto l_model_path = l_ctx.get_depth_model_path();
    ai::doodle_depth_estimation l_estimator{l_model_path};

    auto l_output_path = core_set::get_set().get_cache_root("depth_output") /
                         fmt::format("{}.mp4", args_.preview_id_);
    if (auto l_p = l_output_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);

    {
      auto l_capture = cv::VideoCapture{l_input_path.generic_string()};
      auto l_fps     = l_capture.get(cv::CAP_PROP_FPS);
      auto l_width   = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_WIDTH));
      auto l_height  = static_cast<int>(l_capture.get(cv::CAP_PROP_FRAME_HEIGHT));

      auto l_writer = cv::VideoWriter{
          l_output_path.generic_string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), l_fps,
          cv::Size{l_width, l_height}
      };

      cv::Mat l_frame, l_depth;
      while (l_capture.read(l_frame)) {
        l_depth = l_estimator.predict(l_frame);
        cv::normalize(l_depth, l_depth, 0, 255, cv::NORM_MINMAX, CV_8U);
        cv::cvtColor(l_depth, l_depth, cv::COLOR_GRAY2BGR);
        l_writer.write(l_depth);
      }
    }  // RAII: l_capture, l_writer 在此析构

    SPDLOG_LOGGER_INFO(l_logger, "深度估计完成, 输出: {}", l_output_path);

    // 3. 上传结果
    co_await l_kitsu_client->upload_depth_file(args_.preview_id_, l_output_path);
    SPDLOG_LOGGER_INFO(l_logger, "上传深度估计结果完成");

    // 4. 清理临时文件
    if (FSys::exists(l_input_path)) FSys::remove(l_input_path);
    if (FSys::exists(l_output_path)) FSys::remove(l_output_path);

  } catch (const std::exception& l_ex) {
    SPDLOG_LOGGER_ERROR(l_logger, "深度估计任务异常: {}", l_ex.what());
    // 异常时也尝试清理临时文件
    auto l_tmp_download = core_set::get_set().get_cache_root("depth_download") /
                          fmt::format("{}.mp4", args_.preview_id_);
    if (FSys::exists(l_tmp_download)) FSys::remove(l_tmp_download);
    auto l_tmp_output = core_set::get_set().get_cache_root("depth_output") /
                        fmt::format("{}.mp4", args_.preview_id_);
    if (FSys::exists(l_tmp_output)) FSys::remove(l_tmp_output);
  }

  co_return;
}

}  // namespace doodle::http