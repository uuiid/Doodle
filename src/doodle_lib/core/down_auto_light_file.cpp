//
// Created by TD on 2024/1/5.
//

#include "down_auto_light_file.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/thread_copy_io.h>
namespace doodle {

void down_auto_light_anim_file::init() {
  data_->logger_ = msg_.get<process_message>().logger();

  if (!g_ctx().contains<thread_copy_io_service>()) g_ctx().emplace<thread_copy_io_service>();
}

void down_auto_light_anim_file::operator()(boost::system::error_code in_error_code) const {
  if (!data_->logger_) {
    default_logger_raw()->log(log_loc(), level::level_enum::err, "缺失组建错误 缺失日志组件");
    in_error_code.assign(error_enum::component_missing_error, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  if (in_error_code) {
    data_->logger_->log(log_loc(), level::level_enum::err, "maya_to_exe_file error:{}", in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }

  switch (data_->status_) {
    case status::begin:

      break;

    case status::end:
      break;
  }
}

}  // namespace doodle