//
// Created by TD on 24-8-20.
//

#include "task.h"
namespace doodle::http {

namespace {
boost::asio::awaitable<boost::beast::http::message_generator> get_task_info(session_data_ptr in_handle) {
  boost::uuids::uuid l_task_id;
  try {
    l_task_id = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("task_id"));
  } catch (...) {
    in_handle->logger_->warn("无效的任务id {}", boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }



  co_return in_handle->make_msg(std::string{});
}
}  // namespace
void kitsu_task_reg(http_route& in_http_route) {
  in_http_route.reg(
      std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{task_id}", get_task_info)
  );
}
}  // namespace doodle::http