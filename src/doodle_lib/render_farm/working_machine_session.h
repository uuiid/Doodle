//
// Created by td_main on 2023/8/3.
//

#pragma once
#include "doodle_core/core/global_function.h"

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <memory>

namespace doodle::render_farm {

class working_machine;
class working_machine_session : public std::enable_shared_from_this<working_machine_session> {
 public:
  explicit working_machine_session(
      boost::asio::ip::tcp::socket in_socket, std::shared_ptr<working_machine> in_working_machine
  )
      : stream_{std::move(in_socket)}, working_machine_{std::move(in_working_machine)} {}

  void run();

 private:
  void do_read();
  void on_parser(boost::system::error_code ec, std::size_t bytes_transferred);
  void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void send_response(boost::beast::http::message_generator&& in_message_generator);
  void on_write(bool keep_alive, boost::system::error_code ec, std::size_t bytes_transferred);
  void do_close();
  boost::beast::tcp_stream stream_;
  boost::beast::flat_buffer buffer_;
  boost::beast::http::request_parser<boost::beast::http::empty_body> request_parser_;
  std::shared_ptr<working_machine> working_machine_;
};
}  // namespace doodle::render_farm
