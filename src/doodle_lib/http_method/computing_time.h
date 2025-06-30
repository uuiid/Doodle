#pragma once

#include "doodle_core/metadata/user.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
boost::asio::awaitable<void> recomputing_time(
    const uuid& in_person_id, const chrono::year_month& in_year_month
);
DOODLE_HTTP_FUN(computing_time, post, "/api/doodle/computing_time/{user_id}/{year_month}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(computing_time_add, post, "/api/doodle/computing_time/{user_id}/{year_month}/add", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(computing_time_custom, post, "/api/doodle/computing_time/{user_id}/{year_month}/custom", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(computing_time_sort, post, "/api/doodle/computing_time/{user_id}/{year_month}/sort", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(computing_time_average, post, "/api/doodle/computing_time/{user_id}/{year_month}/average", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(computing_time, get, "/api/doodle/computing_time/{user_id}/{year_month}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(computing_time, patch, "/api/doodle/computing_time/{user_id}/{year_month}/{task_id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(computing_time, delete_, "/api/doodle/computing_time/{computing_time_id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()



}  // namespace doodle::http