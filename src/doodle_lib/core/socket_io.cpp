//
// Created by TD on 25-1-23.
//

#include "socket_io.h"

#include <doodle_core/core/co_queue.h>

#include <doodle_lib/core/engine_io.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/socket_io/websocket_impl.h>

#include <boost/asio/experimental/parallel_group.hpp>
namespace doodle::socket_io {
socket_io_packet socket_io_packet::parse(const std::string& in_str) {
  socket_io_packet l_packet{};
  std::size_t l_pos{};
  {
    std::int32_t l_type = in_str.front() - '0';
    if (l_type < 0 || l_type > enum_to_num(socket_io_packet_type::binary_ack))
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
  if (l_pos == in_str.size()) return l_packet;

  if (auto l_id = in_str[l_pos + 1]; std::isdigit(l_id)) {
    auto l_begin = l_pos + 1;
    std::size_t l_size{};
    while (std::isdigit(in_str[++l_pos]) && l_pos < in_str.size()) ++l_size;
    l_packet.id_ = std::stoll(in_str.substr(l_begin, l_size));
    --l_pos;
  }
  ++l_pos;
  if (in_str[l_pos] != '[')
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "数据包格式错误"});
  if (in_str.begin() + l_pos != in_str.end())
    l_packet.json_data_ = nlohmann::json::parse(in_str.begin() + l_pos, in_str.end());
  else
    l_packet.json_data_ = nlohmann::json::object();
  return l_packet;
}

std::string socket_io_packet::dump() {
  std::string l_result{std::to_string(enum_to_num(type_))};
  if (!namespace_.empty()) l_result += namespace_ + ',';

  if (type_ == socket_io_packet_type::binary_ack || type_ == socket_io_packet_type::binary_event)
    l_result += std::to_string(binary_data_.size()) + '-';
  if (type_ == socket_io_packet_type::binary_ack || type_ == socket_io_packet_type::ack)
    l_result += std::to_string(id_);

  l_result += json_data_.dump();
  return dump_message(l_result);
}

class socket_io_http_base_fun : public ::doodle::http::http_function {
 protected:
  std::shared_ptr<sid_ctx> sid_ctx_;
  std::string url_;

 public:
  explicit socket_io_http_base_fun(
      boost::beast::http::verb in_verb, std::string in_url, const std::shared_ptr<sid_ctx>& in_sid_ctx
  )
      : http_function(in_verb, {}), sid_ctx_(std::move(in_sid_ctx)), url_(std::move(in_url)) {}

  std::tuple<bool, capture_t> set_match_url(boost::urls::segments_ref in_segments_ref) const override {
    if (in_segments_ref.buffer() == url_) return {true, {}};
    return {false, {}};
  }
};

class socket_io_http_get : public socket_io_http_base_fun {
  // 生成注册回复
  std::string generate_register_reply() {
    auto l_hd             = sid_ctx_->handshake_data_;
    l_hd.sid_             = sid_ctx_->generate()->get_sid();
    nlohmann::json l_json = l_hd;
    return dump_message(l_json.dump(), engine_io_packet_type::open);
  }

 public:
  socket_io_http_get(const std::string& in_path, const std::shared_ptr<sid_ctx>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::get, in_path, in_event) {}

  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override {
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
    co_return in_handle->make_msg(std::move(l_event));
  }

  [[nodiscard]] bool has_websocket() const override { return true; }
  boost::asio::awaitable<void> websocket_callback(
      boost::beast::websocket::stream<http::tcp_stream_type> in_stream, http::session_data_ptr in_handle
  ) override {
    auto l_websocket = std::make_shared<socket_io_websocket_core>(in_handle, sid_ctx_, std::move(in_stream));
    boost::asio::co_spawn(
        co_await boost::asio::this_coro::executor, l_websocket->run(),
        boost::asio::consign(boost::asio::detached, l_websocket)
    );
  }
};

class socket_io_http_post : public socket_io_http_base_fun {
 public:
  socket_io_http_post(const std::string& in_path, const std::shared_ptr<sid_ctx>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::post, in_path, in_event) {}

  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override {
    auto l_p = parse_query_data(in_handle->url_);
    // 注册
    if (l_p.sid_.is_nil()) throw_exception(http_request_error{boost::beast::http::status::bad_request, "sid为空"});

    if (in_handle->content_type_ != http::detail::content_type::text_plain)  // TODO: 二进制数据, 未实现
      co_return in_handle->make_msg(std::string{"null"});
    auto l_body     = std::get<std::string>(in_handle->body_);
    auto l_sid_data = sid_ctx_->get_sid(l_p.sid_);
    switch (auto l_engine_packet = parse_engine_packet(l_body); l_engine_packet) {
      case engine_io_packet_type::open:
        break;
      case engine_io_packet_type::ping:
        l_sid_data->update_sid_time();
        break;
      case engine_io_packet_type::pong:
        l_sid_data->update_sid_time();
        co_return in_handle->make_msg(std::string{});
        break;
      case engine_io_packet_type::message:
        break;
      case engine_io_packet_type::close:
        l_sid_data->close();
        co_return in_handle->make_msg(dump_message({}, engine_io_packet_type::noop));
      case engine_io_packet_type::upgrade:
      case engine_io_packet_type::noop:
        co_return in_handle->make_msg(dump_message({}, engine_io_packet_type::noop));
        break;
    }
    l_body.erase(0, 1);
    auto l_pack = socket_io_packet::parse(l_body);
    co_return in_handle->make_msg("{}");
  }
};

class socket_io_http_put : public socket_io_http_base_fun {
 public:
  socket_io_http_put(const std::string& in_path, const std::shared_ptr<sid_ctx>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::put, in_path, in_event) {}
  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "不支持put请求");
  }
};

void create_socket_io(
    http::http_route& in_route, const std::shared_ptr<sid_ctx>& in_sid_ctx, const std::string& in_path
) {
  in_route.reg(std::make_shared<socket_io_http_get>(in_path, in_sid_ctx))
      .reg(std::make_shared<socket_io_http_post>(in_path, in_sid_ctx))
      .reg(std::make_shared<socket_io_http_put>(in_path, in_sid_ctx));
}

}  // namespace doodle::socket_io