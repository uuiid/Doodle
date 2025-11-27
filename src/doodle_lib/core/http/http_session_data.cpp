//
// Created by TD on 2024/2/20.
//

#include "http_session_data.h"

#include "doodle_core/core/app_base.h"
#include "doodle_core/core/core_set.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "doodle_core/doodle_core_fwd.h"
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

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/cancellation_condition.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/winapi/error_codes.hpp>

#include "cryptopp/hex.h"
#include <cryptopp/adler32.h>
#include <cryptopp/filters.h>
#include <spdlog/spdlog.h>
#include <sqlite_orm/sqlite_orm.h>
#include <variant>
namespace doodle::http::detail {
namespace {

std::string generate_etag(const FSys::path& in_path) {
  auto l_time = FSys::file_time_type::clock::to_utc(FSys::last_write_time(in_path));
  std::string l_path_adler{};
  {
    CryptoPP::Adler32 l_adler;
    CryptoPP::StringSource l_string_source{
        in_path.generic_string(), true,
        new CryptoPP::HashFilter(l_adler, new CryptoPP::HexEncoder{new CryptoPP::StringSink{l_path_adler}})
    };
  }
  return fmt::format("\"{}-{}-{}\"", l_time.time_since_epoch().count(), FSys::file_size(in_path), l_path_adler);
}
}  // namespace

template <typename T>
auto session_data::set_response_header(T& in_res, std::string_view in_mine_type) {
  auto l_time = chrono::floor<chrono::seconds>(chrono::system_clock::now());
  in_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  in_res.set(boost::beast::http::field::content_type, in_mine_type);
  in_res.set(boost::beast::http::field::access_control_allow_origin, access_control_allow_origin_);
  in_res.set(boost::beast::http::field::access_control_allow_credentials, "true");
  in_res.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, PUT, PATCH, DELETE, OPTIONS");
  in_res.set(
      boost::beast::http::field::access_control_allow_headers,
      "Authorization, Origin, X-Requested-With, Content-Type, Accept"
  );
  in_res.set(boost::beast::http::field::date, fmt::format("{:%a, %d %b %Y %T} GMT", l_time));
  in_res.set(boost::beast::http::field::vary, "Origin");
}
template <typename T>
auto session_data::set_response_file_header(
    T& in_res, std::string_view in_mine_type, const FSys::path& in_path, bool has_cache_control
) {
  set_response_header(in_res, in_mine_type);
  auto l_time = chrono::floor<chrono::seconds>(chrono::system_clock::now());
  if (has_cache_control) {
    in_res.set(boost::beast::http::field::cache_control, "public,max-age=300");
    in_res.set(
        boost::beast::http::field::expires, fmt::format("{:%a, %d %b %Y %T} GMT", l_time + chrono::seconds{300})
    );
  }
  auto l_last_time =
      chrono::floor<chrono::seconds>(FSys::file_time_type::clock::to_utc(FSys::last_write_time(in_path)));
  in_res.set(boost::beast::http::field::last_modified, fmt::format("{:%a, %d %b %Y %T} GMT", l_last_time));
  in_res.set(boost::beast::http::field::etag, generate_etag(in_path));
}

session_data::session_data(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr)
    : stream_(std::make_unique<tcp_stream_type>(std::move(in_socket))),
      route_ptr_(std::move(in_route_ptr)),
      logger_(
          std::make_shared<spdlog::async_logger>(
              fmt::format("{}_{}", "socket", SOCKET(stream_->socket().native_handle())),
              spdlog::sinks_init_list{
                  g_logger_ctrl().rotating_file_sink_
#ifndef NDEBUG
                  ,
                  g_logger_ctrl().debug_sink_
#endif
              },
              spdlog::thread_pool()
          )
      ) {
}

boost::asio::awaitable<void> session_data::run() {
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    request_parser_ = std::make_shared<empty_request_parser_type>();
    std::visit([](auto&& in_ptr) { in_ptr->body_limit(g_body_limit); }, request_parser_);
    co_await std::visit(
        [this](auto&& in_ptr) { return boost::beast::http::async_read_header(*stream_, buffer_, *in_ptr); },
        request_parser_
    );
    stream_->expires_after(timeout_);
    set_session();
    callback_ = (*route_ptr_)(method_verb_, url_.segments(), shared_from_this());
    logger_->info("请求 url {} {} t:{}", method_verb_, url_, timeout_);
    // 解析发现是 websocket 后,会直接启动新的携程, 本次携程直接返回
    stream_->expires_after(timeout_);
    if (co_await parse_body()) co_return;

    std::unique_ptr<boost::beast::http::message_generator> l_gen{};
    try {
      stream_->expires_after(timeout_);
      l_gen = std::make_unique<boost::beast::http::message_generator>(co_await callback_->callback(shared_from_this()));
    } catch (const http_request_error& e) {
      logger_->log(log_loc(), level::err, "回复错误 {}", e.what());
      l_gen = std::make_unique<boost::beast::http::message_generator>(make_error_code_msg(e.code_status_, e.what()));
    } catch (...) {
      logger_->log(log_loc(), level::err, "回复错误 {}", boost::current_exception_diagnostic_information());
      l_gen = std::make_unique<boost::beast::http::message_generator>(make_error_code_msg(
          boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
      ));
    }
    if (!l_gen) co_return logger_->error("回复生成器为空 {}", url_);

    logger_->info("回复 url {} {}", method_verb_, url_);
    stream_->expires_after(timeout_);
    co_await boost::beast::async_write(*stream_, std::move(*l_gen));
    if (!keep_alive_) co_return;
    stream_->expires_after(timeout_);
  }
}

