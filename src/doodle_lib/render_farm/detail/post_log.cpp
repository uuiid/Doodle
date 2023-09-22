//
// Created by td_main on 2023/9/5.
//

#include "post_log.h"

#include <doodle_core/thread_pool/process_task.h>

#include <doodle_lib/render_farm/detail/render_ue4.h>
#include <doodle_lib/render_farm/detail/ue4_task.h>
#include <doodle_lib/render_farm/working_machine_session.h>
namespace doodle {
namespace render_farm {
namespace detail {
void post_log::operator()(boost::system::error_code ec, std::size_t bytes_transferred) const {
  if (ec == boost::beast::http::error::end_of_stream) {
    session::do_close{impl_->session_handle_}.run();
  }
  if (ec) {
    session::do_write::send_error_code(impl_->session_handle_, ec);
    return;
  }

  boost::ignore_unused(bytes_transferred);

  if (!impl_->msg_handle_ || !impl_->msg_handle_.all_of<process_message, ue4_task>()) {
    BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::bad_target);
    session::do_write::send_error_code(impl_->session_handle_, ec);
    return;
  }

  auto l_str = impl_->parser_->release().body();
  switch (impl_->msg_type_) {
    case log_enum::log:
      impl_->msg_handle_.patch<process_message>().message(std::move(l_str), process_message::level::info);
      break;
    case log_enum::err:
      impl_->msg_handle_.patch<process_message>().message(std::move(l_str), process_message::level::warning);
      break;
  }
}

}  // namespace detail
}  // namespace render_farm
}  // namespace doodle