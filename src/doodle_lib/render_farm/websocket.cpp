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
  auto& l_data = data_.get<websocket_data>();
  l_data.stream_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  l_data.stream_.set_option(
      boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
        res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
      })
  );

  l_data.stream_.async_accept(
      in_message,
      [this, logger = l_data.logger_, self = shared_from_this()](boost::system::error_code ec) {
        if (ec) {
          log_error(logger, fmt::format("async_accept error: {} ", ec));
          return;
        }
        do_read();
      }
  );
}
void websocket::do_read() {
  if (!data_) return;
  auto& l_data = data_.get<websocket_data>();
  if (l_data.read_flag_) {
    return;
  }

  l_data.stream_.async_read(
      l_data.buffer_,
      [this, logger = l_data.logger_,
       self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == boost::beast::websocket::error::closed) {
          return;
        }
        if (ec) {
          log_error(logger, fmt::format("async_read error: {} ", ec));
        }
        if (!data_) return;
        auto& l_data      = data_.get<websocket_data>();
        l_data.read_flag_ = false;
        l_data.read_queue.emplace(boost::beast::buffers_to_string(l_data.buffer_.data()));
        l_data.buffer_.consume(l_data.buffer_.size());
        do_read();
        boost::asio::post(g_io_context(), [this, self = shared_from_this()] { run_fun(); });
      }
  );
}

void websocket::run_fun() {
  boost::system::error_code ec{};
  if (!data_) return;
  auto& l_data = data_.get<websocket_data>();

  if (!nlohmann::json::accept(l_data.read_queue.front())) {
    log_error(l_data.logger_, fmt::format("json parse error: {} ", l_data.read_queue.front()));
    l_data.read_queue.pop();
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    send_error_code(ec, 0);
    return;
  }
  auto l_json = nlohmann::json::parse(l_data.read_queue.front());
  l_data.read_queue.pop();
  // 这个是回复
  if (l_json.contains("result")) {
    log_info(l_data.logger_, fmt::format("开始检查回复 {}", l_json["id"].get<uint64_t>()));
    if (l_json.contains("error")) {  // 这个是回复的错误
      log_info(
          l_data.logger_,
          fmt::format("回复错误 {} {}", l_json["id"].get<uint64_t>(), l_json["error"]["message"].get<std::string>())
      );
    }
  } else if (l_json.contains("method")) {  // 这个是请求
    log_info(l_data.logger_, fmt::format("开始检查请求 {}", l_json["id"].get<uint64_t>()));
    nlohmann::json l_json_rep{};
    auto l_call = g_ctx().get<functional_registration_manager>().get_function(l_json["method"].get<std::string>());
    if (l_call) {
      l_json_rep["result"] = l_call(l_json["params"]);
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
  if (!data_) return;
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
  if (!data_) return;
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
      [this, logger = l_data.logger_, self = shared_from_this()](boost::system::error_code ec, std::size_t) {
        if (ec == boost::beast::websocket::error::closed) {
          return;
        }
        if (ec) {
          log_error(logger, fmt::format("async_write error: {} ", ec));
        }
        if (!data_) return;
        auto& l_data       = data_.get<websocket_data>();
        l_data.write_flag_ = false;

        l_data.write_queue.pop();
        if (!l_data.write_queue.empty()) {
          do_write();
        }
      }
  );
}

}  // namespace doodle::render_farm
