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
  boost::ignore_unused(bytes_transferred);
  auto& l_session = ptr_->self_handle_.get<working_machine_session>();
  if (ec) {
    DOODLE_LOG_ERROR("forward_to_server error:{}", ec.message());
    l_session.send_error_code(ec);
    return;
  }
  auto l_server_ip = core_set::get_set().server_ip;
  if (l_server_ip.empty()) {
    DOODLE_LOG_ERROR("forward_to_server error: server_ip is empty");
    boost::beast::http::response<boost::beast::http::string_body> l_response{
        boost::beast::http::status::bad_request, 11};
    l_response.body() = "forward_to_server error: server_ip is empty";
    l_response.keep_alive(false);
    l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
    return;
  }

  boost::asio::ip::tcp::resolver resolver{l_session.stream().get_executor()};
  ptr_->stream_        = std::make_shared<boost::beast::tcp_stream>(l_session.stream().get_executor());
  auto const l_results = resolver.resolve(l_server_ip, "50021");
  ptr_->stream_->connect(l_results);
  auto l_body       = ptr_->parser_->release();
  auto l_data = l_body.body();
  ptr_->request_    = std::make_shared<request_ptr::element_type>(std::move(l_body));
  ptr_->request_->target("/v1/render_farm/render_job");
  ptr_->request_->body() = l_data;
  ptr_->request_->keep_alive(false);
  ptr_->request_->prepare_payload();
  boost::beast::http::async_write(
      *ptr_->stream_, *ptr_->request_, bind_reg_handler(&forward_to_server::on_write, g_reg(), this)
  );
}
void forward_to_server::on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) {
    DOODLE_LOG_ERROR("forward_to_server error:{}", ec.message());
    auto& l_session = ptr_->self_handle_.get<working_machine_session>();
    l_session.send_error(ec);
    return;
  }

  boost::beast::http::async_read(
      *ptr_->stream_, ptr_->buffer_, ptr_->response_, bind_reg_handler(&forward_to_server::on_read, g_reg(), this)
  );
}
void forward_to_server::on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);
  if (ec) {
    DOODLE_LOG_ERROR("forward_to_server error:{}", ec.message());
    auto& l_session = ptr_->self_handle_.get<working_machine_session>();
    l_session.send_error_code(ec);
    ptr_->stream_->close();
    return;
  }
  auto& l_session = ptr_->self_handle_.get<working_machine_session>();
  boost::beast::http::response<boost::beast::http::string_body> l_response{boost::beast::http::status::ok, 11};
  l_response.body() = ptr_->response_.body();
  l_response.keep_alive(false);
  l_session.send_response(boost::beast::http::message_generator{std::move(l_response)});
}

}  // namespace detail
}  // namespace render_farm
}  // namespace doodle