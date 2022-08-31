//
// Created by TD on 2022/5/9.
//
#pragma once
#include <memory>
#include <string>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace boost::asio {
class io_context;
}
namespace doodle::json_rpc {

class session_manager;
class rpc_server;
class session : public std::enable_shared_from_this<session> {
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  inline static const std::string end_string      = "\n\n";
  inline static const std::string division_string = "\n";
  explicit session(
      boost::asio::io_context& in_io_context,
      boost::asio::ip::tcp::socket in_socket
  );
  virtual ~session();

 private:
  friend class session_manager;
  void start(std::shared_ptr<rpc_server> in_server);
  void session_manager_attr(session_manager* in_session_manager);
  void stop();
};

}  // namespace doodle::json_rpc