void session_data::set_session() {
  if (!std::visit([](auto&& in_ptr) { return in_ptr->is_header_done(); }, request_parser_))
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "请求解析失败, 请求头未完成解析"});

  std::visit(
      [this](auto&& in_ptr) {
        auto& l_req = in_ptr->get();
        version_    = l_req.version();
        keep_alive_ = in_ptr->keep_alive();
        url_        = boost::urls::parse_origin_form(l_req.target()).value();
        url_.normalize();
        method_verb_ = l_req.method();
        req_header_  = l_req.base();
      },
      request_parser_
  );
  if (auto l_keep_alive = req_header_.find(boost::beast::http::field::keep_alive); l_keep_alive != req_header_.end()) {
    auto l_value = l_keep_alive->value();
    if (auto l_it = l_value.find("timeout="); l_it != std::string_view::npos) {
      timeout_ = std::chrono::seconds{
          boost::lexical_cast<std::int64_t>(l_value.substr(l_it + 8, l_value.find(',', l_it) - l_it - 8))
      };
    }
    // SPDLOG_LOGGER_WARN(logger_, "超时设置 {}", timeout_);
  } else {
    timeout_ = doodle_config::g_timeout;
  }

  callback_.reset();
  access_control_allow_origin_ = "*";
}
boost::asio::awaitable<void> session_data::save_bode_file(const std::string& in_ext) {
  auto l_file_request_parser_ = std::make_shared<boost::beast::http::request_parser<boost::beast::http::file_body>>(
      std::move(*std::get<empty_request_parser_ptr>(request_parser_))
  );
  auto l_path = core_set::get_set().get_cache_root("http") / (core_set::get_set().get_uuid_str() + in_ext);
  boost::system::error_code l_ec_;
  l_file_request_parser_->get().body().open(l_path.generic_string().c_str(), boost::beast::file_mode::write, l_ec_);
  if (l_ec_) throw_exception(http_request_error{boost::beast::http::status::internal_server_error, l_ec_.message()});
  co_await boost::beast::http::async_read(*stream_, buffer_, *l_file_request_parser_);
  body_ = l_path;
  co_return;
}
boost::asio::awaitable<void> session_data::save_multipart_form_data_file() {
  auto l_multipart_request_parser_ = std::make_shared<boost::beast::http::request_parser<doodle::http::multipart_body>>(
      std::move(*std::get<empty_request_parser_ptr>(request_parser_))
  );
  co_await boost::beast::http::async_read(*stream_, buffer_, *l_multipart_request_parser_);
  body_ = l_multipart_request_parser_->get().body();
}
boost::asio::awaitable<void> session_data::async_websocket_session() {
  boost::beast::get_lowest_layer(*stream_).expires_never();
  callback_->websocket_init(shared_from_this());

  boost::beast::websocket::stream<tcp_stream_type> l_stream{std::move(*stream_)};
  stream_.reset();
  l_stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  l_stream.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
    res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle-server");
  }));
  co_await l_stream.async_accept(std::get<empty_request_parser_ptr>(request_parser_)->get());

  callback_->websocket_callback(std::move(l_stream), shared_from_this());
  co_return;
}

