//
// Created by TD on 25-7-1.
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

#include "kitsu.h"
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> auth_reset_password_post::callback(
    session_data_ptr in_handle
) {
  auto l_email = in_handle->get_json()["email"].get<std::string>();
  auto l_sql   = g_ctx().get<sqlite_database>();
  person l_person;
  try {
    l_person = l_sql.get_person_for_email(l_email);
  } catch (const doodle_error& e) {
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request,
            nlohmann::json{{"error", true}, {"message", "Email not listed in database."}}.dump()
        }
    );
  }

  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http