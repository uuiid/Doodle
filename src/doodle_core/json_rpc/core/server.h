#pragma once

#include <boost/asio.hpp>

namespace doodle::json_rpc {

class parser_rpc;
class rpc_server;
class rpc_server_ref;
class session_manager;
class server {
  boost::asio::ip::tcp::acceptor acceptor_;
  std::shared_ptr<session_manager> session_manager_ptr;
  std::shared_ptr<rpc_server> rpc_server_ptr_;

 public:
  explicit server(boost::asio::io_context& in_io_context,
                  std::uint16_t in_port);

  void set_rpc_server(const std::shared_ptr<rpc_server>& in_server);

 private:
  void do_accept();
};

class session : public std::enable_shared_from_this<session> {
  boost::asio::ip::tcp::socket socket_;
  std::shared_ptr<parser_rpc> parser_rpc_ptr;
  std::shared_ptr<rpc_server_ref> rpc_server_;

 public:
  inline static const std::string end_string = "\r\n\r\n";
  explicit session(
      boost::asio::ip::tcp::socket in_socket);

  void start(std::shared_ptr<rpc_server_ref> in_server);
  void stop();

 private:
  void do_read();

  void do_write();

  boost::asio::streambuf data_{};
  std::string msg_{};
};
}  // namespace doodle::json_rpc
