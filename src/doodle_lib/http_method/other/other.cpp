//
// Created by TD on 25-4-22.
//

#include "other.h"

#include <doodle_lib/http_method/kitsu.h>

#include <jwt-cpp/jwt.h>


namespace doodle::http::other {

boost::asio::awaitable<boost::beast::http::message_generator> key_ji_meng::get(session_data_ptr in_handle) {
  auto& l_ctx = g_ctx().get<kitsu_ctx_t>();
  co_return in_handle->make_msg(
      nlohmann::json{
          {"access_key_id", l_ctx.ji_meng_access_key_id_}, {"secret_access_key", l_ctx.ji_meng_secret_access_key_}
      }
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> deepseek_key::get(session_data_ptr in_handle) {
  auto l_list = g_ctx().get<kitsu_ctx_t>().deepseek_keys_;
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
}  // namespace doodle::http::other