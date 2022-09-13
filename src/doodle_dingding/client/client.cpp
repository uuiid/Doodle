//
// Created by TD on 2022/9/7.
//

#include "client.h"

#include <doodle_core/doodle_core.h>
#include <doodle_dingding/configure/config.h>
#include <doodle_dingding/fmt_lib/boost_beast_fmt.h>

#include <boost/asio.hpp>

#include <utility>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/system.hpp>

#include <boost/url.hpp>

namespace doodle::dingding {
class client::impl {
 public:
  explicit impl(
      boost::asio::any_io_executor in_executor,
      boost::asio::ssl::context& in_ssl_context
  )
      : io_executor_(std::move(in_executor)),
        ssl_context_(in_ssl_context),
        resolver_(io_executor_),
        ssl_stream(io_executor_, ssl_context_) {}

  boost::asio::any_io_executor io_executor_;
  boost::asio::ssl::context& ssl_context_;

  boost::asio::ip::tcp::resolver resolver_;
  boost::beast::ssl_stream<boost::beast::tcp_stream> ssl_stream;
  bool init_{false};
  bool is_connect{false};

  config_type config;
};

void client::set_openssl(const std::string& host) {
  if (!ptr->init_) {
    if (
        SSL_set_tlsext_host_name(ptr->ssl_stream.native_handle(), host.c_str()) &&
        ::ERR_get_error() != 0ul
    ) {
      throw_exception(boost::system::system_error{
          static_cast<int>(::ERR_get_error()),
          boost::asio::error::get_ssl_category()});
    }
    ptr->init_ = true;
  }
}

client::client(
    const boost::asio::any_io_executor& in_executor,
    boost::asio::ssl::context& in_ssl_context
)
    : ptr(std::make_unique<impl>(in_executor, in_ssl_context)) {
}

client::executor_type client::get_executor() noexcept {
  return ptr->io_executor_;
}

void client::run(const std::string& in_host, const std::string& in_target) {
  boost::url l_url{fmt::format("{}:{}{}", in_host, "443"s, in_target)};
  client_ns::http_req_res<
      boost::beast::http::request<boost::beast::http::empty_body>,
      boost::beast::http::response<boost::beast::http::string_body> >
      l_http_req_res{shared_from_this()};
  l_http_req_res.req_attr.method(boost::beast::http::verb::get);
  l_http_req_res.url_attr = l_url;
  return run(l_http_req_res);
}

void client::run(
    const boost::url& in_url,
    const config_type& in_config_type
) {
  const std::string host{in_url.host()};
  set_openssl(host);
  const std::string port{in_url.has_port()  //
                             ? std::string{in_url.port()}
                             : "443"s};
  ptr->config = in_config_type;
  /// \brief 如果已经连接, 复用 连接
  if (ptr->is_connect) {
    // 设置操作超时
    boost::beast::get_lowest_layer(ptr->ssl_stream)
        .expires_after(std::chrono::seconds(30));

    // 向远程主机发送 HTTP 请求
    ptr->config.async_write(ptr->ssl_stream);
    ptr->ssl_stream.get_executor();
  } else {
    // Look up the domain name
    ptr->resolver_.async_resolve(
        host,
        port,
        boost::beast::bind_front_handler(&client::on_resolve, shared_from_this())
    );
  }
}

void client::on_resolve(
    boost::system::error_code ec,
    const boost::asio::ip::basic_resolver<
        boost::asio::ip::tcp, boost::asio::any_io_executor>::results_type& results
) {
  if (ec) {
    throw_exception(boost::system::system_error{ec});
  }

  /// \brief 超时设置
  boost::beast::get_lowest_layer(
      ptr->ssl_stream
  )
      .expires_after(30s);

  ptr->ssl_stream.set_verify_mode(boost::asio::ssl::verify_peer);

  ptr->ssl_stream.set_verify_callback(
      [](bool preverified,
         boost::asio::ssl::verify_context& ctx) -> bool {
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        DOODLE_LOG_INFO("Verifying {}", subject_name)

        return true;
      }
  );

  boost::beast::get_lowest_layer(ptr->ssl_stream)
      .async_connect(
          results,
          boost::beast::bind_front_handler(&client::on_connect, shared_from_this())
      );
}

void client::on_connect(
    boost::system::error_code ec,
    const boost::asio::ip::basic_endpoint<boost::asio::ip::tcp>&
) {
  if (ec) {
    throw_exception(boost::system::system_error{ec});
  }

  // 执行SSL握手
  ptr->ssl_stream.async_handshake(
      boost::asio::ssl::stream_base::client,
      boost::beast::bind_front_handler(
          &client::on_handshake,
          shared_from_this()
      )
  );
}

void client::on_handshake(boost::system::error_code ec) {
  if (ec) {
    throw_exception(boost::system::system_error{ec});
  }

  // 设置操作超时
  boost::beast::get_lowest_layer(ptr->ssl_stream)
      .expires_after(std::chrono::seconds(30));
  ptr->is_connect = true;
  // 向远程主机发送 HTTP 请求
  ptr->config.async_write(ptr->ssl_stream);
}

void client::on_write(boost::system::error_code ec, std::size_t bytes_transferred) const {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    throw_exception(boost::system::system_error{ec});
  }

  // 接收HTTP响应
  ptr->config.async_read(ptr->ssl_stream);
}

void client::async_shutdown() {
  // 异步关闭流
  ptr->is_connect = false;
  ptr->ssl_stream.async_shutdown(
      boost::beast::bind_front_handler(
          &client::on_shutdown,
          shared_from_this()
      )
  );
}
void client::on_shutdown(boost::system::error_code ec) {
  if (ec == boost::asio::error::eof) {
    // Rationale:
    // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
    ec = {};
  }
  if (ec) {
    throw_exception(boost::system::system_error{ec});
  }

  // 成功关机
}


client::~client() noexcept = default;

}  // namespace doodle::dingding
