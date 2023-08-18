//
// Created by td_main on 2023/8/17.
//

#include "render_job_put.h"

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/core/erase_handle_component.h>
#include <doodle_lib/render_farm/working_machine_session.h>

namespace doodle {
namespace render_farm {
namespace detail {
void render_job_put::operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  auto& l_session = ptr_->handle_.get<working_machine_session>();
  if (ec) {
    DOODLE_LOG_ERROR("on_read error: {}", ec.message());
    l_session.send_error_code(ec);
    return;
  }
  auto l_json = ptr_->parser_->release().body();
  if (l_json.contains("state")) {
    auto l_state = magic_enum::enum_cast<process_message::state>(l_json["state"].get<std::string>());
    if (l_state) {
      ptr_->handle_.get<process_message>().set_state(*l_state);

      auto l_response   = boost::beast::http::response<detail::basic_json_body>{boost::beast::http::status::ok, 11};
      l_response.body() = {{"state", "ok"}};
      l_response.keep_alive(ptr_->parser_->keep_alive());
      l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});

    } else {
      DOODLE_LOG_ERROR("on_read error: state is not valid");
      auto l_response =
          boost::beast::http::response<detail::basic_json_body>{boost::beast::http::status::bad_request, 11};
      l_response.body() = {{"state", "error"}, {"error", "state is not valid"}};
      l_response.keep_alive(ptr_->parser_->keep_alive());
      l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
    }
  }
  erase_reg_component(this);
}

}  // namespace detail
}  // namespace render_farm
}  // namespace doodle