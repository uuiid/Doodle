//
// Created by TD on 2024/2/20.
//

#include "http_session_data.h"

#include <doodle_core/configure/static_value.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>

// ReSharper disable once CppUnusedIncludeDirective
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>
// ReSharper disable once CppUnusedIncludeDirective
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/multipart_body.h>
#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/core/http/zlib_deflate_file_body.h>
#include <doodle_lib/lib_warp/boost_fmt_error.h>
#include <doodle_lib/lib_warp/boost_fmt_url.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/exception.h>

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

#include "core/global_function.h"
#include "http_content_type.h"
#include <cryptopp/adler32.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
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
  if (!in_mine_type.empty()) in_res.set(boost::beast::http::field::content_type, in_mine_type);
  in_res.set(boost::beast::http::field::access_control_allow_origin, access_control_allow_origin_);
  in_res.set(boost::beast::http::field::access_control_allow_credentials, "true");
  in_res.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, PUT, PATCH, DELETE, OPTIONS");
  in_res.set(
      boost::beast::http::field::access_control_allow_headers,
      "Authorization, Origin, X-Requested-With, Content-Type, Accept"
  );
  in_res.set(boost::beast::http::field::date, fmt::format("{:%a, %d %b %Y %T} GMT", l_time));
  in_res.set(boost::beast::http::field::vary, "Cookie");
  if (timeout_ != doodle_config::g_timeout)
    in_res.set(
        boost::beast::http::field::keep_alive,
        fmt::format("timeout={}, max={}", timeout_.count(), doodle_config::g_max_keep_alive)
    );
}
template <typename T>
auto session_data::set_response_file_header(
    T& in_res, const FSys::path& in_path, const http_header_ctrl& in_http_header_ctrl
) {
  set_response_header(in_res, in_http_header_ctrl.mine_type_);
  auto l_time = chrono::floor<chrono::seconds>(chrono::system_clock::now());
  if (in_http_header_ctrl.has_cache_control_) {
    in_res.set(boost::beast::http::field::cache_control, "public,max-age=300");
    in_res.set(
        boost::beast::http::field::expires, fmt::format("{:%a, %d %b %Y %T} GMT", l_time + chrono::seconds{300})
    );
  }
  auto l_last_time =
      chrono::floor<chrono::seconds>(FSys::file_time_type::clock::to_utc(FSys::last_write_time(in_path)));
  in_res.set(boost::beast::http::field::last_modified, fmt::format("{:%a, %d %b %Y %T} GMT", l_last_time));
  in_res.set(boost::beast::http::field::etag, generate_etag(in_path));
  if (in_http_header_ctrl.is_attachment_) {
    in_res.set(
        boost::beast::http::field::content_disposition,
        fmt::format(
            "attachment; filename=\"{}\"", in_http_header_ctrl.attachment_filename_.empty()
                                               ? in_path.filename().string()
                                               : in_http_header_ctrl.attachment_filename_
        )
    );
  }
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
    stream_->expires_never();
    set_session();
    callback_ = (*route_ptr_)(method_verb_, url_.segments(), shared_from_this());
    logger_->info("请求 url {} {}", method_verb_, url_);

    std::unique_ptr<boost::beast::http::message_generator> l_gen{};
    try {
      // 解析发现是 websocket 后,会直接启动新的携程, 本次携程直接返回
      if (co_await parse_body()) co_return;
      l_gen = std::make_unique<boost::beast::http::message_generator>(
          make_message(co_await callback_->callback(shared_from_this()))
      );
    } catch (const http_request_error& e) {
      logger_->log(log_loc(), level::err, "回复错误 {}", e.what());
      l_gen = std::make_unique<boost::beast::http::message_generator>(make_error_code_msg(e.code_status_(), e.what()));
    } catch (const orm::sqlite_orm_exception& e) {
      logger_->log(log_loc(), level::err, "数据库错误 {} {}", e.code_status_(), e.what());
      l_gen = std::make_unique<boost::beast::http::message_generator>(make_error_code_msg(e.code_status_(), e.what()));
    } catch (const nlohmann::json::exception& e) {
      logger_->log(log_loc(), level::err, "请求体解析错误 {}", e.what());
      l_gen = std::make_unique<boost::beast::http::message_generator>(
          make_error_code_msg(boost::beast::http::status::bad_request, e.what())
      );
    } catch (...) {
      auto l_err_str = boost::current_exception_diagnostic_information();
      logger_->log(log_loc(), level::err, "回复错误 {}", l_err_str);
      SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_main_error(), l_err_str);
      l_gen = std::make_unique<boost::beast::http::message_generator>(
          make_error_code_msg(boost::beast::http::status::internal_server_error, l_err_str)
      );
    }
    if (!l_gen) co_return logger_->error("回复生成器为空 {}", url_);
    if (!stream_) co_return logger_->error("stream_ 已失效, 无法回复 {}", url_);

    logger_->info("回复 url {} {}", method_verb_, url_);
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
      timeout_ = std::min(
          std::chrono::seconds{
              boost::lexical_cast<std::int32_t>(l_value.substr(l_it + 8, l_value.find(',', l_it) - l_it - 8))
          },
          doodle_config::g_max_keep_alive
      );
    } else {
      timeout_ = doodle_config::g_timeout;  // 有 keep-alive 但无 timeout=，重置为默认值
    }
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
        co_await async_websocket_session();
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
            body_ = l_str.empty() ? nlohmann::json{} : nlohmann::json::parse(l_str);
          else
            body_ = l_str;
          break;
        }
        case content_type::multipart_form_data:
          co_await save_multipart_form_data_file();
          break;
        case content_type::unknown:
          break;
        default:
          co_await save_bode_file(extension_from_mime_type(content_type_));
          break;
      }

      break;
    default:
      break;
  }
  co_return l_result;
}

