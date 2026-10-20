#include "http_client_proxy.h"

#include <doodle_core/exception/exception.h>

#include <boost/asio/ip/tcp.hpp>

namespace doodle::http {

http_client_ssl_via_proxy::http_client_ssl_via_proxy(
    std::string in_target_url, boost::asio::ssl::context& in_ctx, std::string in_proxy_url
)
    : http_client_ssl(std::move(in_target_url), in_ctx) {
  parse_proxy_url(std::move(in_proxy_url));
}

void http_client_ssl_via_proxy::parse_proxy_url(std::string in_proxy_url) {
  proxy_host_.clear();
  proxy_port_.clear();
  proxy_authorization_.clear();
  if (in_proxy_url.empty()) return;

  boost::system::result<boost::urls::url_view> l_url = boost::urls::parse_uri(in_proxy_url);
  DOODLE_CHICK(l_url.has_value(), "HTTP 代理地址无法解析: {}", in_proxy_url);
  DOODLE_CHICK(!l_url->host().empty(), "HTTP 代理地址缺少主机名: {}", in_proxy_url);

  proxy_host_ = std::string{l_url->host()};
  proxy_port_ = l_url->has_port() ? std::string{l_url->port()} : std::string{"8080"};

  if (!l_url->user().empty()) {
    auto l_token = base64_encode(fmt::format("{}:{}", l_url->user(), l_url->password()));
    proxy_authorization_ = fmt::format("Basic {}", l_token);
  }
}

boost::beast::http::request<boost::beast::http::empty_body> http_client_ssl_via_proxy::make_connect_request() const {
  boost::beast::http::request<boost::beast::http::empty_body> l_req{
      boost::beast::http::verb::connect, server_ip_and_port_, 11
  };
  l_req.set(boost::beast::http::field::host, server_ip_and_port_);
  l_req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  if (!proxy_authorization_.empty()) l_req.set(boost::beast::http::field::proxy_authorization, proxy_authorization_);
  return l_req;
}

void http_client_ssl_via_proxy::resolve_and_connect_sync() {
  boost::system::error_code l_ec{};
  resolver_t resolver{socket_->get_executor()};

  if (proxy_host_.empty()) {
    auto l_results = resolver.resolve(server_ip_, server_port_, l_ec);
    if (l_ec) throw boost::system::system_error(l_ec);
    resolver_results_ = l_results;
    boost::beast::get_lowest_layer(*socket_).connect(resolver_results_, l_ec);
    if (l_ec) throw boost::system::system_error(l_ec);
    socket_->handshake(boost::asio::ssl::stream_base::client, l_ec);
    if (l_ec) throw boost::system::system_error(l_ec);
    return;
  }

  auto l_proxy_results = resolver.resolve(proxy_host_, proxy_port_, l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);
  boost::beast::get_lowest_layer(*socket_).connect(l_proxy_results, l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);

  auto l_connect_req = make_connect_request();
  boost::beast::http::write(boost::beast::get_lowest_layer(*socket_), l_connect_req, l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);

  boost::beast::http::response_parser<boost::beast::http::empty_body> l_parser{};
  boost::beast::http::read_header(boost::beast::get_lowest_layer(*socket_), buffer_, l_parser, l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);
  DOODLE_CHICK(
      l_parser.get().result_int() / 100 == 2, "HTTP 代理拒绝建立隧道到 {}: {} {}", server_ip_and_port_,
      static_cast<int>(l_parser.get().result()), l_parser.get().reason()
  );
  buffer_.consume(buffer_.size());

  socket_->handshake(boost::asio::ssl::stream_base::client, l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);
}

}  // namespace doodle::http
