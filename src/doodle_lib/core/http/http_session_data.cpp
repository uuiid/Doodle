//
// Created by TD on 2024/2/20.
//

#include "http_session_data.h"

#include "doodle_core/core/core_set.h"
// ReSharper disable once CppUnusedIncludeDirective
#include <doodle_core/lib_warp/boost_fmt_beast.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/lib_warp/boost_fmt_url.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_websocket_client.h>
#include <doodle_lib/core/http/multipart_body.h>
#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/core/http/zlib_deflate_file_body.h>

#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include <sqlite_orm/sqlite_orm.h>
#include <tl/expected.hpp>
namespace doodle::http {
namespace detail {

class async_session_t : public std::enable_shared_from_this<async_session_t> {
  static constexpr auto g_body_limit{500 * 1024 * 1024};  // 500M
  using executor_type                 = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using tcp_stream_type               = executor_type::as_default_on_t<boost::beast::tcp_stream>;
  using tcp_stream_type_ptr           = std::shared_ptr<tcp_stream_type>;
  using empty_body_type               = boost::beast::http::empty_body;
  using empty_request_parser_type     = boost::beast::http::request_parser<empty_body_type>;
  using string_request_parser_type    = boost::beast::http::request_parser<boost::beast::http::string_body>;
  using file_request_parser_type      = boost::beast::http::request_parser<boost::beast::http::file_body>;
  using multipart_request_parser_type = boost::beast::http::request_parser<doodle::http::multipart_body>;

  using empty_request_parser_ptr      = std::shared_ptr<empty_request_parser_type>;
  using string_request_parser_ptr     = std::shared_ptr<string_request_parser_type>;
  using file_request_parser_ptr       = std::shared_ptr<file_request_parser_type>;
  using multipart_request_parser_ptr  = std::shared_ptr<multipart_request_parser_type>;

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
  void do_close(const std::string& in_str = {}) {
    if (ec_) session_->logger_->error("{} {}", in_str, ec_);
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

  boost::asio::awaitable<void> close_websocket(
      boost::beast::websocket::stream<tcp_stream_type>& in_stream, const std::string& in_str
  ) {
    session_->logger_->error("{} {}", in_str, ec_);
    std::tie(ec_) = co_await in_stream.async_close(boost::beast::websocket::close_code::normal);
    if (ec_) session_->logger_->error("错误的关闭 {}", ec_);
  }

  boost::asio::awaitable<void> async_websocket_session() {
    boost::beast::get_lowest_layer(*stream_).expires_never();
    callback_->websocket_init(session_);

    boost::beast::websocket::stream<tcp_stream_type> l_stream{std::move(*stream_)};
    stream_.reset();
    l_stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
    l_stream.set_option(
        boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
          res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle-server");
        })
    );
    std::tie(ec_) = co_await l_stream.async_accept(request_parser_->get());
    if (ec_) {
      l_stream.close(ec_.value(), ec_);
      co_return do_close("无法接受请求");
    }
    co_await callback_->websocket_callback(std::move(l_stream), session_);
    co_return;
  }

  boost::asio::awaitable<bool> save_bode_file(const std::string& in_ext) {
    auto l_file_request_parser_ =
        std::make_shared<boost::beast::http::request_parser<boost::beast::http::file_body>>(std::move(*request_parser_)
        );
    auto l_path = core_set::get_set().get_cache_root("http") / (core_set::get_set().get_uuid_str() + in_ext);
    l_file_request_parser_->get().body().open(l_path.generic_string().c_str(), boost::beast::file_mode::write, ec_);
    if (ec_) co_return false;
    std::tie(ec_, std::ignore) = co_await boost::beast::http::async_read(*stream_, buffer_, *l_file_request_parser_);
    if (ec_) co_return false;
    session_->body_       = l_path;
    session_->req_header_ = std::move(l_file_request_parser_->release().base());
    co_return true;
  }

