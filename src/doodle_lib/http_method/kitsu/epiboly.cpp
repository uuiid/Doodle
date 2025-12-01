//
// Created by TD on 24-12-13.
//

#include "epiboly.h"

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/kitsu.h>

namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> epiboly_user_context::get(session_data_ptr in_handle) {
  nlohmann::json l_json{};
  l_json["projects"] = g_ctx().get<sqlite_database>().get_all<project>();
  co_return in_handle->make_msg(l_json.dump());
}

}  // namespace doodle::http