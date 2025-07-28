//
// Created by TD on 25-1-23.
//

#include "socket_io.h"

#include <doodle_core/core/co_queue.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/socket_io/engine_io.h>
#include <doodle_lib/core/socket_io/sid_data.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>
#include <doodle_lib/core/socket_io/websocket_impl.h>

#include <boost/asio/experimental/parallel_group.hpp>
namespace doodle::socket_io {

void socket_io_http_get::init() { g_ctx().emplace<sid_ctx&>(*sid_ctx_); }

std::string socket_io_http_get::generate_register_reply() {
  auto l_hd             = sid_ctx_->handshake_data_;
  auto l_sid_data       = sid_ctx_->generate();
  l_hd.sid_             = l_sid_data->get_sid();
  nlohmann::json l_json = l_hd;
  return dump_message(l_json.dump(), engine_io_packet_type::open);
}
boost::asio::awaitable<boost::beast::http::message_generator> socket_io_http_get::callback(
    http::session_data_ptr in_handle
) {
  auto l_p = parse_query_data(in_handle->url_);
  // 注册
  if (l_p.sid_.is_nil()) co_return in_handle->make_msg(generate_register_reply());
  auto l_sid_data = sid_ctx_->get_sid(l_p.sid_);

  // 心跳超时检查 或者已经进行了协议升级, 直接返回错误
  if (!l_sid_data || l_sid_data->is_timeout() || l_sid_data->is_upgrade_to_websocket())
    throw_exception(
        http_request_error{boost::beast::http::status::bad_request, "sid超时, 或者已经进行了协议升级, 或者已经关闭"}
    );
  auto l_event = co_await l_sid_data->async_event();
  // default_logger_raw()->info("sid {} 接收到事件 {}", l_p.sid_, l_event);
  auto l_str   = l_event ? l_event->get_dump_data() : std::string{};
  co_return in_handle->make_msg(std::move(l_str), "text/plain; charset=UTF-8", boost::beast::http::status::ok);
}
bool socket_io_http_get::has_websocket() const { return true; }

boost::asio::awaitable<void> socket_io_http_get::websocket_callback(
    boost::beast::websocket::stream<http::tcp_stream_type> in_stream, http::session_data_ptr in_handle
) {
  auto l_websocket = std::make_shared<socket_io_websocket_core>(in_handle, sid_ctx_, std::move(in_stream));
  l_websocket->async_run();
  co_return;
}
boost::asio::awaitable<boost::beast::http::message_generator> socket_io_http_post::callback(
    http::session_data_ptr in_handle
) {
  auto l_p = parse_query_data(in_handle->url_);
  // 注册
  if (l_p.sid_.is_nil()) throw_exception(http_request_error{boost::beast::http::status::bad_request, "sid为空"});

  if (in_handle->content_type_ != http::detail::content_type::text_plain)  // TODO: 二进制数据, 未实现
    co_return in_handle->make_msg("OK", "text/plain; charset=UTF-8", boost::beast::http::status::ok);

  auto l_body     = std::get<std::string>(in_handle->body_);
  auto l_sid_data = sid_ctx_->get_sid(l_p.sid_);
  if (!l_sid_data)
    throw_exception(
        http_request_error{boost::beast::http::status::bad_request, "sid超时, 或者已经进行了协议升级, 或者已经关闭"}
    );
  if (auto [l_r, l_ptr] = l_sid_data->handle_engine_io(l_body); !l_r) {
    // 继续处理 socket io 包
    auto l_packet = socket_io_packet::parse(l_body);
    l_sid_data->handle_socket_io(l_packet);
  };
  co_return in_handle->make_msg("OK", "text/plain; charset=UTF-8", boost::beast::http::status::ok);
}

boost::asio::awaitable<boost::beast::http::message_generator> socket_io_http_put::callback(
    http::session_data_ptr in_handle
) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "不支持put请求");
}

}  // namespace doodle::socket_io