void session_data::async_run_detached() {
  // 特意不连接取消槽, 会在连接结束时关闭, 由于有些大文件传输,这是必须的, 环节要求
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

boost::beast::http::message_generator session_data::make_message(const http_body_ptr& in_body) {
  if (!in_body) return make_error_code_msg(boost::beast::http::status::internal_server_error, "回复体为空");

  auto l_status          = in_body->get_status();
  const auto l_mine_type = in_body->get_mine_type();
  auto const l_body_status =
      l_status == boost::beast::http::status::ok && method_verb_ == boost::beast::http::verb::post
          ? static_cast<boost::beast::http::status>(boost::beast::http::status::created)
          : l_status;

  if (auto const* l_json = dynamic_cast<const http_body_json_t*>(in_body.get())) {
    boost::beast::http::response<boost::beast::http::string_body> l_res{l_body_status, version_};
    set_response_header(l_res, l_mine_type);
    l_res.keep_alive(keep_alive_);
    l_res.body() = l_json->body_.dump();
    if (in_body->is_deflate() && is_deflate()) {
      l_res.set(boost::beast::http::field::content_encoding, "deflate");
      l_res.body() = zlib_compress(l_res.body());
    }
    l_res.prepare_payload();
    return l_res;
  }
  if (auto const* l_str = dynamic_cast<const http_body_string_t*>(in_body.get())) {
    boost::beast::http::response<boost::beast::http::string_body> l_res{l_body_status, version_};
    set_response_header(l_res, l_mine_type);
    l_res.keep_alive(keep_alive_);
    l_res.body() = l_str->body_;
    if (in_body->is_deflate() && is_deflate()) {
      l_res.set(boost::beast::http::field::content_encoding, "deflate");
      l_res.body() = zlib_compress(l_res.body());
    }
    l_res.prepare_payload();
    return l_res;
  }
  if (auto const* l_bin = dynamic_cast<const http_body_binary_t*>(in_body.get())) {
    boost::beast::http::response<boost::beast::http::vector_body<unsigned char>> l_res{l_status, version_};
    set_response_header(l_res, l_mine_type);
    l_res.keep_alive(keep_alive_);
    l_res.body().resize(l_bin->body_.size());
    for (std::size_t l_i = 0; l_i < l_bin->body_.size(); ++l_i)
      l_res.body()[l_i] = static_cast<unsigned char>(l_bin->body_[l_i]);
    l_res.prepare_payload();
    return l_res;
  }
  if (dynamic_cast<const http_body_empty_t*>(in_body.get())) {
    boost::beast::http::response<boost::beast::http::empty_body> l_res{l_status, version_};
    set_response_header(l_res, l_mine_type);
    l_res.keep_alive(keep_alive_);
    l_res.prepare_payload();
    return l_res;
  }
  if (auto const* l_err = dynamic_cast<const http_body_error_t*>(in_body.get())) {
    return make_error_code_msg(l_status, l_err->message_, l_err->code_);
  }
  if (auto const* l_file = dynamic_cast<const http_body_file_t*>(in_body.get())) {
    const auto& l_path = l_file->path_;
    http_header_ctrl l_ctrl{
        .status_              = l_status,
        .mine_type_           = l_mine_type,
        .has_cache_control_   = in_body->has_cache_control(),
        .is_deflate_          = in_body->is_deflate(),
        .is_attachment_       = in_body->is_attachment(),
        .attachment_filename_ = in_body->get_attachment_filename(),
    };
    if (!FSys::exists(l_path)) return make_error_code_msg(boost::beast::http::status::not_found, "文件不存在");

    if (l_file->range_mode_ == http_file_range_mode::head_only || method_verb_ == boost::beast::http::verb::head) {
      boost::beast::http::response<boost::beast::http::empty_body> l_res{boost::beast::http::status::ok, version_};
      set_response_file_header(l_res, l_path, l_ctrl);
      l_res.keep_alive(keep_alive_);
      l_res.prepare_payload();
      return l_res;
    }

    boost::system::error_code l_code{};
    if (l_file->use_deflate_body_) {
      boost::beast::http::response<zlib_deflate_file_body> l_res{boost::beast::http::status::ok, version_};
      l_res.body().open(l_path, std::ios::in | std::ios::binary, l_code);
      if (l_code)
        throw_exception(http_request_error{
            boost::beast::http::status::internal_server_error,
            fmt::format("无法打开文件 {} {}", l_path.generic_string(), l_code.message())
        });
      l_res.set(boost::beast::http::field::content_encoding, "deflate");
      set_response_file_header(l_res, l_path, l_ctrl);
      l_res.keep_alive(keep_alive_);
      l_res.prepare_payload();
      return l_res;
    }

    boost::beast::http::response<boost::beast::http::file_body> l_res{boost::beast::http::status::ok, version_};
    l_res.body().open(l_path.generic_string().c_str(), boost::beast::file_mode::scan, l_code);
    if (l_code)
      throw_exception(http_request_error{
          boost::beast::http::status::internal_server_error,
          fmt::format("无法打开文件 {} {}", l_path.generic_string(), l_code.message())
      });

    auto l_begin = l_file->range_begin_;
    if (l_file->range_mode_ == http_file_range_mode::partial) {
      l_res.body().seek(l_begin, l_code);
      if (l_code)
        throw_exception(http_request_error{
            boost::beast::http::status::internal_server_error,
            fmt::format("无法定位文件 {} {}", l_path.generic_string(), l_code.message())
        });
      l_res.result(boost::beast::http::status::partial_content);
      l_res.set(
          boost::beast::http::field::content_range,
          fmt::format("bytes {}-{}/{}", l_begin, FSys::file_size(l_path) - 1, FSys::file_size(l_path))
      );
    } else if (req_header_.find(boost::beast::http::field::range) != req_header_.end()) {
      if (auto l_range = req_header_.at(boost::beast::http::field::range);
          !l_range.empty() && l_range.starts_with("bytes=")) {
        l_begin = std::stoll(l_range.substr(6, l_range.find("-") - 6));
        l_res.body().seek(l_begin, l_code);
        if (l_code)
          throw_exception(http_request_error{
              boost::beast::http::status::internal_server_error,
              fmt::format("无法定位文件 {} {}", l_path.generic_string(), l_code.message())
          });
        l_res.result(boost::beast::http::status::partial_content);
        l_res.set(
            boost::beast::http::field::content_range,
            fmt::format("bytes {}-{}/{}", l_begin, FSys::file_size(l_path) - 1, FSys::file_size(l_path))
        );
      }
    }

    set_response_file_header(l_res, l_path, l_ctrl);
    l_res.keep_alive(keep_alive_);
    l_res.prepare_payload();
    return l_res;
  }

  throw_exception(http_request_error{boost::beast::http::status::internal_server_error, "未实现的回复体类型"});
}
boost::beast::http::message_generator session_data::make_msg(
    const FSys::path& in_path, const http_header_ctrl& in_http_header_ctrl
) {
  // if (is_deflate()) return make_file_deflate(in_path, mine_type);
  if (!FSys::exists(in_path)) return make_error_code_msg(boost::beast::http::status::not_found, "文件不存在");

  if (method_verb_ == boost::beast::http::verb::head) {
    boost::beast::http::response<boost::beast::http::empty_body> l_res{boost::beast::http::status::ok, version_};
    set_response_file_header(l_res, in_path, in_http_header_ctrl);
    l_res.keep_alive(keep_alive_);
    l_res.prepare_payload();
    return l_res;
  }

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

  set_response_file_header(l_res, in_path, in_http_header_ctrl);
  l_res.keep_alive(keep_alive_);
  l_res.prepare_payload();

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

std::string session_data::get_string() const {
  if (std::holds_alternative<std::string>(body_)) return std::get<std::string>(body_);
  throw_exception(http_request_error{boost::beast::http::status::bad_request, "body 不是字符串"});
}

session_data::~session_data() = default;

}  // namespace doodle::http::detail