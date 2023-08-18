//
// Created by td_main on 2023/8/3.
//
#pragma once
#include <doodle_core/core/global_function.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <memory>
namespace doodle::render_farm {

enum class working_machine_work_type { none, server, client, work };

class working_machine : public std::enable_shared_from_this<working_machine> {
 public:
  explicit working_machine(boost::asio::io_context& in_io_context, std::uint16_t in_port)
      : end_point_{boost::asio::ip::tcp::v4(), in_port}, acceptor_{in_io_context, end_point_} {}
  ~working_machine() = default;
  void run();
  void stop();

  void config_server();
  void config_client();
  void config_work();
  void config(working_machine_work_type in_type);
  inline working_machine_work_type work_type() const { return work_type_; }

  inline void route(http_route_ptr in_route_ptr) { route_ptr_ = std::move(in_route_ptr); }

 private:
  friend class working_machine_session;
  http_route_ptr route_ptr_;
  void do_accept();
  void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);
  boost::asio::ip::tcp::endpoint end_point_;
  boost::asio::ip::tcp::acceptor acceptor_;
  working_machine_work_type work_type_{};
};
using working_machine_ptr = std::shared_ptr<working_machine>;
}  // namespace doodle::render_farm
