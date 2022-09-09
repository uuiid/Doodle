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
  boost::beast::flat_buffer buffer_;
  boost::beast::http::request<boost::beast::http::empty_body> req_;
  boost::beast::http::response<boost::beast::http::string_body> res_;
};

client::client(
    const boost::asio::any_io_executor& in_executor,
    boost::asio::ssl::context& in_ssl_context
)
    : ptr(std::make_unique<impl>(in_executor, in_ssl_context)) {
}
void client::run(
    const std::string& in_host,
    const std::int32_t& in_port,
    const std::string& in_target
) {
  run(in_host, fmt::to_string(in_port), in_target);
}

void client::run(const std::string& in_host, const std::string& in_target) {
  return run(in_host, 443, in_target);
}
void client::run(
    const std::string& in_host,
    const std::string& in_port,
    const std::string& in_target
) {
  boost::url l_url{fmt::format("{}:{}{}", in_host, in_port, in_target)};
  return run(l_url);
}
void client::run(
    boost::url& in_url
) {
  const std::string host{in_url.host()};
  const std::string port{in_url.has_port()  //
                             ? std::string{in_url.port()}
                             : "443"s};
  in_url.remove_origin();  /// \brief 去除一部分

  if (SSL_set_tlsext_host_name(ptr->ssl_stream.native_handle(), host.c_str()) && ::ERR_get_error() != 0ul) {
    throw_exception(boost::system::system_error{
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category()});
  }

  // Set up an HTTP GET request message
  ptr->req_.version(11);
  ptr->req_.method(boost::beast::http::verb::get);
  ptr->req_.target(in_url.c_str());
  ptr->req_.set(boost::beast::http::field::host, host);
  ptr->req_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  ptr->req_.prepare_payload();
//  DOODLE_LOG_INFO(ptr->req_);

  // Look up the domain name
  ptr->resolver_.async_resolve(
      host,
      port,
      boost::beast::bind_front_handler(&client::on_resolve, shared_from_this())
  );
}
void client::on_resolve(
    boost::system::error_code ec,
    const boost::asio::ip::basic_resolver<
        boost::asio::ip::tcp, boost::asio::any_io_executor>::results_type& results
) {
  if (ec) {
    DOODLE_LOG_INFO("resolve {}", ec.what());
    return;
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
    DOODLE_LOG_INFO("on_connect {}", ec.what());
    return;
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
    DOODLE_LOG_INFO("handshake {}", ec.message());
    return;
  }

  // 设置操作超时
  boost::beast::get_lowest_layer(ptr->ssl_stream)
      .expires_after(std::chrono::seconds(30));

  // 向远程主机发送 HTTP 请求
  boost::beast::http::async_write(
      ptr->ssl_stream, ptr->req_,
      boost::beast::bind_front_handler(&client::on_write, shared_from_this())
  );
}
void client::on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    DOODLE_LOG_INFO("write {}", ec.what());
    return;
  }

  // Receive the HTTP response
  boost::beast::http::async_read(
      ptr->ssl_stream, ptr->buffer_, ptr->res_,
      boost::beast::bind_front_handler(&client::on_read, shared_from_this())
  );
}
void client::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) {
    DOODLE_LOG_INFO("read {}", ec.what());
    return;
  }

  // 打印消息
  DOODLE_LOG_INFO(ptr->res_.body())

  // Set a timeout on the operation
  boost::beast::get_lowest_layer(ptr->ssl_stream)
      .expires_after(30s);

  // Gracefully close the stream
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
    DOODLE_LOG_INFO("shutdown {}", ec.what());
  }

  // 成功关机
}
std::string client::gettoken() {
  boost::url l_url{};
  boost::url l_method{"gettoken"};
  l_method.params().set("appkey", dingding_config::get().app_key);
  l_method.params().set("appsecret", dingding_config::get().app_value);

  boost::urls::resolve(boost::urls::url_view{dingding_host}, l_method, l_url);

  //  DOODLE_LOG_INFO(l_url.string());
  //  DOODLE_LOG_INFO(ptr->req_);
  //  DOODLE_LOG_INFO(l_url.remove_origin().string());

  run(l_url);
  return ptr->res_.body();
}

client::~client() noexcept = default;

}  // namespace doodle::dingding
