//
// Created by td_main on 2023/9/14.
//

#include "websocket.h"

#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/msg_error.h>

#include <doodle_lib/render_farm/functional_registration_manager.h>
namespace doodle::render_farm {

void websocket::run(const boost::beast::http::request<boost::beast::http::string_body>& in_message) {
  if (!data_.all_of<socket_logger>()) {
    data_.emplace<socket_logger>();
  }

  log_info(
      data_.get<socket_logger>().logger_,
      fmt::format("开始处理请求 {} {}", to_string(in_message.method()), in_message.target())
  );

  auto& l_data          = data_.get<websocket_data>();
  l_data.websocket_ptr_ = shared_from_this();

  l_data.stream_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  //  l_data.stream_.control_callback();
  l_data.stream_.set_option(
      boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
        res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
      })
  );

  l_data.stream_.async_accept(
      in_message,
      [this, logger = data_.get<socket_logger>().logger_, self = shared_from_this()](boost::system::error_code ec) {
        if (ec) {
          log_error(logger, fmt::format("async_accept error: {} ", ec));
          fail_call(ec);
          return;
        }
        if (data_ && data_.all_of<websocket_data>()) {
          data_.patch<websocket_data>().is_handshake_ = true;
          do_read();
          if (!data_.get<websocket_data>().write_queue.empty()) do_write();
        }
      }
  );
}

void websocket::do_read() {
  if (!data_ || !data_.all_of<websocket_data>()) return;
  auto& l_data = data_.get<websocket_data>();

  if (l_data.read_flag_) {
    return;
  }

  l_data.stream_.async_read(
      l_data.buffer_,
      [this, logger = data_.get<socket_logger>().logger_,
       self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        auto l_has_data = data_ && data_.all_of<websocket_data>();
        if (l_has_data) data_.get<websocket_data>().read_flag_ = false;

        if (ec) {
          log_error(logger, fmt::format("async_write error: {} ", ec));
          if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::operation_aborted) {
            fail_call(ec);
            do_destroy();
            return;
          }
        }
        if (l_has_data) {
          auto& l_data = data_.get<websocket_data>();
          l_data.read_queue.emplace(boost::beast::buffers_to_string(l_data.buffer_.data()));
          l_data.buffer_.consume(l_data.buffer_.size());
          do_read();
          boost::asio::post(g_io_context(), [this, self = shared_from_this()] { run_fun(); });
        }
      }
  );
}

void websocket::run_fun() {
  boost::system::error_code ec{};
  if (!data_ || !data_.all_of<websocket_data>()) return;
  auto& l_data  = data_.get<websocket_data>();
  auto l_logger = data_.get<socket_logger>().logger_;
  if (l_data.read_queue.empty()) return;

  if (!nlohmann::json::accept(l_data.read_queue.front())) {
    log_error(l_logger, fmt::format("json parse error: {} ", l_data.read_queue.front()));
    l_data.read_queue.pop();
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    send_error_code(ec, 0);
    return;
  }
  auto l_json = nlohmann::json::parse(l_data.read_queue.front());
  l_data.read_queue.pop();
  // 这个是回复
  if (l_json.contains("result")) {
    auto id = l_json["id"].get<uint64_t>();
    log_info(l_logger, fmt::format("开始检查回复 {}", id));
    if (l_json.contains("error")) {  // 这个是回复的错误
      log_info(
          l_logger,
          fmt::format("回复错误 {} {}", l_json["id"].get<uint64_t>(), l_json["error"]["message"].get<std::string>())
      );
    }
    l_data.call_map_[id]({}, l_json);
    l_data.call_map_.erase(id);
  } else if (l_json.contains("method")) {  // 这个是请求
    log_info(l_logger, fmt::format("开始检查请求 {}", l_json["id"].get<uint64_t>()));
    nlohmann::json l_json_rep{};
    auto l_call = g_ctx().get<functional_registration_manager>().get_function(l_json["method"].get<std::string>());
    if (l_call) {
      l_json_rep = l_call(data_, l_json["params"]);
    } else {
      l_json_rep["error"]["code"]    = msg_error::error_enum::method_not_found;
      l_json_rep["error"]["message"] = "method not found";
    }
    l_json_rep["id"] = l_json["id"];
    l_data.write_queue.emplace(l_json_rep.dump());
    do_write();
  }
}

void websocket::send_error_code(const boost::system::error_code& in_code, std::uint64_t in_id) {
  if (!data_ || !data_.all_of<websocket_data>()) return;
  auto& l_data = data_.get<websocket_data>();

  nlohmann::json l_json{};
  l_json["id"]               = in_id;
  l_json["error"]["code"]    = in_code.value();
  l_json["error"]["message"] = in_code.message();
  l_json["error"]["data"]    = in_code.category().name();
  l_data.write_queue.emplace(l_json.dump());
  do_write();
}

void websocket::do_write() {
  if (!data_ || !data_.all_of<websocket_data>()) return;
  auto& l_data = data_.get<websocket_data>();

  if (l_data.write_queue.empty()) {
    return;
  }
  if (l_data.write_flag_) {
    return;
  }
  l_data.write_flag_ = true;
  l_data.stream_.async_write(
      boost::asio::buffer(l_data.write_queue.front()),
      [this, logger = data_.get<socket_logger>().logger_,
       self = shared_from_this()](boost::system::error_code ec, std::size_t) {
        auto l_has_data = data_ && data_.all_of<websocket_data>();
        if (l_has_data) data_.get<websocket_data>().write_flag_ = false;

        if (ec) {
          log_error(logger, fmt::format("async_write error: {} ", ec));
          if (ec == boost::beast::websocket::error::closed || ec == boost::asio::error::operation_aborted) {
            fail_call(ec);
            do_destroy();
            return;
          }
        }
        if (l_has_data) {
          auto& l_data = data_.get<websocket_data>();
          l_data.write_queue.pop();
          if (!l_data.write_queue.empty()) {
            do_write();
          }
        }
      }
  );
}

void websocket::do_destroy() {
  boost::asio::post(g_io_context(), [handle = data_] {
    auto l = handle;
    if (l) l.destroy();
  });
}
void websocket::close() {
  if (!data_ || !data_.all_of<websocket_data>()) return;
  auto& l_data = data_.get<websocket_data>();
  l_data.stream_.async_close(
      boost::beast::websocket::close_code::normal,
      [logger = data_.get<socket_logger>().logger_, this](boost::system::error_code ec) {
        if (ec) {
          log_error(logger, fmt::format("async_close error: {} ", ec));
        }
        do_destroy();
      }
  );
}

void websocket::fail_call(boost::system::error_code in_code) {
  if (!data_ || !data_.all_of<websocket_data>()) return;
  for (auto&& [id, call] : data_.get<websocket_data>().call_map_) {
    call(in_code, {});
  }
  data_.patch<websocket_data>().call_map_.clear();
}
void websocket::fail_call(boost::system::error_code in_code, std::int64_t in_id) {
  if (!data_ || !data_.all_of<websocket_data>()) return;
  auto& l_data = data_.get<websocket_data>();
  if (l_data.call_map_.find(in_id) != l_data.call_map_.end()) {
    boost::asio::post(g_io_context(), [in_id, &l_data, call = l_data.call_map_[in_id], in_code] {
      call(in_code, {});
      l_data.call_map_.erase(in_id);
    });
  }
}

}  // namespace doodle::render_farm
