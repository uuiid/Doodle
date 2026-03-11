//
// Created by TD on 24-7-25.
//

#include "http_websocket_client.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/lib_warp/boost_fmt_error.h>
#include <doodle_lib/logger/logger.h>

namespace doodle::http {

boost::asio::awaitable<std::shared_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>>>
make_websocket_stream(const boost::urls::url& in_url) {
  using stream_type   = boost::beast::websocket::stream<boost::beast::tcp_stream>;
  auto executor_      = boost::asio::make_strand(g_io_context());
  auto web_stream_    = std::make_shared<stream_type>(executor_);

  using resolver_type = boost::asio::ip::tcp::resolver;
  auto l_resolver     = std::make_shared<resolver_type>(g_io_context());
  auto l_res          = co_await l_resolver->async_resolve(in_url.host(), in_url.port());

  auto l_res_endpoint = co_await boost::beast::get_lowest_layer(*web_stream_).async_connect(l_res);

  boost::beast::get_lowest_layer(*web_stream_).expires_never();
  web_stream_->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
  web_stream_->set_option(
      boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type& req) {
        req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
      })
  );

  co_await web_stream_->async_handshake(in_url.host(), in_url.encoded_target());
  co_return web_stream_;
}
}  // namespace doodle::http