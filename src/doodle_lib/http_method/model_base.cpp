//
// Created by TD on 24-8-6.
//

#include "model_base.h"

#include <doodle_lib/core/http/http_function.h>
namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> get_asset_tree(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "not found");
}
void model_base_reg(http_route& in_route) {
  in_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/asset_tree", get_asset_tree))

      ;
  // 无操作
}
}