boost::asio::awaitable<bool> session_data::parse_body() {
  bool l_result{};
  switch (method_verb_) {
    case boost::beast::http::verb::get:
      if (boost::beast::websocket::is_upgrade(std::get<empty_request_parser_ptr>(request_parser_)->get()) &&
          callback_->has_websocket()) {
        boost::asio::co_spawn(
            stream_->get_executor(), async_websocket_session(),
            boost::asio::consign(boost::asio::detached, shared_from_this())
        );
        l_result = true;
      }
      break;
    case boost::beast::http::verb::head:
    case boost::beast::http::verb::options:
      break;

    case boost::beast::http::verb::post:
    case boost::beast::http::verb::put:
    case boost::beast::http::verb::delete_:
    case boost::beast::http::verb::patch:
      content_type_ = get_content_type(req_header_[boost::beast::http::field::content_type]);
      switch (content_type_) {
        case content_type::image_jpeg:
        case content_type::image_jpg:
        case content_type::image_png:
        case content_type::text_plain:
        case content_type::application_json: {
          request_parser_ = std::make_shared<boost::beast::http::request_parser<boost::beast::http::string_body>>(
              std::move(*std::get<empty_request_parser_ptr>(request_parser_))
          );

          co_await std::visit(
              [this](auto&& in_ptr) { return boost::beast::http::async_read(*stream_, buffer_, *in_ptr); },
              request_parser_
          );
          const auto& l_str = std::get<string_request_parser_ptr>(request_parser_)->get().body();
          if (content_type_ == content_type::application_json)
            body_ = nlohmann::json::parse(l_str);
          else
            body_ = l_str;
          break;
        }
        case content_type::image_gif:
          co_await save_bode_file(".gif");
          break;
        case content_type::video_mp4:
          co_await save_bode_file(".mp4");
          break;
        case content_type::application_nuknown:
          co_await save_bode_file(".tmp");
          break;
        case content_type::multipart_form_data:
          co_await save_multipart_form_data_file();
          break;
        case content_type::unknown:
          break;
        default:
          break;
      }

      break;
    default:
      break;
  }
  co_return l_result;
}

void session_data::async_run_detached() {
  boost::asio::co_spawn(g_io_context(), run(), boost::asio::consign(boost::asio::detached, shared_from_this()));
}

