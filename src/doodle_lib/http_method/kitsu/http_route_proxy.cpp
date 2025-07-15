//
// Created by TD on 24-10-10.
//

#include "http_route_proxy.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
http_function_ptr http_route_proxy::operator()(
    boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
) const {
  auto l_ptr = http_route::operator()(in_verb, in_segment, in_handle);
  if (!l_ptr && in_segment.front() == "api") return my_not_function;
  if (l_ptr == nullptr) {
    return in_verb == boost::beast::http::verb::head ? head_file_ : get_file_;
  }
  return l_ptr;
}

http_route_proxy& http_route_proxy::reg_proxy(const http_function_ptr in_function) {
  proxy_urls.emplace_back(in_function);
  return *this;
}
void http_route_proxy::reg_front_end(const http_function_ptr in_get_index, const http_function_ptr in_head_file) {
  get_file_  = in_get_index;
  head_file_ = in_head_file;
}

boost::asio::awaitable<tcp_stream_type_ptr> http_route_proxy::create_proxy() const {
  using co_executor_type = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;

  using resolver_t       = co_executor_type::as_default_on_t<boost::asio::ip::tcp::resolver>;
  using resolver_ptr     = std::shared_ptr<resolver_t>;

  auto l_tcp             = std::make_shared<tcp_stream_type>(co_await boost::asio::this_coro::executor);
  auto l_resolver        = std::make_shared<resolver_t>(co_await boost::asio::this_coro::executor);

  auto kitsu_url         = g_ctx().get<kitsu_ctx_t>().url_;
  boost::urls::url l_url{kitsu_url};
  boost::system::error_code l_ec{};
  boost::asio::ip::tcp::resolver::results_type l_result;

  std::tie(l_ec, l_result) =
      co_await l_resolver->async_resolve(l_url.host(), l_url.has_port() ? l_url.port() : l_url.scheme());
  if (l_ec) {
    default_logger_raw()->log(log_loc(), level::err, "async_resolve error: {}", l_ec.message());
    co_return nullptr;
  }
  l_tcp->expires_after(30s);
  std::tie(l_ec, std::ignore) = co_await l_tcp->async_connect(l_result);
  if (l_ec) {
    default_logger_raw()->log(log_loc(), level::err, "async_connect error: {}", l_ec.message());
    co_return nullptr;
  }
  l_tcp->expires_after(30s);

  co_return l_tcp;
}

}  // namespace doodle::http::kitsu