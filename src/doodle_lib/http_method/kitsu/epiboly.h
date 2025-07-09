//
// Created by TD on 24-12-13.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
// "/api/config"
DOODLE_HTTP_FUN(epiboly_config, get, ucom_t{} / "api" / "config", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "/api/auth/authenticated"
DOODLE_HTTP_FUN(epiboly_authenticated, get, ucom_t{} / "api" / "auth" / "authenticated", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "/api/data/user/context"
DOODLE_HTTP_FUN(epiboly_user_context, get, ucom_t{} / "api" / "data" / "user" / "context", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http