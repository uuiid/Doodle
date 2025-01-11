//
// Created by TD on 25-1-11.
//

#include "up_file.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> up_file_asset(session_data_ptr in_handle) {
  std::string l_task_id = in_handle->capture_->get("task_id");
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