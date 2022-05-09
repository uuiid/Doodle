//
// Created by TD on 2022/5/9.
//
#pragma once
#include <memory>
#include <string>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace doodle::json_rpc {
class rpc_server_ref;
class session : public std::enable_shared_from_this<session> {
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  inline static const std::string end_string = "\r\n\r\n";
  explicit session(
      boost::asio::ip::tcp::socket in_socket);
  virtual ~session();
  void start(std::shared_ptr<rpc_server_ref> in_server);
  void stop();

 private:
  void do_read();

  void do_write();
};

}  // namespace doodle::json_rpc
