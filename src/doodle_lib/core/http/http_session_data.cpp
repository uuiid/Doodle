//
// Created by TD on 2024/2/20.
//

#include "http_session_data.h"

#include <doodle_core/lib_warp/boost_fmt_beast.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/lib_warp/boost_fmt_url.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_websocket_client.h>
#include <doodle_lib/core/http/websocket_route.h>

#include <boost/asio/experimental/parallel_group.hpp>

namespace doodle::http {
namespace detail {
using executor_type   = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
using endpoint_type   = boost::asio::ip::tcp::endpoint;
using tcp_stream_type = executor_type::as_default_on_t<boost::beast::tcp_stream>;

boost::asio::awaitable<void> async_websocket_session(
    tcp_stream_type in_socket, boost::beast::http::request<boost::beast::http::empty_body> in_req,
    websocket_route_ptr in_websocket_route, logger_ptr in_logger
) {
  using executor_type   = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using tcp_stream_type = executor_type::as_default_on_t<boost::beast::tcp_stream>;

  boost::beast::websocket::stream<tcp_stream_type> l_stream{std::move(in_socket)};
  l_stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  l_stream.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
    res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle-server");
  }));
  auto [l_ec] = co_await l_stream.async_accept(in_req);
  if (l_ec) {
    in_logger->error("无法接受请求 {}", l_ec.what());
    co_return;
  }
  auto l_c = std::make_shared<http_websocket_client>();
  l_c->init(std::move(l_stream), in_websocket_route, in_logger);
  co_await l_c->async_read_websocket();
  co_return;
}

