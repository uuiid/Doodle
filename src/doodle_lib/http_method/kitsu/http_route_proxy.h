//
// Created by TD on 24-10-10.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>

namespace doodle::http::kitsu {
class http_route_proxy : public http_route {
  std::vector<http_function_ptr> proxy_urls{};

 public:
  http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
  ) const override;
  http_route_proxy& reg_proxy(const http_function_ptr in_function);

  boost::asio::awaitable<tcp_stream_type_ptr> create_proxy() const override;
};
}  // namespace doodle::http::kitsu