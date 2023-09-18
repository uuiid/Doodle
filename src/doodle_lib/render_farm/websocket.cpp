//
// Created by td_main on 2023/9/14.
//

#include "websocket.h"

#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
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
  impl_ptr_->stream_.async_read(
      impl_ptr_->buffer_,
      [this](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == boost::beast::websocket::error::closed) {
          return;
        }
        if (ec) {
          log_error(impl_ptr_->logger_, fmt::format("async_read error: {} ", ec));
          do_read();
          return;
        }
        run_fun();
      }
  );
}

void websocket::run_fun() {}

}  // namespace doodle::render_farm
