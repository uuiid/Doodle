//
// Created by TD on 24-8-21.
//

#include "user.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
namespace {

boost::asio::awaitable<boost::beast::http::message_generator> user_context(session_data_ptr in_handle) {}
}  // namespace
void user_reg(http_route& in_http_route) {}
}  // namespace doodle::http::kitsu