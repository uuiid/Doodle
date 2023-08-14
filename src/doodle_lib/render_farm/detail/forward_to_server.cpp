//
// Created by td_main on 2023/8/14.
//

#include "forward_to_server.h"

#include <doodle_lib/core/bind_front_handler.h>
#include <doodle_lib/render_farm/working_machine_session.h>
namespace doodle {
namespace render_farm {
namespace detail {
void forward_to_server::operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
  auto& l_session = handle_.get<working_machine_session>();
  if (ec) {
    DOODLE_LOG_ERROR("forward_to_server error:{}", ec.message());
    l_session.send_error(ec.message());
    return;
  }
  auto l_server_ip = core_set::get_set().server_ip;

  boost::asio::ip::tcp::resolver resolver{l_session.stream_.get_executor()};
  boost::beast::tcp_stream l_stream{l_session.stream_.get_executor()};
  auto const l_results = resolver.resolve(l_server_ip, "50021");
  l_stream.connect(l_results);
  boost::beast::http::request<boost::beast::http::string_body> l_request{parser_->get()};
  l_request.keep_alive(true);
  boost::beast::http::write(l_stream, l_request);
}

}  // namespace detail
}  // namespace render_farm
}  // namespace doodle