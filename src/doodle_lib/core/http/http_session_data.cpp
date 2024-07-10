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
#include <doodle_lib/core/http/http_websocket_data.h>
#include <doodle_lib/core/http/socket_logger.h>

#include <boost/asio/experimental/parallel_group.hpp>

namespace doodle::http {

void http_session_data::rend_head() {
  stream_->expires_after(30s);
  request_parser_ = std::make_unique<boost::beast::http::request_parser<boost::beast::http::empty_body>>();
  buffer_.clear();
  boost::beast::http::async_read_header(
      *stream_, buffer_, *request_parser_,
      boost::beast::bind_front_handler(&http_session_data::do_read, shared_from_this())
  );
}

void http_session_data::do_read(boost::system::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    logger_->log(log_loc(), level::err, "读取头部失败 {}", ec);
    do_close();
    return;
  }
  if (!request_parser_->is_header_done()) {
    logger_->log(log_loc(), level::err, "读取头部不完整");
    do_close();
    return;
  }

  version_    = request_parser_->get().version();
  keep_alive_ = request_parser_->keep_alive();

  url_        = boost::url{request_parser_->get().target()};
  logger_->log(log_loc(), level::info, "开始解析 url {} {}", request_parser_->get().method(), url_);
  auto& l_rote    = *route_ptr_;
  auto l_self_ptr = shared_from_this();
  // l_rote(request_parser_->get().method(), url_.segments(), l_self_ptr)->callback_(l_self_ptr);
}
void http_session_data::seed_error(
    boost::beast::http::status in_status, boost::system::error_code ec, const std::string& in_str
) {
  logger_->log(log_loc(), level::err, "发送错误码 {} {}", ec, in_str);

  boost::beast::http::response<boost::beast::http::string_body> l_response{in_status, version_};
  l_response.set(boost::beast::http::field::content_type, "plain/text");
  l_response.set(boost::beast::http::field::accept, "application/json");
  l_response.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_response.keep_alive(keep_alive_);
  l_response.body() = ec.message() + in_str;
  l_response.prepare_payload();
  seed(std::move(l_response));
}
void http_session_data::seed(boost::beast::http::message_generator in_message_generator) {
  logger_->log(log_loc(), level::err, "写入 {} {}", request_parser_->get().method(), url_);
  boost::beast::async_write(
      *stream_, std::move(in_message_generator),
      boost::beast::bind_front_handler(&http_session_data::do_send, shared_from_this())
  );
}
void http_session_data::do_send(boost::system::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    logger_->log(log_loc(), level::err, "发送错误码 {}", ec);
    do_close();
    return;
  }
  if (!keep_alive_) {
    do_close();
    return;
  }
  if (!stream_) {
    logger_->log(log_loc(), level::err, "发送错误码 stream 为空");
    do_close();
    return;
  }
  stream_->expires_after(30s);
  rend_head();
}
void http_session_data::do_close() {
  boost::system::error_code l_error_code{};
  if (stream_) {
    stream_->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_error_code);
    if (l_error_code) {
      logger_->log(log_loc(), level::err, "关闭 socket 失败 {}", l_error_code);
    }
  }
}

namespace detail {

boost::asio::awaitable<void> async_session(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr) {
  using executor_type    = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using endpoint_type    = boost::asio::ip::tcp::endpoint;
  using tcp_stream_type  = executor_type::as_default_on_t<boost::beast::tcp_stream>;
  using session_data_ptr = std::shared_ptr<session_data>;
  tcp_stream_type l_stream{std::move(in_socket)};
  l_stream.expires_after(30s);
  auto l_session = std::make_shared<session_data>();
  l_session->logger_ =
      g_logger_ctrl().make_log(fmt::format("{}_{}", "socket", SOCKET(l_stream.socket().native_handle())));
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
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    l_session->version_    = l_request_parser->get().version();
    l_session->keep_alive_ = l_request_parser->keep_alive();
    l_session->url_        = boost::url{l_request_parser->get().target()};

    l_session->logger_->log(
        log_loc(), level::info, "开始解析 url {} {}", l_request_parser->get().method(), l_session->url_
    );
    switch (l_request_parser->get().method()) {
      case boost::beast::http::verb::get:
      case boost::beast::http::verb::head:
      case boost::beast::http::verb::options:
        break;

      case boost::beast::http::verb::post:
      case boost::beast::http::verb::put:
      case boost::beast::http::verb::delete_:
      case boost::beast::http::verb::patch:
        l_request_parser_string = std::make_shared<boost::beast::http::request_parser<boost::beast::http::string_body>>(
            std::move(*l_request_parser)
        );
        co_await boost::beast::http::async_read(l_stream, buffer_, *l_request_parser_string);
        break;
      default:
        co_return;
    }
    // todo: 请求分发到对应的处理函数
    auto l_callback = (*in_route_ptr)(l_request_parser->get().method(), l_session->url_.segments(), l_session);
    auto l_gen      = co_await l_callback->callback_(l_session);

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
    if (l_ec_r) {
      l_session->logger_->log(log_loc(), level::err, "读取头部失败 {}", l_ec_r);
      l_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_ec_r);
      l_stream.close();
      co_return;
    }
    if (l_ec_w) {
      l_session->logger_->log(log_loc(), level::err, "发送错误码 {}", l_ec_w);
      l_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, l_ec_w);
      l_stream.close();
      co_return;
    }
  }
}

boost::beast::http::message_generator session_data::make_error_code_msg(
    boost::beast::http::status in_status, const boost::system::error_code& ec, const std::string& in_str
) {
  logger_->log(log_loc(), level::err, "发送错误码 {} {}", ec, in_str);

  boost::beast::http::response<boost::beast::http::string_body> l_response{in_status, version_};
  l_response.set(boost::beast::http::field::content_type, "plain/text");
  l_response.set(boost::beast::http::field::accept, "application/json");
  l_response.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_response.keep_alive(keep_alive_);
  l_response.body() = ec.message() + in_str;
  l_response.prepare_payload();
  return l_response;
}
}  // namespace detail

}  // namespace doodle::http