//
// Created by td_main on 2023/8/17.
//

#include "render_job_put.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/core/erase_handle_component.h>
#include <doodle_lib/render_farm/render_farm_fwd.h>
#include <doodle_lib/render_farm/working_machine_session.h>

namespace doodle {
namespace render_farm {
namespace detail {
void render_job_put::operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  auto& l_session = ptr_->session_handle_.get<working_machine_session>();
  auto l_logger   = ptr_->session_handle_.get<socket_logger>().logger_;
  l_session.stream().expires_after(30s);
  if (ec) {
    log_error(l_logger, fmt::format("on_read error: {} ", ec));
    l_session.send_error_code(ec);
    return;
  }
  if (!ptr_->modify_handle_) {
    log_warn(l_logger, fmt::format("on_read error: modify_handle_ {} is not valid", ptr_->modify_handle_));
    l_session.send_error_code(
        boost::asio::error::make_error_code(boost::asio::error::invalid_argument),
        boost::beast::http::status::bad_request
    );
    return;
  }

  auto l_json = ptr_->parser_->release().body();
  if (l_json.contains("state")) {
    auto l_state = l_json["state"].get<process_message::state>();
    ptr_->modify_handle_.get<process_message>().set_state(l_state);

    auto l_response   = boost::beast::http::response<detail::basic_json_body>{boost::beast::http::status::ok, 11};
    l_response.body() = {{"state", "ok"}};
    l_response.keep_alive(ptr_->parser_->keep_alive());
    l_response.prepare_payload();
    l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
  } else {
    log_error(l_logger, fmt::format("json parse error: state is not valid"));
    l_session.send_error_code(
        boost::asio::error::make_error_code(boost::asio::error::invalid_argument),
        boost::beast::http::status::bad_request
    );
  }
}

}  // namespace detail
}  // namespace render_farm
}  // namespace doodle