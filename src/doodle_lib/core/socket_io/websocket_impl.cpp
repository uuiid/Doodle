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

packet_base_ptr socket_io_websocket_core::generate_register_reply() {
  auto l_hd = sid_ctx_->handshake_data_;
  sid_data_ = sid_ctx_->generate();
  l_hd.sid_ = sid_data_->get_sid();
  l_hd.upgrades_.clear();
  nlohmann::json l_json = l_hd;
  auto l_ptr            = std::make_shared<engine_io_packet>(engine_io_packet_type::open, l_json.dump());
  return l_ptr;
}

void socket_io_websocket_core::async_run() {
  boost::asio::co_spawn(g_io_context(), run(), [l_shared = shared_from_this()](std::exception_ptr in_eptr) {
    try {
      if (in_eptr) std::rethrow_exception(in_eptr);
    } catch (const std::exception& e) {
      l_shared->logger_->error(e.what());
    };
  });
}

boost::asio::awaitable<void> socket_io_websocket_core::init() {
  // 注册
  if (const auto l_p = parse_query_data(handle_->url_); l_p.sid_.is_nil())
    co_await async_write_websocket(generate_register_reply());
  else
    sid_data_ = sid_ctx_->get_sid(l_p.sid_);

  /// 查看是否有锁, 有锁直接返回
  if (sid_data_->is_locked()) co_return co_await async_close_websocket();
  sid_lock_ = sid_data_->get_lock();
  // boost::beast::flat_buffer l_buffer{};
  if (!web_stream_) throw_exception(std::runtime_error("web_stream_ is null"));

  {  // 第一次验证ping pong
    std::string l_body{};
    auto l_buffer = boost::asio::dynamic_buffer(l_body);
    co_await web_stream_->async_read(l_buffer);
    if (auto l_engine_packet = parse_engine_packet(l_body); l_engine_packet != engine_io_packet_type::ping)
      co_await async_close_websocket();
    auto l_ptr = std::make_shared<engine_io_packet>(engine_io_packet_type::pong, l_body.erase(0, 1));
    l_ptr->start_dump();
    co_await async_write_websocket(l_ptr);
  }

  sid_data_->upgrade_to_websocket();
  sid_data_->cancel_async_event();
  boost::asio::co_spawn(
      co_await boost::asio::this_coro::executor, async_write(),
      [l_shared = shared_from_this()](std::exception_ptr in_eptr) {
        try {
          if (in_eptr) std::rethrow_exception(in_eptr);
        } catch (const std::exception& e) {
          l_shared->logger_->error(e.what());
        };
      }
  );

  {  // 第二次验证 升级协议
    std::string l_body{};
    auto l_buffer = boost::asio::dynamic_buffer(l_body);
    co_await web_stream_->async_read(l_buffer);
    if (auto l_engine_packet = parse_engine_packet(l_body); l_engine_packet != engine_io_packet_type::upgrade)
      co_await async_close_websocket();
  }
}

boost::asio::awaitable<void> socket_io_websocket_core::run() {
  try {
    co_await init();
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      // boost::beast::flat_buffer l_buffer{};
      std::string l_body{};
      auto l_buffer = boost::asio::dynamic_buffer(l_body);
      if (!web_stream_) co_return;
      co_await web_stream_->async_read(l_buffer);
      if (auto [l_r, l_ptr] = sid_data_->handle_engine_io(l_body); l_r) continue;
      auto l_socket_io = socket_io_packet::parse(l_body);
      /// 解析二进制数据
      for (int i = 0; i < l_socket_io.binary_count_; ++i) {
        std::string l_body_{};
        auto l_buffer_ = boost::asio::dynamic_buffer(l_body_);
        if (!web_stream_) co_return;
        co_await web_stream_->async_read(l_buffer_);
        l_socket_io.binary_data_.emplace_back(l_body_);
      }
      sid_data_->handle_socket_io(l_socket_io);
    }
    socket_io_contexts_.clear();
  } catch (const std::exception& e) {
    default_logger_raw()->error("socket_io_websocket_core error {}", e.what());
  }
  co_await async_close_websocket();
}

boost::asio::awaitable<void> socket_io_websocket_core::async_write() {
  boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    co_await async_write_websocket(co_await sid_data_->channel_.async_receive(boost::asio::use_awaitable));
    if (sid_data_->is_timeout()) co_return co_await async_close_websocket();
  }
  co_return;
}

boost::asio::awaitable<void> socket_io_websocket_core::async_write_websocket(packet_base_ptr in_data) {
  if (!in_data) co_return;
  auto l_g = co_await write_queue_limitation_->queue(boost::asio::use_awaitable);
  if (!web_stream_) co_return;
  auto l_str = in_data->get_dump_data();
  // default_logger_raw()->error("async_write_websocket {}", l_str);
  co_await web_stream_->async_write(boost::asio::buffer(l_str));

  if (!in_data->get_binary_data().empty()) {
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
    for (auto& l_str : in_data->get_binary_data()) {
      co_await web_stream_->async_write(boost::asio::buffer(l_str));
    }
  }
}
boost::asio::awaitable<void> socket_io_websocket_core::async_close_websocket() {
  auto l_g = co_await write_queue_limitation_->queue(boost::asio::use_awaitable);
  if (!web_stream_) co_return;
  co_await web_stream_->async_close(boost::beast::websocket::close_code::normal);
  web_stream_.reset();
  socket_io_contexts_.clear();
}

}  // namespace doodle::socket_io