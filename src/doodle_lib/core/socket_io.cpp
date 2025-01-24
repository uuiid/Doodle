//
// Created by TD on 25-1-23.
//

#include "socket_io.h"

#include <doodle_lib/core/engine_io.h>

namespace doodle::socket_io {

boost::asio::awaitable<boost::beast::http::message_generator> socket_io_http_get::operator()(
    http::session_data_ptr in_handle
) {
  auto l_p = parse_query_data(in_handle->url_);
  // 注册
  if (l_p.sid_.empty()) {
    auto l_hd             = g_ctx().get<sid_ctx>().handshake_data_;
    l_hd.sid_             = g_ctx().get<sid_ctx>().generate_sid();
    nlohmann::json l_json = l_hd;
    co_return in_handle->make_msg(dump_message(l_json.dump(), engine_io_packet_type::open));
  } else {  // 获取事件
  }
}

}  // namespace doodle::socket_io