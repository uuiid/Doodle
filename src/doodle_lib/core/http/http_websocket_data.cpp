//
// Created by TD on 2024/2/20.
//

#include "http_websocket_data.h"

#include "doodle_core/lib_warp/boost_fmt_asio.h"
#include "doodle_core/lib_warp/boost_fmt_error.h"
#include <doodle_core/metadata/computer.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include "socket_logger.h"
namespace doodle::http {
void http_websocket_data::run() {
  auto l_self_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_logger      = l_self_handle.get<socket_logger>().logger_;
  auto& l_data       = l_self_handle.get<session::async_read_body<boost::beast::http::string_body>>();
  l_logger->log(
      log_loc(), level::info, "开始处理请求 {} {}", l_data.request_parser_->get().body(),
      l_data.request_parser_->get().target()
  );

  l_self_handle.emplace<doodle::computer>("", std::string{l_data.request_parser_->get().target()});
  stream_->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  stream_->set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
    res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  }));
  stream_->async_accept(l_data.request_parser_->get(), [l_logger, this](boost::system::error_code ec) {
    if (ec) {
      l_logger->log(log_loc(), level::err, "async_accept error: {}", ec);
      return;
    }
    do_read();
  });
}

void http_websocket_data::run_fun() {
  if (read_queue_.empty()) return;
  auto l_self_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_logger      = l_self_handle.get<socket_logger>().logger_;

  auto l_json        = nlohmann::json::parse(read_queue_.front());
  read_queue_.pop();

  if (l_json.contains("type") && l_json["type"] == "ping") {
    l_logger->log(log_loc(), level::info, "ping");
    if (l_json.contains("name")) {
      l_self_handle.get<doodle::computer>().name_ = l_json["name"];
    }
    write_queue_.emplace(l_json.dump());
    do_write();
  }
}

void http_websocket_data::do_read() {
  auto l_self_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_logger      = l_self_handle.get<socket_logger>().logger_;

  stream_->async_read(buffer_, [l_logger, this](boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
      if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::operation_aborted) {
        do_destroy();
        return;
      }
      l_logger->log(log_loc(), level::err, "async_read error: {}", ec);
      return;
    }
    l_logger->log(log_loc(), level::info, "async_read success: {}", bytes_transferred);
    read_queue_.emplace(boost::beast::buffers_to_string(buffer_.data()));
    buffer_.consume(buffer_.size());
    do_read();
    run_fun();
  });
}
void http_websocket_data::do_write() {
  auto l_self_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_logger      = l_self_handle.get<socket_logger>().logger_;
  if (write_queue_.empty()) return;
  stream_->async_write(
      boost::asio::buffer(write_queue_.front()),
      [l_logger, this](boost::system::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
          if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::operation_aborted) {
            do_destroy();
            return;
          }
          l_logger->log(log_loc(), level::err, "async_write error: {}", ec);
          return;
        }
        l_logger->log(log_loc(), level::info, "async_write success: {}", bytes_transferred);
        write_queue_.pop();
        do_write();
      }
  );
}
void http_websocket_data::do_destroy() {
  boost::asio::post(g_io_context(), [l_self_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)}] {
    auto l = l_self_handle;
    if (l) l.destroy();
  });
}
}  // namespace doodle::http