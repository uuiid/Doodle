//
// Created by TD on 25-4-22.
//

#include "other.h"

#include <jwt-cpp/jwt.h>

namespace doodle::http::other {
boost::asio::awaitable<boost::beast::http::message_generator> ke_ling_au_get::callback(session_data_ptr in_handle) {
  const auto l_time = jwt::date::clock::now();
  auto l_jwt        = jwt::create()
                   .set_payload_claim("iss", jwt::claim{std::string{"access key"}})
                   .set_payload_claim("exp", jwt::claim{l_time + std::chrono::minutes{30}})
                   .set_payload_claim("nbf", jwt::claim{l_time - 5s})
                   .set_header_claim("alg", jwt::claim{std::string{"HS256"}})
                   .set_header_claim("typ", jwt::claim{std::string{"JWT"}})
                   .sign(jwt::algorithm::hs256{"secret key"});

  co_return in_handle->make_msg(nlohmann::json{{"access_token", l_jwt}}.dump());
}

}  // namespace doodle::http::other