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

void client::async_shutdown() {
  // 异步关闭流
  ptr->is_connect = false;
  ptr->ssl_stream.async_shutdown(
      [l_c = shared_from_this()](
          boost::system::error_code ec
      ) {
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
  );
}
boost::beast::ssl_stream<boost::beast::tcp_stream>& client::ssl_stream() {
  return ptr->ssl_stream;
}
bool client::is_connect() const {
  return ptr->is_connect;
}
void client::is_connect(bool in_connect) {
  ptr->is_connect = in_connect;
}
boost::asio::ssl::context& client::ssl_context() {
  return ptr->ssl_context_;
}
boost::asio::ip::tcp::resolver& client::resolver() {
  return ptr->resolver_;
}

client::~client() noexcept = default;

}  // namespace doodle::dingding
