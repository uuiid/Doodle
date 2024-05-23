//
// Created by TD on 2024/2/20.
//

#include "http_websocket_data.h"

#include "doodle_core/lib_warp/boost_fmt_asio.h"
#include "doodle_core/lib_warp/boost_fmt_error.h"
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include "socket_logger.h"
namespace doodle::http {
void http_websocket_data::run(const http_session_data_ptr& in_data) {
  logger_ =
      g_logger_ctrl().make_log(fmt::format("{}_{}", "socket", SOCKET(in_data->stream_->socket().native_handle())));
  logger_->log(log_loc(), level::info, "开始处理请求 {}", in_data->request_parser_->get().target());

  stream_->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  stream_->set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
    res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  }));
  g_websocket_data_manager().push_back(shared_from_this());
  stream_->async_accept(
      in_data->request_parser_->get(),
      [this, l_self = shared_from_this()](boost::system::error_code ec) {
        if (ec) {
          logger_->log(log_loc(), level::err, "async_accept error: {}", ec);
          return;
        }
        do_read();
      }
  );
}

void http_websocket_data::run_fun() {
  if (read_queue_.empty()) return;
  if (!nlohmann::json::accept(read_queue_.front())) {
    logger_->log(log_loc(), level::err, "json parse error: {}", read_queue_.front());
    read_queue_.pop();
    return;
  }
  auto l_json = nlohmann::json::parse(read_queue_.front());
  read_queue_.pop();
  if (!l_json.contains("type")) {
    logger_->log(log_loc(), level::err, "json parse error: {}", l_json.dump());
    return;
  }
  auto l_fun = (*route_ptr_)(l_json["type"].get<std::string>());
  (*l_fun)(shared_from_this(), l_json);
}
void http_websocket_data::seed(const nlohmann::json& in_json) {
  write_queue_.emplace(in_json.dump());
  do_write();
}

void http_websocket_data::do_read() {
  if (is_reading_) return;
  stream_->async_read(
      buffer_,
      [this, l_g = std::make_shared<read_guard_t>(this),
       l_self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
        l_g.reset();
        if (ec) {
          if (ec != boost::beast::websocket::error::closed && ec != boost::asio::error::operation_aborted) {
            logger_->log(log_loc(), level::err, "async_read error: {}", ec);
          }
          do_close();
          return;
        }
        //        logger_->log(log_loc(), level::info, "async_read success: {}", bytes_transferred);
        read_queue_.emplace(boost::beast::buffers_to_string(buffer_.data()));
        buffer_.consume(buffer_.size());
        do_read();
        run_fun();
      }
  );
}
void http_websocket_data::do_write() {
  if (write_queue_.empty()) return;
  if (is_writing_) return;

  stream_->async_write(
      boost::asio::buffer(write_queue_.front()),
      [this, l_g = std::make_shared<write_guard_t>(this),
       l_self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
        l_g.reset();
        if (ec) {
          if (ec != boost::beast::websocket::error::closed && ec != boost::asio::error::operation_aborted) {
            logger_->log(log_loc(), level::err, "async_write error: {}", ec);
          }
          do_close();
          return;
        }
        //        logger_->log(log_loc(), level::info, "async_write success: {}", bytes_transferred);
        write_queue_.pop();
        do_write();
      }
  );
}
void http_websocket_data::do_close() {
  boost::system::error_code l_error_code{};
  stream_->async_close(
      boost::beast::websocket::close_code::normal,
      [this, l_self = shared_from_this()](boost::system::error_code ec) {
        if (ec) {
          logger_->log(log_loc(), level::err, "async_close error: {}", ec);
        }
      }
  );
}

void http_websocket_data_manager::push_back(const std::shared_ptr<http_websocket_data>& in_data) {
  std::unique_lock lock(mutex_);
  if (auto l_it =
          std::find_if(data_list_.begin(), data_list_.end(), [](const auto& in_data) { return in_data.expired(); });
      l_it != data_list_.end()) {
    *l_it = in_data;
  } else {
    data_list_.emplace_back(in_data);
  }
}

std::vector<std::shared_ptr<http_websocket_data>> http_websocket_data_manager::get_list() {
  {
    std::shared_lock lock(mutex_);
    std::vector<std::shared_ptr<http_websocket_data>> l_data_list_;
    for (auto&& l_data : data_list_) {
      if (auto l_data_ = l_data.lock(); l_data_) {
        l_data_list_.emplace_back(l_data_);
      }
    }
    return l_data_list_;
  }
}

http_websocket_data_manager& g_websocket_data_manager() {
  static http_websocket_data_manager l_manager{};
  return l_manager;
}

}  // namespace doodle::http