#pragma once

#include <doodle_lib/core/http_client_core.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/url.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace doodle::http {

class DOODLELIB_API http_client_ssl_via_proxy : public http_client_ssl {
 public:
  http_client_ssl_via_proxy(std::string in_target_url, boost::asio::ssl::context& in_ctx, std::string in_proxy_url);

  template <typename ResponseBody, typename RequestType, typename Handle = boost::asio::use_awaitable_t<>>
  auto read_and_write(
      boost::beast::http::request<RequestType>& in_req, boost::beast::http::response<ResponseBody>& out_res,
      Handle&& in_handle = boost::asio::use_awaitable
  ) {
    set_request_timeout(in_req);
    in_req.prepare_payload();
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        read_and_write_compose_parser<http_client_ssl_via_proxy, RequestType, ResponseBody>{
            in_req, this, boost::asio::coroutine{}, out_res
        },
        in_handle
    );
  }

  template <typename Handle>
  auto resolve_and_connect(Handle&& in_handle) {
    return boost::asio::async_compose<std::decay_t<Handle>, void(boost::system::error_code)>(
        resolve_and_connect_compose{this, resolver_t{socket_->get_executor()}}, in_handle
    );
  }

  void resolve_and_connect_sync() override;

  [[nodiscard]] const std::string& get_proxy_host() const { return proxy_host_; }
  [[nodiscard]] const std::string& get_proxy_port() const { return proxy_port_; }

 private:
  enum class socks5_phase_t {
    start,
    tcp_connecting,
    greeting_writing,
    method_reading,
    connect_writing,
    reply_head_reading,
    reply_len_reading,
    reply_addr_reading,
    handshaking
  };

  struct socks5_state_t {
    http_client_ssl_via_proxy* self_{};
    resolver_t resolver_;
    socks5_phase_t phase_{socks5_phase_t::start};
    std::array<std::uint8_t, 3> greeting_{};
    std::array<std::uint8_t, 2> method_reply_{};
    std::array<std::uint8_t, 4> reply_head_{};
    std::array<std::uint8_t, 1> domain_len_buf_{};
    std::vector<std::uint8_t> connect_request_{};
    std::vector<std::uint8_t> reply_addr_{};
  };

  class resolve_and_connect_compose {
   public:
    resolve_and_connect_compose(http_client_ssl_via_proxy* in_self, resolver_t in_resolver)
        : state_(std::make_shared<socks5_state_t>(socks5_state_t{in_self, std::move(in_resolver)})) {}

    std::shared_ptr<socks5_state_t> state_;

    template <typename Self>
    void operator()(Self&& self) {
      if (state_->self_->proxy_host_.empty()) {
        log_info(
            fmt::format(
                "代理直连: 不经过代理, 直接 TLS 握手 {}:{}", state_->self_->server_ip_, state_->self_->server_port_
            )
        );
        state_->phase_ = socks5_phase_t::handshaking;
        state_->self_->socket_->async_handshake(boost::asio::ssl::stream_base::client, std::move(self));
        return;
      }
      log_info(
          fmt::format(
              "SOCKS5: 解析代理地址 {}:{} (目标 {}:{})", state_->self_->proxy_host_, state_->self_->proxy_port_,
              state_->self_->server_ip_, state_->self_->server_port_
          )
      );
      state_->resolver_.async_resolve(state_->self_->proxy_host_, state_->self_->proxy_port_, std::move(self));
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec) {
      if (in_ec) {
        log_error(
            fmt::format(
                "代理链路 TLS 握手失败: {}:{} -> {}", state_->self_->server_ip_, state_->self_->server_port_, in_ec
            )
        );
      } else {
        log_info(
            fmt::format(
                "代理链路 TLS 握手完成: 目标 {}:{}", state_->self_->server_ip_, state_->self_->server_port_
            )
        );
      }
      self.complete(in_ec);
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, resolver_t::results_type in_results) {
      if (in_ec) {
        finish_error(self, in_ec, "代理地址解析");
        return;
      }
      state_->self_->resolver_results_ = in_results;
      const auto& l_ep                 = state_->self_->resolver_results_.begin()->endpoint();
      log_info(fmt::format("SOCKS5: 已解析到 {}:{}, 开始 TCP 连接", l_ep.address().to_string(), l_ep.port()));
      state_->phase_ = socks5_phase_t::tcp_connecting;
      boost::beast::get_lowest_layer(*state_->self_->socket_)
          .async_connect(state_->self_->resolver_results_, std::move(self));
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, boost::asio::ip::tcp::endpoint in_endpoint) {
      if (in_ec) {
        finish_error(self, in_ec, "代理 TCP 连接");
        return;
      }
      state_->self_->proxy_endpoint_ = in_endpoint;
      log_info(
          fmt::format("SOCKS5: 已连接代理 {}:{}, 发送握手", in_endpoint.address().to_string(), in_endpoint.port())
      );
      state_->greeting_ = make_socks5_greeting();
      state_->phase_    = socks5_phase_t::greeting_writing;
      boost::asio::async_write(
          boost::beast::get_lowest_layer(*state_->self_->socket_), boost::asio::buffer(state_->greeting_),
          std::move(self)
      );
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, std::size_t in_bytes_transferred) {
      if (in_ec) {
        finish_error(self, in_ec, phase_name());
        return;
      }
      switch (state_->phase_) {
        case socks5_phase_t::greeting_writing:
          state_->phase_ = socks5_phase_t::method_reading;
          boost::asio::async_read(
              boost::beast::get_lowest_layer(*state_->self_->socket_), boost::asio::buffer(state_->method_reply_),
              std::move(self)
          );
          return;
        case socks5_phase_t::method_reading:
          on_method_reply(self);
          return;
        case socks5_phase_t::connect_writing:
          state_->phase_ = socks5_phase_t::reply_head_reading;
          boost::asio::async_read(
              boost::beast::get_lowest_layer(*state_->self_->socket_), boost::asio::buffer(state_->reply_head_),
              std::move(self)
          );
          return;
        case socks5_phase_t::reply_head_reading:
          on_reply_head(self);
          return;
        case socks5_phase_t::reply_len_reading:
          on_reply_len(self);
          return;
        case socks5_phase_t::reply_addr_reading:
          on_reply_addr(self);
          return;
        default:
          finish_error(
              self,
              boost::system::error_code{
                  boost::system::errc::operation_not_supported, boost::system::generic_category()
              },
              "SOCKS5 状态机"
          );
          return;
      }
    }

   private:
    static std::array<std::uint8_t, 3> make_socks5_greeting() { return {0x05, 0x01, 0x00}; }

    static std::string socks5_rep_name(std::uint8_t in_rep) {
      switch (in_rep) {
        case 0x01:
          return "一般故障";
        case 0x02:
          return "规则不允许";
        case 0x03:
          return "网络不可达";
        case 0x04:
          return "主机不可达";
        case 0x05:
          return "连接被拒";
        case 0x06:
          return "TTL 超时";
        case 0x07:
          return "不支持的命令";
        case 0x08:
          return "不支持的地址类型";
        default:
          return "未知错误";
      }
    }

    [[nodiscard]] std::string phase_name() const {
      switch (state_->phase_) {
        case socks5_phase_t::tcp_connecting:
          return "代理 TCP 连接";
        case socks5_phase_t::greeting_writing:
          return "写 SOCKS5 握手";
        case socks5_phase_t::method_reading:
          return "读 SOCKS5 认证方式";
        case socks5_phase_t::connect_writing:
          return "写 SOCKS5 CONNECT 请求";
        case socks5_phase_t::reply_head_reading:
          return "读 SOCKS5 reply 头";
        case socks5_phase_t::reply_len_reading:
          return "读 SOCKS5 reply 域名长度";
        case socks5_phase_t::reply_addr_reading:
          return "读 SOCKS5 reply 地址";
        case socks5_phase_t::handshaking:
          return "TLS 握手";
        default:
          return "SOCKS5 协商";
      }
    }

    template <typename Self>
    static void finish_error(Self& self, boost::system::error_code in_ec, std::string_view in_where) {
      log_error(fmt::format("SOCKS5: {} 失败: {}", in_where, in_ec));
      self.complete(in_ec);
    }

    template <typename Self>
    static void finish_protocol_error(Self& self, std::string in_message) {
      log_error(fmt::format("SOCKS5: {}", in_message));
      self.complete(boost::system::error_code{
          boost::system::errc::protocol_error, boost::system::generic_category()
      });
    }

    template <typename Self>
    void on_method_reply(Self& self) {
      if (state_->method_reply_[0] != 0x05) {
        finish_protocol_error(
            self, fmt::format(
                      "代理返回版本 0x{:02X} 认证方式 0x{:02X}, 不是 SOCKS5 代理",
                      static_cast<int>(state_->method_reply_[0]), static_cast<int>(state_->method_reply_[1])
                  )
        );
        return;
      }
      if (state_->method_reply_[1] != 0x00) {
        std::string l_reason{};
        if (state_->method_reply_[1] == 0x02)
          l_reason = "代理要求用户名密码认证, 本实现不支持";
        else if (state_->method_reply_[1] == 0xFF)
          l_reason = "代理无可接受的认证方式";
        else
          l_reason =
              fmt::format("代理返回不支持的认证方式 0x{:02X}", static_cast<int>(state_->method_reply_[1]));
        finish_protocol_error(self, l_reason);
        return;
      }
      log_info("SOCKS5: 代理选择无认证, 发送 CONNECT");
      state_->connect_request_ = state_->self_->make_socks5_connect();
      state_->phase_           = socks5_phase_t::connect_writing;
      boost::asio::async_write(
          boost::beast::get_lowest_layer(*state_->self_->socket_), boost::asio::buffer(state_->connect_request_),
          std::move(self)
      );
    }

    template <typename Self>
    void on_reply_head(Self& self) {
      if (state_->reply_head_[0] != 0x05) {
        finish_protocol_error(
            self, fmt::format("reply 版本 0x{:02X}, 不是 SOCKS5 代理", static_cast<int>(state_->reply_head_[0]))
        );
        return;
      }
      if (state_->reply_head_[1] != 0x00) {
        finish_protocol_error(
            self, fmt::format(
                      "CONNECT {}:{} 被拒绝, REP=0x{:02X} {}", state_->self_->server_ip_,
                      state_->self_->server_port_,
                      static_cast<int>(state_->reply_head_[1]), socks5_rep_name(state_->reply_head_[1])
                  )
        );
        return;
      }
      switch (state_->reply_head_[3]) {
        case 0x01:
          state_->reply_addr_.assign(6, 0);
          break;
        case 0x04:
          state_->reply_addr_.assign(18, 0);
          break;
        case 0x03:
          state_->phase_ = socks5_phase_t::reply_len_reading;
          boost::asio::async_read(
              boost::beast::get_lowest_layer(*state_->self_->socket_), boost::asio::buffer(state_->domain_len_buf_),
              std::move(self)
          );
          return;
        default:
          finish_protocol_error(
              self, fmt::format("reply 里未知的地址类型 0x{:02X}", static_cast<int>(state_->reply_head_[3]))
          );
          return;
      }
      state_->phase_ = socks5_phase_t::reply_addr_reading;
      boost::asio::async_read(
          boost::beast::get_lowest_layer(*state_->self_->socket_), boost::asio::buffer(state_->reply_addr_),
          std::move(self)
      );
    }

    template <typename Self>
    void on_reply_len(Self& self) {
      const std::size_t l_len = state_->domain_len_buf_[0];
      if (l_len == 0) {
        finish_protocol_error(self, "reply 里域名长度为 0");
        return;
      }
      state_->reply_addr_.assign(l_len + 2, 0);
      state_->phase_ = socks5_phase_t::reply_addr_reading;
      boost::asio::async_read(
          boost::beast::get_lowest_layer(*state_->self_->socket_), boost::asio::buffer(state_->reply_addr_),
          std::move(self)
      );
    }

    template <typename Self>
    void on_reply_addr(Self& self) {
      std::string l_bind{};
      if (state_->reply_head_[3] == 0x03) {
        l_bind = std::string{reinterpret_cast<const char*>(state_->reply_addr_.data()), state_->reply_addr_.size() - 2};
      } else if (state_->reply_head_[3] == 0x01) {
        l_bind = boost::asio::ip::address_v4(
                     std::array<std::uint8_t, 4>{
                         state_->reply_addr_[0], state_->reply_addr_[1], state_->reply_addr_[2], state_->reply_addr_[3]
                     }
        )
                     .to_string();
      } else {
        std::array<std::uint8_t, 16> l_v6{};
        std::copy_n(state_->reply_addr_.begin(), 16, l_v6.begin());
        l_bind = boost::asio::ip::address_v6(l_v6).to_string();
      }
      const auto l_port = static_cast<std::uint16_t>(
          (state_->reply_addr_[state_->reply_addr_.size() - 2] << 8) | state_->reply_addr_.back()
      );
      log_info(
          fmt::format(
              "SOCKS5: CONNECT {}:{} 隧道已建立 (BND {}:{})", state_->self_->server_ip_,
              state_->self_->server_port_, l_bind, l_port
          )
      );
      state_->phase_ = socks5_phase_t::handshaking;
      state_->self_->socket_->async_handshake(boost::asio::ssl::stream_base::client, std::move(self));
    }
  };

  [[nodiscard]] std::vector<std::uint8_t> make_socks5_connect() const;

  void parse_proxy_url(std::string in_proxy_url);

  std::string proxy_host_{};
  std::string proxy_port_{};
  boost::asio::ip::tcp::endpoint proxy_endpoint_{};
};

}  // namespace doodle::http
