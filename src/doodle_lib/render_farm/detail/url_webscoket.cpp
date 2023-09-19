//
// Created by td_main on 2023/9/14.
//
#include "url_webscoket.h"

#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/render_farm/websocket.h>
namespace doodle::render_farm {
namespace detail {

void computer_reg_type_websocket::operator()(
    const entt::handle& in_handle, const std::map<std::string, std::string>& in_cap
) const {
  auto& l_session   = in_handle.get<working_machine_session>();
  auto l_parser_ptr = std::make_shared<boost::beast::http::request_parser<boost::beast::http::string_body>>(
      std::move(l_session.request_parser())
  );
  boost::beast::http::async_read(
      l_session.stream(), l_session.buffer(), *l_parser_ptr,
      [in_handle, l_parser_ptr](boost::system::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        auto& l_session = in_handle.get<working_machine_session>();
        if (ec == boost::beast::http::error::end_of_stream) {
          return l_session.do_close();
        }
        if (ec) {
          log_error(l_session.logger(), fmt::format("on_read error: {} ", ec));
          l_session.send_error_code(ec);
          return;
        }
        in_handle.emplace<render_farm::websocket>(l_session.stream().release_socket()).run(l_parser_ptr->release());
      }
  );
}

}  // namespace detail
}  // namespace doodle::render_farm
