//
// Created by TD on 25-4-1.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
DOODLE_HTTP_FUN(user_context, get, "api/data/user/context", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(auth_login, post, "api/auth/login", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(authenticated, get, "api/auth/authenticated", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(organisations, get, "api/data/organisations", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(with_tasks, get, "api/data/assets/with-tasks", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(shared_used, get, "api/data/projects/{project_id}/assets/shared-used", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(data_user_tasks, get, "api/data/user/tasks", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(config, get, "api/config", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http