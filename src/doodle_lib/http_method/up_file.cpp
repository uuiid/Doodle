//
// Created by TD on 25-1-11.
//

#include "up_file.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include "boost/beast/http/field.hpp"

namespace doodle::http {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> up_file_asset(session_data_ptr in_handle) {
  std::string l_task_id = in_handle->capture_->get("task_id");
  if (in_handle->content_type_ != detail::content_type::application_nuknown)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  if (in_handle->req_header_.count(boost::beast::http::field::content_disposition) == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "缺失必要的请求头信息");

  auto l_d = in_handle->req_header_[boost::beast::http::field::content_disposition];
  default_logger_raw()->info("{}", std::string{l_d});

  co_return in_handle->make_msg("{}");
}
}  // namespace
void up_file_reg(http_route& in_route) {
  in_route.reg(
      std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/doodle/data/asset/{task_id}/file/maya", up_file_asset
      )
  );
}
}  // namespace doodle::http