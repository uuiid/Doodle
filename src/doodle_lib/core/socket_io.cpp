//
// Created by TD on 25-1-23.
//

#include "socket_io.h"

#include <doodle_core/core/co_queue.h>

#include <doodle_lib/core/engine_io.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>

#include <boost/asio/experimental/parallel_group.hpp>
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
  return l_packet;
}

void event_base::event(const socket_io_packet& in_packet) { event(std::string{}); }

std::string event_t::get_last_event() const {
  auto l_ptr = event_.load(std::memory_order_acquire);
  return l_ptr ? *l_ptr : std::string{};
}
void event_t::event(std::string in_packet) {
  event_signal_(in_packet);
  event_ = std::make_shared<std::string>(in_packet);
}

class socket_io_http_base_fun : public ::doodle::http::http_function {
 protected:
  std::shared_ptr<event_base> event_;
  std::string url_;

 public:
  explicit socket_io_http_base_fun(
      boost::beast::http::verb in_verb, std::string in_url, const std::shared_ptr<event_base>& in_event
  )
      : http_function(in_verb, {}), event_(std::move(in_event)), url_(std::move(in_url)) {}

  std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const override {
    if (in_segments_ref.buffer() == url_) return {true, {}};
    return {false, {}};
  }
};

socket_io_websocket_core::socket_io_websocket_core(
    http::session_data_ptr in_handle, boost::beast::websocket::stream<http::tcp_stream_type> in_stream
)
    : logger_(in_handle->logger_),
      web_stream_(std::make_shared<boost::beast::websocket::stream<http::tcp_stream_type>>(std::move(in_stream))),
      write_queue_limitation_(std::make_shared<awaitable_queue_limitation>()),
      handle_(std::move(in_handle)) {}

std::string socket_io_websocket_core::generate_register_reply() {
  auto l_hd = g_ctx().get<sid_ctx>().handshake_data_;
  l_hd.sid_ = g_ctx().get<sid_ctx>().generate_sid();
  sid_      = l_hd.sid_;
  l_hd.upgrades_.clear();
  nlohmann::json l_json = l_hd;
  return dump_message(l_json.dump(), engine_io_packet_type::open);
}
boost::asio::awaitable<void> socket_io_websocket_core::run() {
  // 注册
  if (const auto l_p = parse_query_data(handle_->url_); l_p.sid_.is_nil())
    co_await async_write_websocket(generate_register_reply());
  else
    sid_ = l_p.sid_;
  g_ctx().get<sid_ctx>().set_upgrade_to_websocket(sid_);

  /// 查看是否有锁, 有锁直接返回
  if (g_ctx().get<sid_ctx>().has_sid_lock(sid_)) co_return co_await async_close_websocket();

  sid_lock_ = g_ctx().get<sid_ctx>().get_sid_lock(sid_);
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

    switch (auto l_engine_packet = parse_engine_packet(l_body); l_engine_packet) {
      case engine_io_packet_type::open:
        break;
      case engine_io_packet_type::ping:
        g_ctx().get<sid_ctx>().update_sid_time(sid_);
        l_body.erase(0, 1);
        co_await async_write_websocket(dump_message(l_body, engine_io_packet_type::pong));
        break;
      case engine_io_packet_type::pong:
        g_ctx().get<sid_ctx>().update_sid_time(sid_);
        break;
      case engine_io_packet_type::message:
        break;
      case engine_io_packet_type::close:
        co_return co_await async_close_websocket();

      case engine_io_packet_type::upgrade:
      case engine_io_packet_type::noop:
        break;
    }
  }
}
boost::asio::awaitable<void> socket_io_websocket_core::async_ping_pong() {
  boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor};
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    l_timer.expires_from_now(g_ctx().get<sid_ctx>().handshake_data_.ping_interval_);
    if (g_ctx().get<sid_ctx>().is_sid_timeout(sid_)) co_return co_await async_close_websocket();

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
}

class socket_io_http_get : public socket_io_http_base_fun {
  // 生成注册回复
  std::string generate_register_reply() {
    auto l_hd             = g_ctx().get<sid_ctx>().handshake_data_;
    l_hd.sid_             = g_ctx().get<sid_ctx>().generate_sid();
    nlohmann::json l_json = l_hd;
    return dump_message(l_json.dump(), engine_io_packet_type::open);
  }

