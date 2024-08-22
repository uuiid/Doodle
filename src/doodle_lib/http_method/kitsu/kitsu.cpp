//
// Created by TD on 24-8-21.
//
#include "kitsu.h"

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/kitsu/user.h>
namespace doodle::http {

namespace {
boost::asio::awaitable<tcp_stream_type_ptr> create_kitsu_proxy() {
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
}  // namespace

http_route_ptr create_kitsu_route() {
  auto l_router = std::make_shared<http_route>(create_kitsu_proxy);
  kitsu::user_reg(*l_router);
  return l_router;
}

namespace kitsu {
http::detail::http_client_data_base_ptr create_kitsu_proxy(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data{};
  if (!in_handle->user_data_.has_value()) {
    kitsu_data_t l_data{std::make_shared<detail::http_client_data_base>(g_io_context().get_executor())};
    l_client_data = l_data.http_kitsu_;
    l_client_data->init(g_ctx().get<kitsu_ctx_t>().url_);
    in_handle->user_data_ = l_data;
  } else {
    l_client_data = std::any_cast<kitsu_data_t&>(in_handle->user_data_).http_kitsu_;
  }
  return l_client_data;
}
}  // namespace kitsu

}  // namespace doodle::http