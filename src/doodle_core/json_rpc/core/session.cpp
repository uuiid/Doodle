//
// Created by TD on 2022/5/9.
//

#include "session.h"
#include <boost/asio.hpp>
namespace doodle {
namespace json_rpc {

class parser_rpc;
class rpc_server;
class rpc_server_ref;
class session_manager;
class session::impl {
 public:
  explicit impl(
      boost::asio::ip::tcp::socket& in_socket)
      : socket_(std::move(in_socket)),
        data_(),
        rpc_server_(){

        };

  boost::asio::ip::tcp::socket socket_;
  std::shared_ptr<parser_rpc> parser_rpc_ptr;
  std::shared_ptr<rpc_server_ref> rpc_server_;

  boost::asio::streambuf data_{};
  std::string msg_{};
};

session::session(boost::asio::ip::tcp::socket& in_socket)
    : ptr(std::make_unique<impl>(in_socket)){

      };

void session::start(std::shared_ptr<rpc_server_ref> in_server) {
  ptr->rpc_server_ = std::move(in_server);
  do_read();
}

void session::do_read() {
  static std::string l_delim{};
  /// 确保json字符串完全读取完整
  boost::asio::async_read_until(
      ptr->socket_,
      ptr->data_,
      end_string,
      [self = shared_from_this(), this](const boost::system::error_code in_err,
                                        std::size_t in_len) {
        if (!in_err) {
          ptr->parser_rpc_ptr = std::make_shared<parser_rpc>(std::string{
              boost::asio::buffers_begin(ptr->data_.data()),
              boost::asio::buffers_begin(ptr->data_.data()) + in_len - end_string.size()});
          ptr->data_.consume(in_len);
          ptr->msg_ = (*ptr->parser_rpc_ptr)(*ptr->rpc_server_) + end_string;
          this->do_write();
        } else {
          ptr->socket_.close();
        }
      });
}

void session::do_write() {
  if (ptr->socket_.is_open()) {
    boost::asio::async_write(
        ptr->socket_,
        boost::asio::buffer(ptr->msg_),
        [self = shared_from_this(), this](boost::system::error_code in_err, std::size_t in_len_) {
          if (!in_err) {
            this->do_read();
          } else {
            ptr->socket_.close();
          }
        });
  }
}
void session::stop() {
  ptr->socket_.close();
}

}  // namespace json_rpc
}  // namespace doodle
