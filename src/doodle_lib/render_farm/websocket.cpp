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
void websocket::make_ptr() {
  impl_ptr_->logger_ = g_logger_ctrl().make_log(
      fmt::format("websocket {} {}", fmt::ptr(this), boost::beast::get_lowest_layer(impl_ptr_->stream_))
  );
}
void websocket::run(const boost::beast::http::request<boost::beast::http::string_body>& in_message) {
  impl_ptr_->stream_.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server
  ));
  impl_ptr_->stream_.set_option(
      boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
        res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
      })
  );

  impl_ptr_->stream_.async_accept(in_message, [this](boost::system::error_code ec) {
    if (ec) {
      log_error(impl_ptr_->logger_, fmt::format("async_accept error: {} ", ec));
      return;
    }
    do_read();
  });
}
void websocket::do_read() {
  if (impl_ptr_->read_flag_) {
    return;
  }

  impl_ptr_->stream_.async_read(
      impl_ptr_->buffer_,
      [this](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == boost::beast::websocket::error::closed) {
          return;
        }
        if (ec) {
          log_error(impl_ptr_->logger_, fmt::format("async_read error: {} ", ec));
        }
        impl_ptr_->read_flag_ = false;
        impl_ptr_->read_queue.emplace(boost::beast::buffers_to_string(impl_ptr_->buffer_.data()));
        impl_ptr_->buffer_.consume(impl_ptr_->buffer_.size());
        do_read();
        boost::asio::post(g_io_context(), [this] { run_fun(); });
      }
  );
}

void websocket::run_fun() {
  boost::system::error_code ec{};

  if (!nlohmann::json::accept(impl_ptr_->read_queue.front())) {
    log_error(impl_ptr_->logger_, fmt::format("json parse error: {} ", impl_ptr_->read_queue.front()));
    impl_ptr_->read_queue.pop();
    BOOST_BEAST_ASSIGN_EC(ec, error_enum::bad_json_string);
    send_error_code(ec, 0);
    return;
  }
  auto l_json = nlohmann::json::parse(impl_ptr_->read_queue.front());
  impl_ptr_->read_queue.pop();
  // 这个是回复
  if (l_json.contains("result")) {
    log_info(impl_ptr_->logger_, fmt::format("开始检查回复 {}", l_json["id"].get<uint64_t>()));
    if (l_json.contains("error")) {  // 这个是回复的错误
      log_info(
          impl_ptr_->logger_,
          fmt::format("回复错误 {} {}", l_json["id"].get<uint64_t>(), l_json["error"]["message"].get<std::string>())
      );
    }
  } else if (l_json.contains("method")) {  // 这个是请求
    log_info(impl_ptr_->logger_, fmt::format("开始检查请求 {}", l_json["id"].get<uint64_t>()));
    nlohmann::json l_json_rep{};
    auto l_call = g_ctx().get<functional_registration_manager>().get_function(l_json["method"].get<std::string>());
    if (l_call) {
      l_json_rep["result"] = l_call(l_json["params"]);
    } else {
      l_json_rep["error"]["code"]    = msg_error::error_enum::method_not_found;
      l_json_rep["error"]["message"] = "method not found";
    }
    l_json_rep["id"] = l_json["id"];
    impl_ptr_->write_queue.emplace(l_json_rep.dump());
    do_write();
  }
}

void websocket::send_error_code(const boost::system::error_code& in_code, std::uint64_t in_id) {
  nlohmann::json l_json{};
  l_json["id"]               = in_id;
  l_json["error"]["code"]    = in_code.value();
  l_json["error"]["message"] = in_code.message();
  l_json["error"]["data"]    = in_code.category().name();
  impl_ptr_->write_queue.emplace(l_json.dump());
  do_write();
}

void websocket::do_write() {
  if (impl_ptr_->write_queue.empty()) {
    return;
  }
  if (impl_ptr_->write_flag_) {
    return;
  }
  impl_ptr_->write_flag_ = true;
  impl_ptr_->stream_.async_write(
      boost::asio::buffer(impl_ptr_->write_queue.front()),
      [this](boost::system::error_code ec, std::size_t) {
        impl_ptr_->write_flag_ = false;
        if (ec == boost::beast::websocket::error::closed) {
          return;
        }
        if (ec) {
          log_error(impl_ptr_->logger_, fmt::format("async_write error: {} ", ec));
        }

        impl_ptr_->write_queue.pop();
        if (!impl_ptr_->write_queue.empty()) {
          do_write();
        }
      }
  );
}

}  // namespace doodle::render_farm
