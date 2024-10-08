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

#include <tl/expected.hpp>

namespace doodle::http {
namespace detail {
class async_session_t {
  static constexpr auto g_body_limit{500 * 1024 * 1024};  // 500M
  using executor_type              = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using tcp_stream_type            = executor_type::as_default_on_t<boost::beast::tcp_stream>;
  using tcp_stream_type_ptr        = std::shared_ptr<tcp_stream_type>;
  using empty_body_type            = boost::beast::http::empty_body;
  using empty_request_parser_type  = boost::beast::http::request_parser<empty_body_type>;
  using string_request_parser_type = boost::beast::http::request_parser<boost::beast::http::string_body>;
  using empty_request_parser_ptr   = std::shared_ptr<empty_request_parser_type>;
  using string_request_parser_ptr  = std::shared_ptr<string_request_parser_type>;

  std::shared_ptr<tcp_stream_type> stream_;
  std::shared_ptr<tcp_stream_type> proxy_relay_stream_;

  http_route_ptr route_ptr_;
  empty_request_parser_ptr request_parser_;
  string_request_parser_ptr string_request_parser_;
  std::shared_ptr<session_data> session_;

  http_function_ptr callback_;
  boost::system::error_code ec_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::verb method_verb_{};

 public:
  explicit async_session_t(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr)
      : stream_(std::make_shared<tcp_stream_type>(std::move(in_socket))),
        route_ptr_(std::move(in_route_ptr)),
        session_(std::make_shared<session_data>()) {
    session_->logger_ = std::make_shared<spdlog::async_logger>(
        fmt::format("{}_{}", "socket", SOCKET(stream_->socket().native_handle())),
        spdlog::sinks_init_list{
            g_logger_ctrl().rotating_file_sink_
#ifndef NDEBUG
            ,
            g_logger_ctrl().debug_sink_
#endif
        },
        spdlog::thread_pool()
    );
  }

 private:
  void do_close(const std::string& in_str) {
    session_->logger_->error("{} {}", in_str, ec_);
    if (stream_) {
      stream_->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec_);
      stream_->close();
    }
    if (proxy_relay_stream_) {
      proxy_relay_stream_->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec_);
      proxy_relay_stream_->close();
    }
  }
  void set_session() {
    auto& l_req           = request_parser_->get();
    session_->version_    = l_req.version();
    session_->keep_alive_ = l_req.keep_alive();
    session_->url_        = boost::url{l_req.target()};
    method_verb_          = l_req.method();
    callback_.reset();
  }

  template <bool isRequest, typename SyncWriteStream, typename SyncReadStream>
  boost::asio::awaitable<boost::system::error_code> async_relay(
      SyncWriteStream& in_sync_write_stream, SyncReadStream& in_sync_read_stream,
      boost::beast::http::parser<isRequest, boost::beast::http::buffer_body>& in_parser,
      boost::beast::http::serializer<isRequest, boost::beast::http::buffer_body>& in_serializer
  ) {
    char l_buffer[1024 * 4];
    boost::system::error_code l_ec{};
    // 异步读取, 并且写入
    do {
      if (!in_parser.is_done()) {
        // Set up the body for writing into our small buffer
        in_parser.get().body().data = l_buffer;
        in_parser.get().body().size = sizeof(l_buffer);
        std::tie(l_ec, std::ignore) = co_await boost::beast::http::async_read(in_sync_read_stream, buffer_, in_parser);

        // This error is returned when buffer_body uses up the buffer
        if (l_ec == boost::beast::http::error::need_buffer) l_ec = {};
        if (l_ec) {
          session_->logger_->error("无法读取请求体 {}", l_ec.message());
          co_return l_ec;
        }
        // Set up the body for reading.
        // This is how much was parsed:
        in_parser.get().body().size = sizeof(l_buffer) - in_parser.get().body().size;
        in_parser.get().body().data = l_buffer;
        in_parser.get().body().more = !in_parser.is_done();
      } else {
        in_parser.get().body().data = nullptr;
        in_parser.get().body().size = 0;
      }
      // Write everything in the buffer (which might be empty)
      std::tie(l_ec, std::ignore) = co_await boost::beast::http::async_write(in_sync_write_stream, in_serializer);
      // This error is returned when buffer_body uses up the buffer
      if (l_ec == boost::beast::http::error::need_buffer) l_ec = {};
      if (l_ec) {
        session_->logger_->error("无法发送请求体 {}", l_ec.message());
        co_return l_ec;
      }
    } while (!in_parser.is_done() && !in_serializer.is_done());

    co_return l_ec;
  }

