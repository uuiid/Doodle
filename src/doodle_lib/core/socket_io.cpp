//
// Created by TD on 25-1-23.
//

#include "socket_io.h"

#include <doodle_lib/core/engine_io.h>

namespace doodle::socket_io {

socket_io_packet socket_io_packet::parse(const std::string& in_str) {
  socket_io_packet l_packet{};
  std::size_t l_pos{};
  {
    auto l_type = in_str.front();
    if (l_type < 0 || l_type > 6)
      throw_exception(http_request_error{boost::beast::http::status::bad_request, "数据包格式错误"});
    l_packet.type_ = static_cast<socket_io_packet_type>(l_type);
  }

  if (l_packet.type_ == socket_io_packet_type::binary_ack || l_packet.type_ == socket_io_packet_type::binary_event) {
    std::size_t l_size{};
    while (in_str[++l_pos] != '-' && l_pos < in_str.size()) ++l_size;
    l_packet.binary_count_ = std::stoll(in_str.substr(1, l_size));
  }

  if (in_str[l_pos + 1] == '/') {
    auto l_begin = l_pos + 1;
    std::size_t l_size{};
    while (in_str[++l_pos] != ',' && l_pos < in_str.size()) ++l_size;
    l_packet.namespace_ = in_str.substr(l_begin, l_size);
  }
  if (auto l_id = in_str[l_pos + 1]; std::isdigit(l_id)) {
    auto l_begin = l_pos + 1;
    std::size_t l_size{};
    while (!std::isdigit(in_str[++l_pos]) && l_pos < in_str.size()) ++l_size;
    l_packet.id_ = std::stoll(in_str.substr(l_begin, l_size));
  }
  ++l_pos;
  l_packet.json_data_ = nlohmann::json::parse(in_str.begin() + l_pos, in_str.end());
}

boost::asio::awaitable<boost::beast::http::message_generator> socket_io_http::get_fun_impl(
    http::session_data_ptr in_handle
) const {
  auto l_p = parse_query_data(in_handle->url_);
  // 注册
  if (l_p.sid_.is_nil()) {
    auto l_hd             = g_ctx().get<sid_ctx>().handshake_data_;
    l_hd.sid_             = g_ctx().get<sid_ctx>().generate_sid();
    nlohmann::json l_json = l_hd;
    co_return in_handle->make_msg(dump_message(l_json.dump(), engine_io_packet_type::open));
  }

  // 心跳超时检查
  if (g_ctx().get<sid_ctx>().is_sid_timeout(l_p.sid_))
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "sid超时"});
  if (auto l_packet = event_->get_last_event(); l_packet) {
    // TODO: 序列化数据包
  } else
    co_return in_handle->make_msg(dump_message({}, engine_io_packet_type::ping));
}
boost::asio::awaitable<boost::beast::http::message_generator> socket_io_http::post_fun_impl(
    http::session_data_ptr in_handle
) const {
  auto l_p = parse_query_data(in_handle->url_);
  // 注册
  if (l_p.sid_.is_nil()) throw_exception(http_request_error{boost::beast::http::status::bad_request, "sid为空"});

  if (in_handle->content_type_ != http::detail::content_type::application_nuknown)  // TODO: 二进制数据, 未实现
    co_return in_handle->make_msg(std::string{"null"});
  auto l_body = std::get<std::string>(in_handle->body_);
  switch (auto l_engine_packet = parse_engine_packet(l_body); l_engine_packet) {
    case engine_io_packet_type::open:
      break;
    case engine_io_packet_type::ping:
      g_ctx().get<sid_ctx>().update_sid_time(l_p.sid_);
      break;
    case engine_io_packet_type::pong:
      co_return in_handle->make_msg(std::string{});
      break;
    case engine_io_packet_type::message:
      break;
    case engine_io_packet_type::close:
    case engine_io_packet_type::upgrade:
    case engine_io_packet_type::noop:
      co_return in_handle->make_msg(dump_message({}, engine_io_packet_type::noop));
      break;
  }
  l_body.erase(0, 1);
  auto l_pack = socket_io_packet::parse(l_body);
  event_->event(l_pack);
  co_return in_handle->make_msg("{}");
}

}  // namespace doodle::socket_io