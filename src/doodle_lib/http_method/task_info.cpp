//
// Created by TD on 2024/2/27.
//

#include "task_info.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
namespace doodle::http {
void task_info::post_task(boost::system::error_code in_error_code, entt::handle in_handle) {}

void task_info::get_task(boost::system::error_code in_error_code, entt::handle in_handle) {}
void task_info::list_task(boost::system::error_code in_error_code, entt::handle in_handle) {}

void task_info::reg(doodle::http::http_route &in_route) {
  in_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task",
          session::make_http_reg_fun<false>(boost::asio::bind_executor(g_io_context(), &task_info::list_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task/{id}",
          session::make_http_reg_fun<false>(boost::asio::bind_executor(g_io_context(), &task_info::get_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "v1/task",
          session::make_http_reg_fun<basic_json_body>(boost::asio::bind_executor(g_io_context(), &task_info::post_task))
      ));
}

}  // namespace doodle::http