  boost::asio::awaitable<bool> save_multipart_form_data_file() {
    auto l_multipart_request_parser_ =
        std::make_shared<boost::beast::http::request_parser<doodle::http::multipart_body>>(std::move(*request_parser_));
    std::tie(ec_, std::ignore) =
        co_await boost::beast::http::async_read(*stream_, buffer_, *l_multipart_request_parser_);
    if (ec_) co_return false;
    session_->body_       = l_multipart_request_parser_->get().body();
    session_->req_header_ = std::move(l_multipart_request_parser_->release().base());
    co_return true;
  }

  boost::asio::awaitable<bool> parse_body() {
    std::string l_content_type = request_parser_->get()[boost::beast::http::field::content_type];
    switch (method_verb_) {
      case boost::beast::http::verb::get:
        if (boost::beast::websocket::is_upgrade(request_parser_->get()) && callback_->has_websocket()) {
          co_return boost::asio::co_spawn(
              stream_->get_executor(), async_websocket_session(),
              boost::asio::consign(boost::asio::detached, shared_from_this())
          ),
              true;
        }
      case boost::beast::http::verb::head:
      case boost::beast::http::verb::options:
        session_->req_header_ = std::move(request_parser_->release().base());
        break;

      case boost::beast::http::verb::post:
      case boost::beast::http::verb::put:
      case boost::beast::http::verb::delete_:
      case boost::beast::http::verb::patch:
        session_->content_type_ = get_content_type(request_parser_->get()[boost::beast::http::field::content_type]);
        session_->req_header_   = request_parser_->get().base();
        switch (session_->content_type_) {
          case content_type::image_jpeg:
          case content_type::image_jpg:
          case content_type::image_png:
          case content_type::text_plain:
          case content_type::application_json: {
            string_request_parser_ =
                std::make_shared<boost::beast::http::request_parser<boost::beast::http::string_body>>(
                    std::move(*request_parser_)
                );
            std::tie(ec_, std::ignore) =
                co_await boost::beast::http::async_read(*stream_, buffer_, *string_request_parser_);
            if (ec_) co_return false;
            break;
          }
          case content_type::image_gif:
            if (!co_await save_bode_file(".gif")) co_return false;
            break;
          case content_type::video_mp4:
            if (!co_await save_bode_file(".mp4")) co_return false;
            break;
          case content_type::application_nuknown:
            if (!co_await save_bode_file(".tmp")) co_return false;
            break;
          case content_type::multipart_form_data:
            if (!co_await save_multipart_form_data_file()) co_return false;
            break;
          case content_type::unknown:
            break;
          default:
            break;
        }
        stream_->expires_after(30s);

        switch (session_->content_type_) {
          case content_type::image_jpeg:
          case content_type::image_jpg:
          case content_type::text_plain:
          case content_type::image_png:
            session_->body_ = string_request_parser_->get().body();

            break;
          case content_type::application_json:
            try {
              session_->body_ = nlohmann::json::parse(string_request_parser_->get().body());
            } catch (const nlohmann::json::exception& e) {
              session_->logger_->log(log_loc(), level::err, "json 解析错误 {}", e.what());
              ec_ = error_enum::bad_json_string;
              co_return false;
            }
            break;
          case content_type::image_gif:
            break;
          default:
            break;
        }

        break;
      default:
        co_return false;
    }
    co_return false;
  }

