//
// Created by TD on 24-8-20.
//

#include "task.h"

#include <doodle_core/metadata/project.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {

namespace {

boost::asio::awaitable<boost::beast::http::message_generator> get_task_info(session_data_ptr in_handle) {
  boost::uuids::uuid l_task_id;
  try {
    l_task_id = boost::lexical_cast<boost::uuids::uuid>(in_handle->capture_->get("task_id"));
  } catch (...) {
    in_handle->logger_->warn("无效的任务id {}", boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }
  detail::http_client_data_base_ptr l_client_data{};
  if (!in_handle->user_data_.has_value()) {
    kitsu_data_t l_data{std::make_shared<detail::http_client_data_base>()};
    l_client_data = l_data.http_kitsu_;
    l_client_data->init(g_ctx().get<kitsu_ctx_t>().url_);
    in_handle->user_data_ = l_data;
  } else {
    l_client_data = std::any_cast<kitsu_data_t>(in_handle->user_data_).http_kitsu_;
  }

  co_return in_handle->make_msg(std::string{});
}

}  // namespace
void kitsu_task_reg(http_route& in_http_route) {
  // in_http_route
  //     // .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/task/{task_id}",
  //     // get_task_info))
  //     .reg(std::make_shared<http_function>(
  //         boost::beast::http::verb::get, "api/data/tasks/{task_id}/sy/working_file", sy_working_file
  //     ));
}
}  // namespace doodle::http::kitsu