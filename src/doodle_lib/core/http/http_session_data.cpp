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
#include <boost/winapi/error_codes.hpp>

#include "core/global_function.h"
#include "http_content_type.h"
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <variant>

namespace doodle::http::detail {

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
      l_gen = std::make_unique<boost::beast::http::message_generator>(http_body_base_t::make_error_code_msg(
          e.code_status_(), e.what(), version_, keep_alive_, access_control_allow_origin_, timeout_
      ));
    } catch (const orm::sqlite_orm_exception& e) {
      logger_->log(log_loc(), level::err, "数据库错误 {} {}", e.code_status_(), e.what());
      l_gen = std::make_unique<boost::beast::http::message_generator>(http_body_base_t::make_error_code_msg(
          e.code_status_(), e.what(), version_, keep_alive_, access_control_allow_origin_, timeout_
      ));
    } catch (const nlohmann::json::exception& e) {
      logger_->log(log_loc(), level::err, "请求体解析错误 {}", e.what());
      l_gen = std::make_unique<boost::beast::http::message_generator>(http_body_base_t::make_error_code_msg(
          boost::beast::http::status::bad_request, e.what(), version_, keep_alive_, access_control_allow_origin_,
          timeout_
      ));
    } catch (...) {
      auto l_err_str = boost::current_exception_diagnostic_information();
      logger_->log(log_loc(), level::err, "回复错误 {}", l_err_str);
      SPDLOG_LOGGER_ERROR(g_logger_ctrl().get_main_error(), l_err_str);
      l_gen = std::make_unique<boost::beast::http::message_generator>(http_body_base_t::make_error_code_msg(
          boost::beast::http::status::internal_server_error, l_err_str, version_, keep_alive_,
          access_control_allow_origin_, timeout_
      ));
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

boost::beast::http::message_generator session_data::make_message(const http_body_ptr& in_body) const {
  if (!in_body)
    return http_body_base_t::make_error_code_msg(
        boost::beast::http::status::internal_server_error, "回复体为空", version_, keep_alive_,
        access_control_allow_origin_, timeout_
    );
  return in_body->make_message(*this);
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
