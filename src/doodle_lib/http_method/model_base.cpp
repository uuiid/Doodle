//
// Created by TD on 24-8-6.
//

#include "model_base.h"

#include "doodle_core/database_task/details/project_config.h"

#include <doodle_lib/core/http/http_function.h>
namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> get_asset_tree(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "not found");
}

// 获取模型库基本的上下文
boost::asio::awaitable<boost::beast::http::message_generator> model_base_get(session_data_ptr in_handle) {
  auto l_this_exe = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto&& l_prj = g_reg()->ctx().get<project_config::base_config>();
  nlohmann::json l_json;
  l_json["project"]        = g_reg()->ctx().get<project>();
  l_json["project_config"] = l_prj;
  l_json["user"]           = {};
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  co_return in_handle->make_msg(l_json.dump());
}

void model_base_reg(http_route& in_route) {
  in_route
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/model/asset_tree", get_asset_tree)
      )
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/model/ctx", model_base_get))

      ;
  // 无操作
}
}