  template <typename SyncWriteStream, typename SyncReadStream>
  boost::asio::awaitable<boost::system::error_code> async_relay_raw(
      SyncWriteStream& in_sync_write_stream, SyncReadStream& in_sync_read_stream
  ) {
    boost::system::error_code l_ec{};
    char l_buffer[1024 * 4];
    std::size_t l_bytes_transferred;
    do {
      std::tie(l_ec, l_bytes_transferred) =
          co_await boost::asio::async_read(in_sync_read_stream, boost::asio::buffer(l_buffer));
      if (l_ec == boost::beast::http::error::need_buffer) l_ec = {};
      if (l_ec) {
        session_->logger_->error("无法读取请求体 {}", l_ec.message());
        co_return l_ec;
      }

      std::tie(l_ec, l_bytes_transferred) =
          co_await boost::asio::async_write(in_sync_write_stream, boost::asio::buffer(l_buffer));
    } while (!l_ec);
    co_return l_ec;
  }

  boost::asio::awaitable<void> proxy_websocket_relay() {
    co_await proxy_http_relay();
    if (ec_) co_return;

    session_->logger_->warn("开始代理 websocket");
    auto& l_source_tcp = stream_->socket();
    auto& l_target_tcp = proxy_relay_stream_->socket();

    // auto&& [l_arr, l_ptr1, l_e1, l_ptr2, l_e2] =
    //     co_await boost::asio::experimental::make_parallel_group(
    //         boost::asio::co_spawn(
    //             l_source_tcp.get_executor(), async_relay_raw(l_target_tcp, l_source_tcp),
    //             boost::asio::deferred
    //         ),
    //         boost::asio::co_spawn(
    //             l_source_tcp.get_executor(), async_relay_raw(l_source_tcp, l_target_tcp),
    //             boost::asio::deferred
    //         )
    //     )
    //         .async_wait(
    //             boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable_t<>{})
    //         );
    // /// 直接转发原始数据
    // // co_await boost::beast::http::async_read_some(*in_source_stream, in_flat_buffer, *in_request_parser);
    // if (l_e1) l_ec = l_e1;
    // if (l_e2) l_ec = l_e2;
  }

