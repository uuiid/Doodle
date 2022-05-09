#include <json_rpc/core/server.h>
#include <iostream>

#include <json_rpc/core/parser_rpc.h>
#include <json_rpc/core/rpc_server.h>
#include <json_rpc/core/session_manager.h>
#include <nlohmann/json.hpp>
#include <utility>
#include <boost/asio.hpp>

namespace doodle::json_rpc {
class server::impl {
 public:
  explicit impl(boost::asio::io_context &in_io_context,
                std::uint16_t in_port)
      : acceptor_(in_io_context,
                  boost::asio::ip::tcp::endpoint{
                      boost::asio::ip::address::from_string("127.0.0.1"), in_port}),
        rpc_server_ptr_(),
        session_manager_ptr(std::make_shared<session_manager>()) {}

  boost::asio::ip::tcp::acceptor acceptor_;
  std::shared_ptr<session_manager> session_manager_ptr;
  std::shared_ptr<rpc_server> rpc_server_ptr_;
};

server::server(boost::asio::io_context &in_io_context,
               std::uint16_t in_port)
    : ptr(std::make_unique<impl>(in_io_context, in_port)) {
  do_accept();
}
void server::do_accept() {
  ptr->acceptor_.async_accept(
      [this](boost::system::error_code in_err, boost::asio::ip::tcp::socket &in_socket) {
        if (!in_err) {
          auto l_session = std::make_shared<session>(std::move(in_socket));
          ptr->session_manager_ptr->start(
              l_session,
              std::make_shared<rpc_server_ref>(
                  ptr->rpc_server_ptr_,
                  [            =,
                   se_wptr     = l_session->weak_from_this(),
                   session_ptr = ptr->session_manager_ptr]() {
                    session_ptr->stop(se_wptr.lock());
                  }));
        }
        this->do_accept();
      });
}
void server::set_rpc_server(const std::shared_ptr<rpc_server> &in_server) {
  in_server->init_register();
  ptr->rpc_server_ptr_ = in_server;
}


}  // namespace doodle::json_rpc
