//
// Created by TD on 24-10-10.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>

namespace doodle::http::kitsu {
class http_route_proxy : public http_route {
  std::vector<http_function_ptr> proxy_urls{};
  http_function_ptr get_file_, head_file_;

 public:
  http_route_proxy() {
    not_function     = nullptr;
    options_function = nullptr;
  }
  http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
  ) const override;
  http_route_proxy& reg_proxy(const http_function_ptr in_function);
  void reg_front_end(const http_function_ptr in_get_index, const http_function_ptr in_head_file);

  boost::asio::awaitable<tcp_stream_type_ptr> create_proxy() const override;
};
}  // namespace doodle::http::kitsu