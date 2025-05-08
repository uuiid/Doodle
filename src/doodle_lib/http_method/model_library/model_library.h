//
// Created by TD on 25-5-8.
//

#pragma once
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http::model_library {

DOODLE_HTTP_FUN(assets, get, "api/doodle/model_library/assets", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets, post, "api/doodle/model_library/assets", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets_modify, post, "api/doodle/model_library/assets/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets, delete_, "api/doodle/model_library/assets/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets, patch, "api/doodle/model_library/assets", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

}  // namespace doodle::model_library
