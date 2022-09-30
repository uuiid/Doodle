#pragma once
#include <memory>

namespace boost::asio {
class io_context;
}

namespace doodle::json_rpc {

class parser_rpc;
class rpc_server;
class rpc_server_ref;
class session_manager;

class server {
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  explicit server(boost::asio::io_context &in_io_context, std::uint16_t in_port = 0);
  virtual ~server();
  void set_rpc_server(const std::shared_ptr<rpc_server> &in_server);

  server(const server &) noexcept            = delete;
  server &operator=(const server &) noexcept = delete;
  server(server &&) noexcept;
  server &operator=(server &&) noexcept;

 private:
  void do_accept();
};

}  // namespace doodle::json_rpc
