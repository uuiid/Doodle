//
// Created by TD on 24-7-25.
//

#include "http_websocket_client.h"

#include "doodle_core/logger/logger.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/websocket_route.h>

namespace doodle::http {
boost::asio::awaitable<boost::system::error_code> http_websocket_client::init(
    const std::string& in_url, websocket_route_ptr in_websocket_route
) {
  url_             = boost::urls::url{in_url};
  websocket_route_ = in_websocket_route;
  executor_        = boost::asio::make_strand(g_io_context());
  web_stream_ =
      std::make_shared<boost::beast::websocket::stream<tcp_stream_type>>(boost::asio::make_strand(g_io_context()));
  logger_ =
      g_logger_ctrl().make_log(fmt::format("websocket_{}_{}", std::string{url_.host()}, std::string{url_.port()}));
  write_queue_limitation_ = std::make_shared<awaitable_queue_limitation>();

  using resolver_type     = executor_type::as_default_on_t<boost::asio::ip::tcp::resolver>;
  auto l_resolver         = std::make_shared<resolver_type>(g_io_context());
  auto [l_ec, l_res]      = co_await l_resolver->async_resolve(url_.host(), url_.port());
  if (l_ec) {
    logger_->error("resolver error", l_ec.what());
    co_return l_ec;
  }
  auto [l_ec_c, l_res_endpoint] = co_await boost::beast::get_lowest_layer(*web_stream_).async_connect(l_res);
  if (l_ec_c) {
    logger_->error("connect error", l_ec_c.what());
    co_return l_ec_c;
  }
  boost::beast::get_lowest_layer(*web_stream_).expires_never();
  web_stream_->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
  web_stream_->set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type& req
                                                                          ) {
    req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  }));

  auto [l_ec_h] = co_await web_stream_->async_handshake(url_.host(), url_.encoded_target());
  if (l_ec_h) {
    logger_->error("handshake error", l_ec_h.what());
    co_return l_ec_h;
  }
  co_return boost::system::error_code{};
}

void http_websocket_client::init(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, websocket_route_ptr in_websocket_route,
    logger_ptr in_logger
) {
  // executor_        = in_stream.get_executor();
  web_stream_             = std::make_shared<boost::beast::websocket::stream<tcp_stream_type>>(std::move(in_stream));
  websocket_route_        = in_websocket_route;
  logger_                 = in_logger;
  write_queue_limitation_ = std::make_shared<awaitable_queue_limitation>();
}

boost::asio::awaitable<void> http_websocket_client::async_read_websocket() {
  auto l_data     = std::make_shared<detail::http_websocket_data>();
  l_data->client_ = weak_from_this();
  l_data->logger_ = logger_;
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    boost::beast::flat_buffer l_buffer{};
    auto [l_ec_r, l_tr_s] = co_await web_stream_->async_read(l_buffer);
    if (l_ec_r) {
      if (l_ec_r == boost::beast::websocket::error::closed) {
        co_return;
      }
      logger_->error(l_ec_r.what());
      auto [l_ec_close] = co_await web_stream_->async_close(boost::beast::websocket::close_code::normal);
      if (l_ec_close) logger_->error(l_ec_close.what());
      co_return;
    }

    std::string l_call_fun_name{};
    try {
      l_data->body_ =
          nlohmann::json::parse(std::string_view{reinterpret_cast<const char*>(l_buffer.data().data()), l_buffer.size()}
          );
      l_call_fun_name = l_data->body_["type"].get<std::string>();
    } catch (const nlohmann::json::exception& in_e) {
      logger_->error(in_e.what());
      continue;
    }
    web_stream_->text(true);

    auto l_call_fun   = (*websocket_route_)(l_call_fun_name);
    std::string l_str = co_await (*l_call_fun)(l_data);
    if (l_str.empty()) continue;

    co_await async_write_websocket(l_str);
    boost::asio::co_spawn(g_io_context(), async_write_websocket(std::move(l_str)), boost::asio::detached);
  }
}

boost::asio::awaitable<boost::system::error_code> http_websocket_client::async_write_websocket(std::string in_data) {
  auto l_g              = co_await write_queue_limitation_->queue(boost::asio::use_awaitable);
  auto [l_ec_w, l_tr_w] = co_await web_stream_->async_write(boost::asio::buffer(in_data));
  if (l_ec_w) {
    logger_->error(l_ec_w.what());
    auto [l_ec_close] = co_await web_stream_->async_close(boost::beast::websocket::close_code::normal);
    if (l_ec_close) {
      logger_->error(l_ec_close.what());
      co_return l_ec_close;
    }
    co_return l_ec_w;
  }
  co_return l_ec_w;
}
}  // namespace doodle::http