boost::beast::http::message_generator session_data::make_error_code_msg(
    boost::beast::http::status in_code, const std::string& in_str, std::int32_t in_msg_code
) {
  logger_->log(log_loc(), level::err, "发送错误码 {} {}", in_code, in_str);
  if (in_msg_code == -1) in_msg_code = enum_to_num(in_code);

  boost::beast::http::response<boost::beast::http::string_body> l_response{in_code, version_};
  set_response_header(l_response, "application/json; charset=utf-8");
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
    const FSys::path& in_path, const http_header_ctrl& in_http_header_ctrl
) {
  // if (is_deflate()) return make_file_deflate(in_path, mine_type);
  if (!FSys::exists(in_path)) return make_error_code_msg(boost::beast::http::status::not_found, "文件不存在");
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
  if (req_header_.find(boost::beast::http::field::range) != req_header_.end())
    if (auto l_range = req_header_.at(boost::beast::http::field::range);
        !l_range.empty() && l_range.starts_with("bytes=")) {
      auto l_begin = std::stoll(l_range.substr(6, l_range.find("-") - 6));
      // auto l_end   = std::stoll(l_range.substr(l_range.find("-") + 1));
      l_res.body().seek(l_begin, l_code);
      if (l_code)
        throw_exception(
            http_request_error{
                boost::beast::http::status::internal_server_error,
                fmt::format("无法定位文件 {} {}", in_path.generic_string(), l_code.message())
            }
        );
      // l_res.body().next(l_end - l_begin + 1, l_code);
      l_res.result(boost::beast::http::status::partial_content);

      l_res.set(
          boost::beast::http::field::content_range,
          fmt::format("bytes {}-{}/{}", l_begin, FSys::file_size(in_path) - 1, FSys::file_size(in_path))
      );
    }

  set_response_file_header(l_res, in_http_header_ctrl.mine_type_, in_path, in_http_header_ctrl.has_cache_control_);

  l_res.keep_alive(keep_alive_);
  l_res.prepare_payload();

  if (auto l_size = FSys::file_size(in_path); l_size > 1024 * 1024 * 1024)  // 1GB
  {
    timeout_ += chrono::seconds(l_size / (20 * 1024 * 1024));
    l_res.set(boost::beast::http::field::keep_alive, fmt::format("timeout={}", timeout_.count()));
  }
  return l_res;
}
template <>
boost::beast::http::response<boost::beast::http::vector_body<unsigned char>> session_data::make_msg(
    std::vector<unsigned char>&& in_body, const http_header_ctrl& in_http_header_ctrl
) {
  boost::beast::http::response<boost::beast::http::vector_body<unsigned char>> l_res{
      in_http_header_ctrl.status_, version_
  };
  set_response_header(l_res, in_http_header_ctrl.mine_type_);
  l_res.keep_alive(keep_alive_);
  l_res.body() = std::move(in_body);
  l_res.prepare_payload();
  return l_res;
}
template <>
boost::beast::http::response<boost::beast::http::vector_body<char>> session_data::make_msg(
    std::vector<char>&& in_body, const http_header_ctrl& in_http_header_ctrl
) {
  boost::beast::http::response<boost::beast::http::vector_body<char>> l_res{in_http_header_ctrl.status_, version_};
  set_response_header(l_res, in_http_header_ctrl.mine_type_);
  l_res.keep_alive(keep_alive_);
  l_res.body() = std::move(in_body);
  l_res.prepare_payload();
  return l_res;
}

// boost::beast::http::response<zlib_deflate_file_body> session_data::make_file_deflate(
//     const FSys::path& in_path, const std::string_view& mine_type, bool has_cache_control
// ) {
//   if (!FSys::exists(in_path)) throw_exception(http_request_error{boost::beast::http::status::not_found,
//   "文件不存在"}); boost::system::error_code l_code{}; boost::beast::http::response<zlib_deflate_file_body>
//   l_res{boost::beast::http::status::ok, version_}; l_res.body().open(in_path, std::ios::in | std::ios::binary,
//   l_code); if (l_code)
//     throw_exception(
//         http_request_error{
//             boost::beast::http::status::internal_server_error,
//             fmt::format("无法打开文件 {} {}", in_path.generic_string(), l_code.message())
//         }
//     );
//   set_response_file_header(l_res, mine_type, in_path, has_cache_control);
//   l_res.keep_alive(keep_alive_);
//   l_res.prepare_payload();
//   return l_res;
// }

boost::beast::http::response<boost::beast::http::string_body> session_data::make_msg(
    std::string&& in_body, const http_header_ctrl& in_http_header_ctrl
) {
  boost::beast::http::response<boost::beast::http::string_body> l_res{in_http_header_ctrl.status_, version_};
  set_response_header(l_res, in_http_header_ctrl.mine_type_);
  if (in_http_header_ctrl.is_deflate_) l_res.set(boost::beast::http::field::content_encoding, "deflate");
  l_res.keep_alive(keep_alive_);
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
  if (std::holds_alternative<FSys::path>(body_)) return {std::get<FSys::path>(body_)};
  return {};
}
FSys::path session_data::get_file() const {
  if (std::holds_alternative<FSys::path>(body_)) return std::get<FSys::path>(body_);
  if (std::holds_alternative<multipart_body::value_type>(body_) &&
      !std::get<multipart_body::value_type>(body_).get_files().empty())
    return std::get<multipart_body::value_type>(body_).get_files().front();
  return {};
}

}  // namespace doodle::http::detail