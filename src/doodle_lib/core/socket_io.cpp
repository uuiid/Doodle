//
// Created by TD on 25-1-23.
//

#include "socket_io.h"

#include <doodle_lib/core/engine_io.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
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
    default_logger_raw()->info(std::string{in_segments_ref.buffer()});
    if (in_segments_ref.buffer() == url_) return {true, {}};
    return {false, {}};
  }
};

class socket_io_http_get : public socket_io_http_base_fun {
 public:
  socket_io_http_get(const std::string& in_path, const std::shared_ptr<event_base>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::get, in_path, in_event) {}

  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override {
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
  [[nodiscard]] bool has_websocket() const override { return true; }

};

class socket_io_http_post : public socket_io_http_base_fun {
 public:
  socket_io_http_post(const std::string& in_path, const std::shared_ptr<event_base>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::post, in_path, in_event) {}

  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override {
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