//
// Created by TD on 25-2-12.
//

#include "websocket_impl.h"

#include <doodle_core/core/co_queue.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/socket_io/engine_io.h>
#include <doodle_lib/core/socket_io/sid_data.h>
#include <doodle_lib/core/socket_io/socket_io_core.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/core/socket_io/socket_io_packet.h>

#include <boost/asio/experimental/parallel_group.hpp>
namespace doodle::socket_io {

socket_io_websocket_core::socket_io_websocket_core(
    http::session_data_ptr in_handle, const std::shared_ptr<sid_ctx>& in_sid_ctx,
    boost::beast::websocket::stream<http::tcp_stream_type> in_stream
)
    : logger_(in_handle->logger_),
      web_stream_(std::make_shared<boost::beast::websocket::stream<http::tcp_stream_type>>(std::move(in_stream))),
      sid_ctx_(in_sid_ctx),
      write_queue_limitation_(std::make_shared<awaitable_queue_limitation>()),
      handle_(std::move(in_handle)) {}

std::string socket_io_websocket_core::generate_register_reply() {
  auto l_hd = sid_ctx_->handshake_data_;
  sid_data_ = sid_ctx_->generate();
  l_hd.sid_ = sid_data_->get_sid();
  l_hd.upgrades_.clear();
  nlohmann::json l_json = l_hd;
  return dump_message(l_json.dump(), engine_io_packet_type::open);
}
boost::asio::awaitable<void> socket_io_websocket_core::run() {
  // 注册
  if (const auto l_p = parse_query_data(handle_->url_); l_p.sid_.is_nil())
    co_await async_write_websocket(generate_register_reply());
  else
    sid_data_ = sid_ctx_->get_sid(l_p.sid_);
  sid_data_->set_websocket_connect(shared_from_this());

  /// 查看是否有锁, 有锁直接返回
  if (sid_data_->is_locked()) co_return co_await async_close_websocket();
  sid_lock_ = sid_data_->get_lock();

  boost::asio::co_spawn(
      co_await boost::asio::this_coro::executor, async_ping_pong(),
      boost::asio::consign(boost::asio::detached, shared_from_this())
  );

  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    // boost::beast::flat_buffer l_buffer{};
    std::string l_body{};
    auto l_buffer = boost::asio::dynamic_buffer(l_body);
    if (!web_stream_) co_return;
    auto [l_ec_r, l_tr_s] = co_await web_stream_->async_read(l_buffer);
    if (l_ec_r == boost::beast::websocket::error::closed) co_return;
    if (l_ec_r) co_return logger_->error(l_ec_r.what()), co_await async_close_websocket();

    if (co_await parse_engine_io(l_body)) continue;
    auto l_socket_io = socket_io_packet::parse(l_body);
    /// 解析二进制数据
    for (int i = 0; i < l_socket_io.binary_count_; ++i) {
      std::string l_body_{};
      auto l_buffer_ = boost::asio::dynamic_buffer(l_body_);
      if (!web_stream_) co_return;
      auto [l_ec_r, l_tr_s] = co_await web_stream_->async_read(l_buffer_);
      if (l_ec_r == boost::beast::websocket::error::closed) co_return;
      if (l_ec_r) co_return logger_->error(l_ec_r.what()), co_await async_close_websocket();
      l_socket_io.binary_data_.emplace_back(l_body);
    }
    if (auto l_str = sid_data_->parse_socket_io(l_socket_io); !l_str.empty()) co_await async_write_websocket(l_str);
  }
  socket_io_contexts_.clear();
}

boost::asio::awaitable<bool> socket_io_websocket_core::parse_engine_io(std::string& in_body) {
  switch (auto l_engine_packet = parse_engine_packet(in_body); l_engine_packet) {
    case engine_io_packet_type::ping:
      sid_data_->update_sid_time();
      in_body.erase(0, 1);
      co_await async_write_websocket(dump_message(in_body, engine_io_packet_type::pong));
      break;
    case engine_io_packet_type::pong:
      sid_data_->update_sid_time();

      break;
    case engine_io_packet_type::message:
      sid_data_->update_sid_time();
      in_body.erase(0, 1);
      co_return false;
      break;
    case engine_io_packet_type::close:
      co_await async_close_websocket();
      break;
    case engine_io_packet_type::upgrade:
      // 发出连接事件
      co_await async_write_websocket(sid_data_->connect_namespace(std::string{handle_->url_.segments().buffer()}));
    case engine_io_packet_type::open:
    case engine_io_packet_type::noop:
      break;
  }
  co_return true;
}

boost::asio::awaitable<void> socket_io_websocket_core::async_ping_pong() {
  boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    l_timer.expires_from_now(sid_ctx_->handshake_data_.ping_interval_);
    if (sid_data_->is_timeout()) co_return co_await async_close_websocket();
    co_await l_timer.async_wait(boost::asio::use_awaitable);
    co_await async_write_websocket(dump_message({}, engine_io_packet_type::ping));
  }
  co_return;
}
boost::asio::awaitable<void> socket_io_websocket_core::async_write_websocket(std::string in_data) {
  auto l_g = co_await write_queue_limitation_->queue(boost::asio::use_awaitable);
  if (!web_stream_) co_return;
  // web_stream_->binary(false);
  auto [l_ec_w, l_tr_w] = co_await web_stream_->async_write(boost::asio::buffer(in_data));
  if (l_ec_w == boost::beast::websocket::error::closed || l_ec_w == boost::asio::error::operation_aborted) co_return;
  if (l_ec_w) logger_->error(l_ec_w.what()), co_await async_close_websocket();
}
boost::asio::awaitable<void> socket_io_websocket_core::async_write_websocket(socket_io_packet_ptr in_data) {
  auto l_g = co_await write_queue_limitation_->queue(boost::asio::use_awaitable);
  if (!web_stream_) co_return;
  {
    // web_stream_->binary(false);
    auto l_str            = in_data->dump();
    auto [l_ec_w, l_tr_w] = co_await web_stream_->async_write(boost::asio::buffer(l_str));
    if (l_ec_w == boost::beast::websocket::error::closed || l_ec_w == boost::asio::error::operation_aborted) co_return;
    if (l_ec_w) co_return logger_->error(l_ec_w.what()), co_await async_close_websocket();
  }

  struct binary_data_guard {
    boost::beast::websocket::stream<http::tcp_stream_type>* web_stream_;
    explicit binary_data_guard(boost::beast::websocket::stream<http::tcp_stream_type>* in_web_stream)
        : web_stream_(std::move(in_web_stream)) {
      web_stream_->binary(true);
    }
    ~binary_data_guard() {
      if (web_stream_) web_stream_->binary(false);
    }
  };
  binary_data_guard l_binary_data_guard{web_stream_.get()};
  for (auto& l_str : in_data->binary_data_) {
    auto [l_ec_w, l_tr_w] = co_await web_stream_->async_write(boost::asio::buffer(l_str));
    if (l_ec_w == boost::beast::websocket::error::closed || l_ec_w == boost::asio::error::operation_aborted) co_return;
    if (l_ec_w) co_return logger_->error(l_ec_w.what()), co_await async_close_websocket();
  }
}
boost::asio::awaitable<void> socket_io_websocket_core::async_close_websocket() {
  auto l_g = co_await write_queue_limitation_->queue(boost::asio::use_awaitable);
  if (!web_stream_) co_return;
  auto [l_ec_close] = co_await web_stream_->async_close(boost::beast::websocket::close_code::normal);
  if (l_ec_close) logger_->error(l_ec_close.what());
  web_stream_.reset();
  socket_io_contexts_.clear();
}

}  // namespace doodle::socket_io