//
// Created by TD on 24-8-21.
//

#include "user.h"

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> user_context(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);

  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  auto [l_ec, l_res]  = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  auto&& l_kitsu_data = std::any_cast<kitsu_data_t&>(in_handle->user_data_);
  try {
    if (l_res.result() == boost::beast::http::status::ok) {
      auto l_json = nlohmann::json::parse(l_res.body());
      for (auto&& l_task : l_json["task_types"]) {
        l_kitsu_data.task_types_[boost::lexical_cast<boost::uuids::uuid>(l_task["id"].get<std::string>())] =
            l_task["name"].get<std::string>();
      }
    }

  } catch (...) {
    in_handle->logger_->warn("user_context error: {}", boost::current_exception_diagnostic_information());
  }

  DOODLE_TO_MAIN_THREAD();
  l_kitsu_data.task_types_.merge(g_ctx().get<kitsu_ctx_t>().task_types_);
  g_ctx().get<kitsu_ctx_t>().task_types_ = l_kitsu_data.task_types_;
  DOODLE_TO_SELF();

  co_return std::move(l_res);
}
}  // namespace
void user_reg(http_route& in_http_route) {
  in_http_route.reg(
      std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/user/context", user_context)
  );
}
}  // namespace doodle::http::kitsu