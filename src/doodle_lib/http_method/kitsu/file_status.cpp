//
// Created by TD on 25-8-12.
//
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>


namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_file_status::get(session_data_ptr in_handle) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

}  // namespace doodle::http