boost::asio::awaitable<void> async_session(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr) {
  using executor_type    = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using endpoint_type    = boost::asio::ip::tcp::endpoint;
  using tcp_stream_type  = executor_type::as_default_on_t<boost::beast::tcp_stream>;
  using session_data_ptr = std::shared_ptr<session_data>;
  tcp_stream_type l_stream{std::move(in_socket)};
  l_stream.expires_after(30s);
  auto l_session     = std::make_shared<session_data>();

  l_session->logger_ = std::make_shared<spdlog::async_logger>(
      fmt::format("{}_{}", "socket", SOCKET(l_stream.socket().native_handle())),
      spdlog::sinks_init_list{g_logger_ctrl().rotating_file_sink_}, spdlog::thread_pool()
  );

  boost::beast::flat_buffer buffer_;
  auto l_request_parser = std::make_shared<boost::beast::http::request_parser<boost::beast::http::empty_body>>();
  std::shared_ptr<boost::beast::http::request_parser<boost::beast::http::string_body>> l_request_parser_string;

  auto [l_ec, bytes_transferred] = co_await boost::beast::http::async_read_header(l_stream, buffer_, *l_request_parser);
  if (l_ec) {
    l_session->logger_->log(log_loc(), level::err, "读取头部失败 {}", l_ec);
    l_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_ec);
    l_stream.close();
    co_return;
  }
  l_stream.expires_after(30s);
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    l_session->version_    = l_request_parser->get().version();
    l_session->keep_alive_ = l_request_parser->keep_alive();
    l_session->url_        = boost::url{l_request_parser->get().target()};

    l_session->logger_->log(
        log_loc(), level::info, "开始解析 url {} {}", l_request_parser->get().method(), l_session->url_
    );
    auto l_method              = l_request_parser->get().method();
    std::string l_content_type = l_request_parser->get()[boost::beast::http::field::content_type];
    // 检查内容格式
    boost::system::error_code l_error_code{};
    switch (l_method) {
      case boost::beast::http::verb::get:
        if (boost::beast::websocket::is_upgrade(l_request_parser->get()) &&
            (*in_route_ptr)(l_method, l_session->url_.segments(), l_session)->has_websocket()) {
          boost::beast::get_lowest_layer(l_stream).expires_never();
          auto l_websocket_route = std::make_shared<websocket_route>();
          (*in_route_ptr)(l_method, l_session->url_.segments(), l_session)->websocket_callback_(l_websocket_route);
          co_await async_websocket_session(
              std::move(l_stream), l_request_parser->get(), l_websocket_route, l_session->logger_
          );
          co_return;
        }
      case boost::beast::http::verb::head:
        case boost::beast::http::verb::options:
        l_session->req_header_ = std::move(l_request_parser->release().base());
        break;

      case boost::beast::http::verb::post:
      case boost::beast::http::verb::put:
      case boost::beast::http::verb::delete_:
      case boost::beast::http::verb::patch:
        l_request_parser_string = std::make_shared<boost::beast::http::request_parser<boost::beast::http::string_body>>(
            std::move(*l_request_parser)
        );
        co_await boost::beast::http::async_read(l_stream, buffer_, *l_request_parser_string);
        l_stream.expires_after(30s);

        if (l_content_type.find("application/json") != std::string::npos) {
          try {
            l_session->body_         = nlohmann::json::parse(l_request_parser_string->get().body());
            l_session->content_type_ = content_type::application_json;
          } catch (const nlohmann::json::exception& e) {
            l_session->logger_->log(log_loc(), level::err, "json 解析错误 {}", e.what());
            l_error_code = error_enum::bad_json_string;
          }
        } else {
          l_session->body_ = l_request_parser_string->get().body();
        }
        l_session->req_header_ = std::move(l_request_parser_string->release().base());
        break;
      default:
        co_return;
    }
    // todo: 请求分发到对应的处理函数
    auto l_callback = (*in_route_ptr)(l_method, l_session->url_.segments(), l_session);

    auto l_gen = l_error_code ? l_session->make_error_code_msg(boost::beast::http::status::bad_request, l_error_code)
                              : co_await l_callback->callback_(l_session);

    // auto l_gen  = co_await l_callback->callback_(l_session);

    if (!l_session->keep_alive_) {
      auto [l_ec2, _] = co_await boost::beast::async_write(l_stream, std::move(l_gen));
      if (l_ec2) {
        l_session->logger_->log(log_loc(), level::err, "发送错误码 {}", l_ec2);
      }
      l_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_ec2);
      l_stream.close();
      co_return;
    }

    // 初始化新的 parser
    l_request_parser = std::make_shared<boost::beast::http::request_parser<boost::beast::http::empty_body>>();
    auto [_, l_ec_r, l_sz_r, l_ec_w, l_sz_w] =
        co_await boost::asio::experimental::make_parallel_group(
            boost::beast::http::async_read_header(l_stream, buffer_, *l_request_parser, boost::asio::deferred),
            boost::beast::async_write(l_stream, std::move(l_gen), boost::asio::deferred)
        )
            .async_wait(
                boost::asio::experimental::wait_for_all(), boost::asio::as_tuple(boost::asio::use_awaitable_t<>{})
            );
    if (l_ec_r || l_ec_w) {
      if (l_ec_r) {
        l_session->logger_->log(log_loc(), level::err, "读取头部失败 {}", l_ec_r);
        l_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_ec_r);
        l_stream.close();
      }
      if (l_ec_w) {
        l_session->logger_->log(log_loc(), level::err, "发送错误 {}", l_ec_w);
        l_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_ec_w);
        l_stream.close();
      }
      co_return;
    }
    l_stream.expires_after(30s);
  }
}

boost::beast::http::message_generator session_data::make_error_code_msg(
    boost::beast::http::status in_code, const std::string& in_str, std::int32_t in_msg_code
) {
  logger_->log(log_loc(), level::err, "发送错误码 {} {}", in_code, in_str);
  if (in_msg_code == -1) in_msg_code = enum_to_num(in_code);

  boost::beast::http::response<boost::beast::http::string_body> l_response{in_code, version_};
  l_response.set(boost::beast::http::field::content_type, "plain/text");
  l_response.set(boost::beast::http::field::accept, "application/json");
  l_response.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_response.keep_alive(keep_alive_);
  l_response.body() = nlohmann::json{{"error", in_str}, {"core", in_msg_code}}.dump();
  l_response.prepare_payload();
  return l_response;
}
} // namespace detail
} // namespace doodle::http