  boost::asio::awaitable<void> proxy_http_relay() {
    using buffer_request_type                = boost::beast::http::request_parser<boost::beast::http::buffer_body>;
    using buffer_response_type               = boost::beast::http::response_parser<boost::beast::http::buffer_body>;
    using buffer_request_type_ptr            = std::shared_ptr<buffer_request_type>;
    using buffer_response_type_ptr           = std::shared_ptr<buffer_response_type>;

    using buffer_request_serializer_type     = boost::beast::http::request_serializer<boost::beast::http::buffer_body>;
    using buffer_request_serializer_type_ptr = std::shared_ptr<buffer_request_serializer_type>;

    using buffer_response_serializer_type    = boost::beast::http::response_serializer<boost::beast::http::buffer_body>;
    using buffer_response_serializer_type_ptr = std::shared_ptr<buffer_response_serializer_type>;
    // 开始转发请求
    {
      // 开始转发请求
      buffer_request_type_ptr l_request_parser{std::make_shared<buffer_request_type>(std::move(*request_parser_))};
      buffer_request_serializer_type_ptr l_request_serializer{
          std::make_shared<buffer_request_serializer_type>(l_request_parser->get())
      };
      std::tie(ec_, std::ignore) =
          co_await boost::beast::http::async_write_header(*proxy_relay_stream_, *l_request_serializer);
      proxy_relay_stream_->expires_after(30s);
      stream_->expires_after(30s);
      if (ec_) co_return session_->logger_->error("无法发送请求头 {}", ec_);

      // 异步读取, 并且写入
      ec_ = co_await async_relay<true>(*proxy_relay_stream_, *stream_, *l_request_parser, *l_request_serializer);
      if (ec_) co_return;
      proxy_relay_stream_->expires_after(30s);
      stream_->expires_after(30s);
    }

    {  // 开始转发回复
      buffer_response_type_ptr l_response_parser{std::make_shared<buffer_response_type>()};
      l_response_parser->body_limit(g_body_limit);
      buffer_response_serializer_type_ptr l_response_serializer{
          std::make_shared<buffer_response_serializer_type>(l_response_parser->get())
      };

      std::tie(ec_, std::ignore) =
          co_await boost::beast::http::async_read_header(*proxy_relay_stream_, buffer_, *l_response_parser);
      if (ec_) co_return session_->logger_->error("无法发送请求头 {}", ec_);
      proxy_relay_stream_->expires_after(30s);
      stream_->expires_after(30s);

      std::tie(ec_, std::ignore) = co_await boost::beast::http::async_write_header(*stream_, *l_response_serializer);
      if (ec_) co_return session_->logger_->error("无法发送回复头 {}", ec_);

      ec_ = co_await async_relay<false>(*stream_, *proxy_relay_stream_, *l_response_parser, *l_response_serializer);
      if (ec_) co_return;
      proxy_relay_stream_->expires_after(30s);
      stream_->expires_after(30s);
    }
    co_return;
  }

