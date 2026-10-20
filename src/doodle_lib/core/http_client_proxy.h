#pragma once

#include <doodle_lib/core/base64.h>
#include <doodle_lib/core/http_client_core.h>

#include <boost/beast/http.hpp>
#include <boost/url.hpp>

#include <memory>
#include <string>
#include <utility>

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
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        resolve_and_connect_compose{this, resolver_t{socket_->get_executor()}}, in_handle
    );
  }

  void resolve_and_connect_sync() override;

  [[nodiscard]] const std::string& get_proxy_host() const { return proxy_host_; }
  [[nodiscard]] const std::string& get_proxy_port() const { return proxy_port_; }

 private:
  class resolve_and_connect_compose {
   public:
    resolve_and_connect_compose(http_client_ssl_via_proxy* in_self, resolver_t in_resolver)
        : self_(in_self),
          resolver_(std::move(in_resolver)),
          connect_parser_(std::make_shared<boost::beast::http::response_parser<boost::beast::http::empty_body>>()) {}

    http_client_ssl_via_proxy* self_;
    resolver_t resolver_;
    boost::beast::http::request<boost::beast::http::empty_body> connect_request_{};
    std::shared_ptr<boost::beast::http::response_parser<boost::beast::http::empty_body>> connect_parser_;

    template <typename Self>
    void operator()(Self&& self) {
      if (self_->proxy_host_.empty()) {
        self_->socket_->async_handshake(boost::asio::ssl::stream_base::client, std::move(self));
        return;
      }
      resolver_.async_resolve(self_->proxy_host_, self_->proxy_port_, std::move(self));
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec) {
      self.complete(in_ec);
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, resolver_t::results_type in_results) {
      if (in_ec) {
        self.complete(in_ec);
        return;
      }
      self_->resolver_results_ = in_results;
      boost::beast::get_lowest_layer(*self_->socket_).async_connect(self_->resolver_results_, std::move(self));
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, boost::asio::ip::tcp::endpoint in_endpoint) {
      if (in_ec) {
        self.complete(in_ec);
        return;
      }
      connect_request_ = self_->make_connect_request();
      boost::beast::http::async_write(
          boost::beast::get_lowest_layer(*self_->socket_), connect_request_, std::move(self)
      );
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, std::size_t in_bytes_transferred) {
      if (in_ec) {
        self.complete(in_ec);
        return;
      }
      if (in_bytes_transferred == 0) {
        boost::beast::http::async_read_header(
            boost::beast::get_lowest_layer(*self_->socket_), self_->buffer_, *connect_parser_, std::move(self)
        );
        return;
      }
      if (connect_parser_->get().result_int() / 100 != 2) {
        self.complete(boost::system::error_code{
            boost::system::errc::connection_refused, boost::system::generic_category()
        });
        return;
      }
      self_->buffer_.consume(self_->buffer_.size());
      self_->socket_->async_handshake(boost::asio::ssl::stream_base::client, std::move(self));
    }
  };

  boost::beast::http::request<boost::beast::http::empty_body> make_connect_request() const;

  void parse_proxy_url(std::string in_proxy_url);

  std::string proxy_host_{};
  std::string proxy_port_{};
  std::string proxy_authorization_{};
};

}  // namespace doodle::http