 public:
  boost::asio::awaitable<void> run() {
    request_parser_ = std::make_shared<empty_request_parser_type>();
    request_parser_->body_limit(g_body_limit);
    std::tie(ec_, std::ignore) = co_await boost::beast::http::async_read_header(*stream_, buffer_, *request_parser_);
    if (ec_) co_return do_close("头读取错误_1");
    stream_->expires_after(30s);

    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      set_session();
      callback_ = (*route_ptr_)(method_verb_, session_->url_.segments(), session_);
      session_->logger_->info("开始解析 url {} {}", request_parser_->get().method(), session_->url_);
      // 解析发现是 websocket 后,会直接启动新的携程, 本次携程直接返回
      if (co_await parse_body()) co_return;

      std::unique_ptr<boost::beast::http::message_generator> l_gen{};
      if (ec_)
        l_gen = std::make_unique<boost::beast::http::message_generator>(
            session_->make_error_code_msg(boost::beast::http::status::bad_request, ec_)
        );
      else {
        try {
          l_gen = std::make_unique<boost::beast::http::message_generator>(co_await callback_->callback(session_));
        } catch (const http_request_error& e) {
          session_->logger_->log(log_loc(), level::err, "回复错误 {}", e.what());
          l_gen = std::make_unique<boost::beast::http::message_generator>(
              session_->make_error_code_msg(e.code_status_, e.what())
          );
        } catch (const boost::bad_lexical_cast& e) {
          session_->logger_->log(log_loc(), level::err, "回复错误 {}", e.what());
          l_gen = std::make_unique<boost::beast::http::message_generator>(
              session_->make_error_code_msg(boost::beast::http::status::bad_request, e.what())
          );
        } catch (const nlohmann::json::exception& e) {
          session_->logger_->log(log_loc(), level::err, "回复错误 {}", e.what());
          l_gen = std::make_unique<boost::beast::http::message_generator>(
              session_->make_error_code_msg(boost::beast::http::status::bad_request, e.what())
          );
        } catch (const std::system_error& e) {  // 暂时是捕获sql错的
          session_->logger_->log(log_loc(), level::err, "回复错误 {}", e.what());
          l_gen = std::make_unique<boost::beast::http::message_generator>(
              session_->make_error_code_msg(boost::beast::http::status::internal_server_error, e.what())
          );
        } catch (...) {
          session_->logger_->log(
              log_loc(), level::err, "回复错误 {}", boost::current_exception_diagnostic_information()
          );
          l_gen = std::make_unique<boost::beast::http::message_generator>(session_->make_error_code_msg(
              boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
          ));
        }
      }

      session_->logger_->info("回复 url {} {}", request_parser_->get().method(), session_->url_);
      if (!session_->keep_alive_) {
        std::tie(ec_, std::ignore) = co_await boost::beast::async_write(*stream_, std::move(*l_gen));
        if (ec_) co_return do_close("发送错误");
        co_return do_close();
      }
      // 初始化新的 parser
      request_parser_ = std::make_shared<boost::beast::http::request_parser<boost::beast::http::empty_body>>();
      request_parser_->body_limit(g_body_limit);
      auto [_, l_ec_r, l_sz_r, l_ec_w, l_sz_w] =
          co_await boost::asio::experimental::make_parallel_group(
              boost::beast::http::async_read_header(*stream_, buffer_, *request_parser_, boost::asio::deferred),
              boost::beast::async_write(*stream_, std::move(*l_gen), boost::asio::deferred)
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
  l_response.set(boost::beast::http::field::content_type, "application/json; charset=utf-8");
  l_response.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_response.set(boost::beast::http::field::access_control_allow_credentials, "true");
  l_response.set(boost::beast::http::field::access_control_allow_methods, "*");
  l_response.set(boost::beast::http::field::access_control_allow_headers, "*");
  l_response.keep_alive(keep_alive_);
  l_response.body() = nlohmann::json{{"error", in_str}, {"code", in_msg_code}}.dump();
  l_response.prepare_payload();
  return l_response;
}

std::string session_data::zlib_compress(const std::string& in_str) {
  std::stringstream compressed{};
  std::stringstream original{in_str};
  boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
  out.push(boost::iostreams::zlib_compressor{});
  out.push(original);
  boost::iostreams::copy(out, compressed);
  return compressed.str();
}
boost::beast::http::message_generator session_data::make_msg(
    const FSys::path& in_path, const std::string_view& mine_type
) {
  // if (is_deflate()) return make_file_deflate(in_path, mine_type);
  if (!FSys::exists(in_path)) return make_error_code_msg(boost::beast::http::status::not_found, "文件不存在");
  return make_file(in_path, mine_type);
}

boost::beast::http::response<boost::beast::http::file_body> session_data::make_file(
    const FSys::path& in_path, const std::string_view& mine_type
) {
  if (!FSys::exists(in_path)) throw_exception(http_request_error{boost::beast::http::status::not_found, "文件不存在"});
  boost::system::error_code l_code{};
  boost::beast::http::response<boost::beast::http::file_body> l_res{boost::beast::http::status::ok, version_};
  l_res.body().open(in_path.generic_string().c_str(), boost::beast::file_mode::scan, l_code);
  if (l_code)
    throw_exception(
        http_request_error{
            boost::beast::http::status::internal_server_error,
            fmt::format("无法打开文件 {} {}", in_path.generic_string(), l_code.message())
        }
    );
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, mine_type);
  l_res.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_res.set(boost::beast::http::field::access_control_allow_credentials, "true");
  l_res.set(boost::beast::http::field::access_control_allow_methods, "*");
  l_res.set(boost::beast::http::field::access_control_allow_headers, "*");
  l_res.keep_alive(keep_alive_);
  l_res.prepare_payload();
  return l_res;
}
boost::beast::http::response<zlib_deflate_file_body> session_data::make_file_deflate(
    const FSys::path& in_path, const std::string_view& mine_type
) {
  if (!FSys::exists(in_path)) throw_exception(http_request_error{boost::beast::http::status::not_found, "文件不存在"});
  boost::system::error_code l_code{};
  boost::beast::http::response<zlib_deflate_file_body> l_res{boost::beast::http::status::ok, version_};
  l_res.body().open(in_path, std::ios::in | std::ios::binary, l_code);
  if (l_code)
    throw_exception(
        http_request_error{
            boost::beast::http::status::internal_server_error,
            fmt::format("无法打开文件 {} {}", in_path.generic_string(), l_code.message())
        }
    );
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, mine_type);
  l_res.set(boost::beast::http::field::content_encoding, "deflate");
  l_res.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_res.set(boost::beast::http::field::access_control_allow_credentials, "true");
  l_res.set(boost::beast::http::field::access_control_allow_methods, "*");
  l_res.set(boost::beast::http::field::access_control_allow_headers, "*");
  l_res.keep_alive(keep_alive_);
  l_res.prepare_payload();
  return l_res;
}

boost::beast::http::response<boost::beast::http::string_body> session_data::make_msg(
    std::string&& in_body, const std::string_view& mine_type, boost::beast::http::status in_status
) {
  boost::beast::http::response<boost::beast::http::string_body> l_res{in_status, version_};
  l_res.set(boost::beast::http::field::content_type, mine_type);
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_res.set(boost::beast::http::field::access_control_allow_credentials, "true");
  l_res.set(boost::beast::http::field::access_control_allow_methods, "*");
  l_res.set(boost::beast::http::field::access_control_allow_headers, "*");
  l_res.keep_alive(keep_alive_);
  // if (req_header_[boost::beast::http::field::accept_encoding].contains("deflate")) {
  //   l_res.body() = zlib_compress(std::move(in_body));
  //   l_res.set(boost::beast::http::field::content_encoding, "deflate");
  // } else
  l_res.body() = std::move(in_body);

  l_res.prepare_payload();
  return l_res;
}
nlohmann::json session_data::get_json() const {
  if (content_type_ == content_type::application_json && std::holds_alternative<nlohmann::json>(body_))
    return std::get<nlohmann::json>(body_);
  if (content_type_ == content_type::multipart_form_data && std::holds_alternative<multipart_body::value_type>(body_)) {
    return std::get<multipart_body::value_type>(body_).to_json();
  }
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "body 不是 json"});
}
std::vector<FSys::path> session_data::get_files() const {
  if (content_type_ == content_type::multipart_form_data && std::holds_alternative<multipart_body::value_type>(body_))
    return std::get<multipart_body::value_type>(body_).get_files();
  return {};
}

}  // namespace detail
}  // namespace doodle::http