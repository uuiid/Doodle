//
// Created by TD on 25-6-26.
//
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_project_schedule_items_get::callback(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg(nlohmann::json::array());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_project_milestones_get::callback(
    session_data_ptr in_handle
) {
  co_return in_handle->make_msg(nlohmann::json::array());

}


}