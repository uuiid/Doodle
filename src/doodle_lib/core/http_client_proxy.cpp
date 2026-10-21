#include "http_client_proxy.h"

#include <doodle_core/exception/exception.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::http {
namespace {
constexpr std::uint8_t g_socks5_version{0x05};
constexpr std::uint8_t g_socks5_cmd_connect{0x01};
constexpr std::uint8_t g_socks5_atyp_ipv4{0x01};
constexpr std::uint8_t g_socks5_atyp_domain{0x03};
constexpr std::uint8_t g_socks5_atyp_ipv6{0x04};
constexpr std::uint8_t g_socks5_method_no_auth{0x00};
constexpr std::string_view g_socks5_default_port{"1080"};

std::string socks5_rep_name(std::uint8_t in_rep) {
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

void check_socks5_method(std::span<const std::uint8_t, 2> in_reply) {
  DOODLE_CHICK(
      in_reply[0] == g_socks5_version, "SOCKS5: 代理返回版本 0x{:02X}, 不是 SOCKS5 代理",
      static_cast<int>(in_reply[0])
  );
  DOODLE_CHICK(
      in_reply[1] == g_socks5_method_no_auth, "SOCKS5: 代理要求认证方式 0x{:02X}, 本实现只支持无认证",
      static_cast<int>(in_reply[1])
  );
}

std::vector<std::uint8_t> make_socks5_connect_request(std::string_view in_host, std::string_view in_port) {
  DOODLE_CHICK(!in_host.empty(), "SOCKS5: 目标主机名为空");
  DOODLE_CHICK(in_host.size() <= 255, "SOCKS5: 目标主机名过长 ({} 字节), 上限 255", in_host.size());
  std::uint16_t l_port{};
  try {
    l_port = boost::numeric_cast<std::uint16_t>(boost::lexical_cast<std::int64_t>(in_port));
  } catch (...) {
    throw_exception(doodle_error{"SOCKS5: 目标端口非法: {}", in_port});
  }

  std::vector<std::uint8_t> l_request{};
  l_request.reserve(in_host.size() + 7);
  l_request.push_back(g_socks5_version);
  l_request.push_back(g_socks5_cmd_connect);
  l_request.push_back(0x00);
  l_request.push_back(g_socks5_atyp_domain);
  l_request.push_back(static_cast<std::uint8_t>(in_host.size()));
  l_request.insert(l_request.end(), in_host.begin(), in_host.end());
  l_request.push_back(static_cast<std::uint8_t>(l_port >> 8));
  l_request.push_back(static_cast<std::uint8_t>(l_port & 0xFF));
  return l_request;
}

std::vector<std::uint8_t> read_socks5_reply_addr(
    boost::beast::tcp_stream& in_stream, std::uint8_t in_atyp, std::string& out_bind, std::uint16_t& out_port
) {
  std::size_t l_size{};
  switch (in_atyp) {
    case g_socks5_atyp_ipv4:
      l_size = 6;
      break;
    case g_socks5_atyp_ipv6:
      l_size = 18;
      break;
    case g_socks5_atyp_domain: {
      std::array<std::uint8_t, 1> l_len_buf{};
      boost::system::error_code l_ec{};
      boost::asio::read(in_stream, boost::asio::buffer(l_len_buf), l_ec);
      if (l_ec) throw boost::system::system_error(l_ec);
      DOODLE_CHICK(l_len_buf[0] != 0, "SOCKS5: reply 里域名长度为 0");
      l_size = static_cast<std::size_t>(l_len_buf[0]) + 2;
      break;
    }
    default:
      throw_exception(
          doodle_error{fmt::format("SOCKS5: reply 里未知的地址类型 0x{:02X}", static_cast<int>(in_atyp))}
      );
  }

  std::vector<std::uint8_t> l_addr(l_size, 0);
  boost::system::error_code l_ec{};
  boost::asio::read(in_stream, boost::asio::buffer(l_addr), l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);

  if (in_atyp == g_socks5_atyp_domain) {
    out_bind = std::string{reinterpret_cast<const char*>(l_addr.data()), l_size - 2};
  } else if (in_atyp == g_socks5_atyp_ipv4) {
    out_bind = boost::asio::ip::address_v4(std::array<std::uint8_t, 4>{l_addr[0], l_addr[1], l_addr[2], l_addr[3]})
                   .to_string();
  } else {
    std::array<std::uint8_t, 16> l_v6{};
    std::copy_n(l_addr.begin(), 16, l_v6.begin());
    out_bind = boost::asio::ip::address_v6(l_v6).to_string();
  }
  out_port = static_cast<std::uint16_t>((l_addr[l_size - 2] << 8) | l_addr[l_size - 1]);
  return l_addr;
}
}  // namespace

http_client_ssl_via_proxy::http_client_ssl_via_proxy(
    std::string in_target_url, boost::asio::ssl::context& in_ctx, std::string in_proxy_url
)
    : http_client_ssl(std::move(in_target_url), in_ctx) {
  parse_proxy_url(std::move(in_proxy_url));
}

void http_client_ssl_via_proxy::parse_proxy_url(std::string in_proxy_url) {
  proxy_host_.clear();
  proxy_port_.clear();
  if (in_proxy_url.empty()) return;

  boost::system::result<boost::urls::url_view> l_url = boost::urls::parse_uri(in_proxy_url);
  DOODLE_CHICK(l_url.has_value(), "SOCKS5: 代理地址无法解析: {}", in_proxy_url);
  DOODLE_CHICK(
      l_url->scheme() == "socks5", "SOCKS5: 代理地址只支持 socks5:// 协议, 收到: {}", in_proxy_url
  );
  DOODLE_CHICK(
      l_url->user().empty(), "SOCKS5: 代理地址不支持用户名密码认证, 收到: {}", in_proxy_url
  );
  DOODLE_CHICK(!l_url->host().empty(), "SOCKS5: 代理地址缺少主机名: {}", in_proxy_url);

  proxy_host_ = std::string{l_url->host()};
  proxy_port_ = l_url->has_port() ? std::string{l_url->port()} : std::string{g_socks5_default_port};
}

std::vector<std::uint8_t> http_client_ssl_via_proxy::make_socks5_connect() const {
  return make_socks5_connect_request(server_ip_, server_port_);
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
  proxy_endpoint_ = boost::beast::get_lowest_layer(*socket_).connect(l_proxy_results, l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);

  const std::array<std::uint8_t, 3> l_greeting{g_socks5_version, 0x01, g_socks5_method_no_auth};
  boost::asio::write(boost::beast::get_lowest_layer(*socket_), boost::asio::buffer(l_greeting), l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);

  std::array<std::uint8_t, 2> l_method_reply{};
  boost::asio::read(boost::beast::get_lowest_layer(*socket_), boost::asio::buffer(l_method_reply), l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);
  check_socks5_method(l_method_reply);

  const auto l_connect_request = make_socks5_connect();
  boost::asio::write(boost::beast::get_lowest_layer(*socket_), boost::asio::buffer(l_connect_request), l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);

  std::array<std::uint8_t, 4> l_reply_head{};
  boost::asio::read(boost::beast::get_lowest_layer(*socket_), boost::asio::buffer(l_reply_head), l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);
  DOODLE_CHICK(
      l_reply_head[0] == g_socks5_version, "SOCKS5: reply 版本 0x{:02X}, 不是 SOCKS5 代理",
      static_cast<int>(l_reply_head[0])
  );
  DOODLE_CHICK(
      l_reply_head[1] == 0x00, "SOCKS5: CONNECT {}:{} 被拒绝, REP=0x{:02X} {}", server_ip_, server_port_,
      static_cast<int>(l_reply_head[1]), socks5_rep_name(l_reply_head[1])
  );

  std::string l_bind{};
  std::uint16_t l_bind_port{};
  read_socks5_reply_addr(boost::beast::get_lowest_layer(*socket_), l_reply_head[3], l_bind, l_bind_port);

  socket_->handshake(boost::asio::ssl::stream_base::client, l_ec);
  if (l_ec) throw boost::system::system_error(l_ec);
}

}  // namespace doodle::http
