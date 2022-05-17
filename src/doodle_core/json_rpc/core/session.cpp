//
// Created by TD on 2022/5/9.
//

#include "session.h"
#include <boost/asio.hpp>
#include <doodle_core/json_rpc/core/parser_rpc.h>
#include <doodle_core/json_rpc/core/rpc_server.h>

#include <boost/asio/spawn.hpp>

namespace doodle {
namespace json_rpc {
using string_coroutine = boost::coroutines2::coroutine<std::string>;

class parser_rpc;
class rpc_server;
class rpc_server_ref;
class session_manager;
class session::impl {
 public:
  explicit impl(
      boost::asio::io_context& in_io_context,
      boost::asio::ip::tcp::socket in_socket)
      : io_context_(in_io_context),
        socket_(std::move(in_socket)),
        data_(),
        rpc_server_(),
        parser_rpc_(){

        };
  boost::asio::io_context& io_context_;
  boost::asio::ip::tcp::socket socket_;
  parser_rpc parser_rpc_;
  std::shared_ptr<rpc_server_ref> rpc_server_;

  boost::asio::streambuf data_{};
  std::string msg_{};
};

session::session(boost::asio::io_context& in_io_context,
                 boost::asio::ip::tcp::socket in_socket)
    : ptr(std::make_unique<impl>(in_io_context, std::move(in_socket))){

      };

void session::start(std::shared_ptr<rpc_server_ref> in_server) {
  ptr->rpc_server_ = std::move(in_server);
  boost::asio::spawn(ptr->io_context_,
                     [self = shared_from_this(), this](const boost::asio::yield_context& yield) {
                       while (true) {
                         boost::system::error_code ec{};

                         auto len = boost::asio::async_read_until(
                             ptr->socket_,
                             ptr->data_,
                             end_string,
                             yield[ec]);
                         if (!ec) {
                           ptr->parser_rpc_.json_data_attr(std::string{
                               boost::asio::buffers_begin(ptr->data_.data()),
                               boost::asio::buffers_begin(ptr->data_.data()) + len - end_string.size()});
                           ptr->data_.consume(len);
                           string_coroutine::pull_type l_pull_c{[=](string_coroutine::push_type& in_skin) {
                             ptr->parser_rpc_(in_skin, *ptr->rpc_server_);
                           }};

                           for (auto&& fun_str : l_pull_c) {
                             auto msg = fun_str + division_string;
                             boost::asio::async_write(ptr->socket_,
                                                      boost::asio::buffer(fun_str),
                                                      yield[ec]);
                           }
                           std::string end{end_string};
                           boost::asio::async_write(ptr->socket_,
                                                    boost::asio::buffer(end),
                                                    yield[ec]);
                         } else {
                           ptr->rpc_server_->close_current();
                           break;
                         }
                       }
                     });
}

void session::stop() {
  ptr->socket_.close();
}
session::~session() = default;

}  // namespace json_rpc
}  // namespace doodle
