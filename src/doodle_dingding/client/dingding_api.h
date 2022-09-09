//
// Created by TD on 2022/9/9.
//
#pragma once

#include <doodle_dingding/client/client.h>

namespace doodle {
namespace dingding {

class DOODLE_DINGDING_API dingding_api : public client {
 private:
 public:
  constexpr static const std::string_view dingding_host{"https://oapi.dingtalk.com"};

  explicit dingding_api(
      const boost::asio::any_io_executor& in_executor,
      boost::asio::ssl::context& in_ssl_context
  );
  std::string gettoken();
};

}  // namespace dingding
}  // namespace doodle
