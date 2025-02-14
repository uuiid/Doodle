//
// Created by TD on 25-2-12.
//

#include "websocket_impl.h"

#include <doodle_core/core/co_queue.h>

#include <doodle_lib/core/engine_io.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/socket_io.h>
#include <doodle_lib/core/socket_io/websocket_impl.h>

#include <boost/asio/experimental/parallel_group.hpp>

#include "socket_io_core.h"
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
  sid_data_->upgrade_to_websocket();

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
    co_await parse_socket_io(l_socket_io);
  }
  socket_io_contexts_.clear();
}
boost::asio::awaitable<void> socket_io_websocket_core::parse_socket_io(socket_io_packet& in_body) {
  if (!sid_ctx_->has_register(in_body.namespace_)) {
    in_body.type_      = socket_io_packet_type::connect_error;
    in_body.json_data_ = nlohmann::json{{"message", "Invalid namespace"}};
    co_return co_await async_write_websocket(in_body.dump());
  }
  switch (in_body.type_) {
    case socket_io_packet_type::connect: {
      auto l_ptr = std::make_shared<socket_io_core>(sid_ctx_, in_body.namespace_, in_body.json_data_);
      socket_io_contexts_[in_body.namespace_].core_ = l_ptr;
      sid_ctx_->emit_connect(l_ptr);
      in_body.json_data_                                         = nlohmann::json{{"sid", l_ptr->get_sid()}};
      socket_io_contexts_[in_body.namespace_].scoped_connection_ = boost::signals2::scoped_connection{
          sid_ctx_->on(in_body.namespace_)
              ->on_emit(
                  std::bind_front(&socket_io_websocket_core::on_message, this)
              )  // 此处不使用共享指针, 避免造成循环引用
      };
      co_await async_write_websocket(in_body.dump());
      break;
    }
    case socket_io_packet_type::disconnect:
      if (socket_io_contexts_.contains(in_body.namespace_)) {
        in_body.type_ = socket_io_packet_type::event;
        auto l_ptr    = socket_io_contexts_[in_body.namespace_].core_;
        if (!in_body.namespace_.empty()) {
          /// 转移到主名称空间
          l_ptr->set_namespace({});
          // 转移到主名称空间
          socket_io_contexts_[in_body.namespace_].scoped_connection_ = boost::signals2::scoped_connection{
              sid_ctx_->on({})->on_emit(
                  std::bind_front(&socket_io_websocket_core::on_message, this)
              )  // 此处不使用共享指针, 避免造成循环引用
          };
        } else
          socket_io_contexts_.erase(in_body.namespace_);
      }
      break;
    case socket_io_packet_type::event:
      sid_ctx_->on(in_body.namespace_)->message(std::make_shared<socket_io_packet>(std::move(in_body)));
      break;
    case socket_io_packet_type::ack:
      break;
    case socket_io_packet_type::connect_error:
      break;
    case socket_io_packet_type::binary_event:
      break;
    case socket_io_packet_type::binary_ack:
      break;
  }
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
    case engine_io_packet_type::open:
    case engine_io_packet_type::noop:
      break;
  }
  co_return true;
}
void socket_io_websocket_core::on_message(const socket_io_packet_ptr& in_data) {
  if (web_stream_)
    boost::asio::co_spawn(
        web_stream_->get_executor(),
        [in_data, this]() -> boost::asio::awaitable<void> { co_await async_write_websocket(in_data->dump()); },
        boost::asio::consign(boost::asio::detached, shared_from_this())
    );
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
  auto [l_ec_w, l_tr_w] = co_await web_stream_->async_write(boost::asio::buffer(in_data));
  if (l_ec_w == boost::beast::websocket::error::closed || l_ec_w == boost::asio::error::operation_aborted) co_return;
  if (l_ec_w) logger_->error(l_ec_w.what()), co_await async_close_websocket();
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