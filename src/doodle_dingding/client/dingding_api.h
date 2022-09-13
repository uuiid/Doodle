//
// Created by TD on 2022/9/9.
//
#pragma once

#include <doodle_dingding/client/client.h>
#include <nlohmann/json_fwd.hpp>

#include <doodle_dingding/metadata/access_token.h>

namespace doodle::dingding {
class access_token;

class DOODLE_DINGDING_API dingding_api : public client {
 private:
 public:
  constexpr static const std::string_view dingding_host{"https://oapi.dingtalk.com"};

  explicit dingding_api(
      const boost::asio::any_io_executor& in_executor,
      boost::asio::ssl::context& in_ssl_context
  );
  void async_get_token(
      read_access_token_fun&& in
  );
};

}  // namespace doodle
