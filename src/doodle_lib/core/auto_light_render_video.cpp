//
// Created by TD on 2024/2/2.
//

#include "auto_light_render_video.h"

#include <doodle_core/thread_pool/image_to_movie.h>
#include <doodle_core/thread_pool/process_message.h>

#include <doodle_lib/long_task/image_to_move.h>
namespace doodle {
void auto_light_render_video::init() {
  // 初始化
  // 1. 初始化日志
  data_->logger_ = msg_.get<process_message>().logger();
  // 2. 初始化合成视屏上下文
  if (!g_ctx().contains<image_to_move>()) {
    g_ctx().emplace<image_to_move>(std::make_shared<detail::image_to_move>());
  }
}

void auto_light_render_video::operator()(boost::system::error_code in_error_code, const FSys::path &in_vector) const {
  if (in_error_code) {
    data_->logger_->error("渲染视频失败:{}", in_error_code.message());
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  data_->logger_->info("开始合成视屏 :{}", in_vector.generic_string());
  // 输出路径直接是输入路径的父路径, 不包括文件名
  auto l_out_path                                                            = in_vector.parent_path();
  msg_.emplace_or_replace<image_to_move::element_type::out_file_path>().path = l_out_path;
  g_ctx().get<image_to_move>()->async_create_move(
      msg_, FSys::list_files(in_vector), boost::asio::bind_executor(g_io_context(), std::move(*this))
  );
}
void auto_light_render_video::operator()(const FSys::path &in_vector, boost::system::error_code in_error_code) const {
  if (in_error_code) {
    data_->logger_->error("合成视屏失败:{}", in_error_code.message());
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  data_->logger_->info("合成视屏完成 :{}", in_vector.generic_string());
  set_info_({in_vector});
  wait_op_->complete();
}
}  // namespace doodle