  boost::asio::awaitable<bool> proxy_run() {
    session_->logger_->info("转发请求 {} {}", request_parser_->get().method(), session_->url_);
    if (!proxy_relay_stream_) proxy_relay_stream_ = co_await route_ptr_->create_proxy_factory_();
    if (!proxy_relay_stream_) co_return do_close("代理打开失败"), true;

    if (boost::beast::websocket::is_upgrade(request_parser_->get()))
      co_await proxy_websocket_relay();
    else
      co_await proxy_http_relay();

    if (ec_) co_return do_close("代理失败"), true;
    // 读取下一次头
    request_parser_ = std::make_shared<boost::beast::http::request_parser<boost::beast::http::empty_body>>();
    request_parser_->body_limit(g_body_limit);
    stream_->expires_after(30s);
    std::tie(ec_, std::ignore) = co_await boost::beast::http::async_read_header(*stream_, buffer_, *request_parser_);
    if (ec_) co_return do_close("请求头读取失败"), true;
    stream_->expires_after(30s);
    co_return false;
  }
  boost::asio::awaitable<void> async_websocket_session() {
    boost::beast::get_lowest_layer(*stream_).expires_never();
    auto l_websocket_route = std::make_shared<websocket_route>();
    callback_->websocket_callback_(l_websocket_route);

    boost::beast::websocket::stream<tcp_stream_type> l_stream{std::move(*stream_)};
    stream_.reset();
    l_stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
    l_stream.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res
                                                                        ) {
      res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle-server");
    }));
    std::tie(ec_) = co_await l_stream.async_accept(request_parser_->get());
    if (ec_) {
      l_stream.close(ec_.value(), ec_);
      co_return do_close("无法接受请求");
    }

    auto l_c = std::make_shared<http_websocket_client>();
    l_c->init(std::move(l_stream), l_websocket_route, session_->logger_);
    co_await l_c->async_read_websocket();
    co_return;
  }

  boost::asio::awaitable<void> parse_body() {
    std::string l_content_type = request_parser_->get()[boost::beast::http::field::content_type];
    switch (method_verb_) {
      case boost::beast::http::verb::get:
        if (boost::beast::websocket::is_upgrade(request_parser_->get()) && callback_->has_websocket()) {
          co_return co_await async_websocket_session();
        }
      case boost::beast::http::verb::head:
      case boost::beast::http::verb::options:
        session_->req_header_ = std::move(request_parser_->release().base());
        break;

      case boost::beast::http::verb::post:
      case boost::beast::http::verb::put:
      case boost::beast::http::verb::delete_:
      case boost::beast::http::verb::patch:
        string_request_parser_ = std::make_shared<boost::beast::http::request_parser<boost::beast::http::string_body>>(
            std::move(*request_parser_)
        );
        std::tie(ec_, std::ignore) =
            co_await boost::beast::http::async_read(*stream_, buffer_, *string_request_parser_);
        if (ec_) co_return;

        stream_->expires_after(30s);

        if (l_content_type.find("application/json") != std::string::npos) {
          session_->content_type_ = content_type::application_json;
          try {
            session_->body_ = nlohmann::json::parse(string_request_parser_->get().body());
          } catch (const nlohmann::json::exception& e) {
            session_->logger_->log(log_loc(), level::err, "json 解析错误 {}", e.what());
            ec_ = error_enum::bad_json_string;
            co_return;
          }
        } else {
          session_->body_ = string_request_parser_->get().body();
        }
        session_->req_header_ = std::move(string_request_parser_->release().base());
        break;
      default:
        co_return;
    }
  }

 public:
  boost::asio::awaitable<void> run() {
    request_parser_ = std::make_shared<empty_request_parser_type>();
    request_parser_->body_limit(g_body_limit);
    std::tie(ec_, std::ignore) = co_await boost::beast::http::async_read_header(*stream_, buffer_, *request_parser_);
    if (ec_) co_return do_close("头读取错误");
    stream_->expires_after(30s);

    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      set_session();
      session_->logger_->info("开始解析 url {} {}", request_parser_->get().method(), session_->url_);
      callback_ = (*route_ptr_)(method_verb_, session_->url_.segments(), session_);
      if (!callback_) {
        if (co_await proxy_run()) co_return;
        continue;
      }
      co_await parse_body();
      auto l_gen = ec_ ? session_->make_error_code_msg(boost::beast::http::status::bad_request, ec_)
                       : co_await callback_->callback_(session_);

      if (!session_->keep_alive_) {
        std::tie(ec_, std::ignore) = co_await boost::beast::async_write(*stream_, std::move(l_gen));
        if (ec_) co_return do_close("发送错误");
      }
      // 初始化新的 parser
      request_parser_ = std::make_shared<boost::beast::http::request_parser<boost::beast::http::empty_body>>();
      request_parser_->body_limit(g_body_limit);
      auto [_, l_ec_r, l_sz_r, l_ec_w, l_sz_w] =
          co_await boost::asio::experimental::make_parallel_group(
              boost::beast::http::async_read_header(*stream_, buffer_, *request_parser_, boost::asio::deferred),
              boost::beast::async_write(*stream_, std::move(l_gen), boost::asio::deferred)
          )
              .async_wait(
                  boost::asio::experimental::wait_for_all(), boost::asio::as_tuple(boost::asio::use_awaitable_t<>{})
              );
      if (ec_ = l_ec_r; ec_) co_return do_close("读取头错误");
      if (ec_ = l_ec_w; ec_) co_return do_close("写入错误");
      stream_->expires_after(30s);
    }
  }
};
boost::asio::awaitable<void> async_session(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr) {
  auto l_ptr = std::make_shared<async_session_t>(std::move(in_socket), in_route_ptr);
  co_await l_ptr->run();
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