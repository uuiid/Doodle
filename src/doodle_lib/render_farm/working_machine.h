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

class working_machine : public std::enable_shared_from_this<working_machine> {
 public:
  working_machine() = default;
  explicit working_machine(boost::asio::io_context& in_io_context, std::uint16_t in_port)
      : end_point_{boost::asio::ip::tcp::v4(), in_port}, acceptor_{in_io_context, end_point_} {}
  ~working_machine() = default;
  void run();
  void stop();

 private:
  void do_accept();
  void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);
  boost::asio::ip::tcp::endpoint end_point_;
  boost::asio::ip::tcp::acceptor acceptor_;
};
using working_machine_ptr = std::shared_ptr<working_machine>;
}  // namespace doodle::render_farm