 public:
  socket_io_http_get(const std::string& in_path, const std::shared_ptr<event_base>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::get, in_path, in_event) {}

  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override {
    auto l_p = parse_query_data(in_handle->url_);
    // 注册
    if (l_p.sid_.is_nil()) co_return in_handle->make_msg(generate_register_reply());

    // 心跳超时检查 或者已经进行了协议升级, 直接返回错误
    if (g_ctx().get<sid_ctx>().is_sid_timeout(l_p.sid_) || g_ctx().get<sid_ctx>().is_upgrade_to_websocket(l_p.sid_) ||
        g_ctx().get<sid_ctx>().is_sid_close(l_p.sid_))
      throw_exception(
          http_request_error{boost::beast::http::status::bad_request, "sid超时, 或者已经进行了协议升级, 或者已经关闭"}
      );

    boost::asio::system_timer l_timer{co_await boost::asio::this_coro::executor, 250ms};
    co_await l_timer.async_wait(boost::asio::use_awaitable);

    if (g_ctx().get<sid_ctx>().is_sid_close(l_p.sid_))
      co_return in_handle->make_msg(dump_message({}, engine_io_packet_type::noop));
    if (auto l_packet = event_->get_last_event(); !l_packet.empty()) {
      co_return in_handle->make_msg(std::move(l_packet));
    }
    co_return in_handle->make_msg(dump_message({}, engine_io_packet_type::ping));
  }

  [[nodiscard]] bool has_websocket() const override { return true; }
  boost::asio::awaitable<void> websocket_callback(
      boost::beast::websocket::stream<http::tcp_stream_type> in_stream, http::session_data_ptr in_handle
  ) override {
    auto l_websocket = std::make_shared<socket_io_websocket_core>(in_handle, std::move(in_stream));
    boost::asio::co_spawn(
        co_await boost::asio::this_coro::executor, l_websocket->run(),
        boost::asio::consign(boost::asio::detached, l_websocket)
    );
  }
};

class socket_io_http_post : public socket_io_http_base_fun {
 public:
  socket_io_http_post(const std::string& in_path, const std::shared_ptr<event_base>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::post, in_path, in_event) {}

  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override {
    auto l_p = parse_query_data(in_handle->url_);
    // 注册
    if (l_p.sid_.is_nil()) throw_exception(http_request_error{boost::beast::http::status::bad_request, "sid为空"});

    if (in_handle->content_type_ != http::detail::content_type::text_plain)  // TODO: 二进制数据, 未实现
      co_return in_handle->make_msg(std::string{"null"});
    auto l_body = std::get<std::string>(in_handle->body_);
    switch (auto l_engine_packet = parse_engine_packet(l_body); l_engine_packet) {
      case engine_io_packet_type::open:
        break;
      case engine_io_packet_type::ping:
        g_ctx().get<sid_ctx>().update_sid_time(l_p.sid_);
        break;
      case engine_io_packet_type::pong:
        g_ctx().get<sid_ctx>().update_sid_time(l_p.sid_);
        co_return in_handle->make_msg(std::string{});
        break;
      case engine_io_packet_type::message:
        break;
      case engine_io_packet_type::close:
        g_ctx().get<sid_ctx>().close_sid(l_p.sid_);
        co_return in_handle->make_msg(dump_message({}, engine_io_packet_type::noop));
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
};

class socket_io_http_put : public socket_io_http_base_fun {
 public:
  socket_io_http_put(const std::string& in_path, const std::shared_ptr<event_base>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::put, in_path, in_event) {}
  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "不支持put请求");
  }
};

void create_socket_io(
    http::http_route& in_route, const std::shared_ptr<event_base>& in_event, const std::string& in_path
) {
  in_route.reg(std::make_shared<socket_io_http_get>(in_path, in_event))
      .reg(std::make_shared<socket_io_http_post>(in_path, in_event))
      .reg(std::make_shared<socket_io_http_put>(in_path, in_event));
}

}  // namespace doodle::socket_io