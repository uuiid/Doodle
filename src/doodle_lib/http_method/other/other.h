//
// Created by TD on 25-4-22.
//

#pragma once
#include <doodle_lib/http_method/http_jwt_fun.h>


namespace doodle::http::other {
// clang-format off
DOODLE_HTTP_FUN(key_ji_meng, get, "api/doodle/key/ji_meng", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
//
}
