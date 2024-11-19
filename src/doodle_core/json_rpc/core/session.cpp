//
// Created by TD on 2022/5/9.
//

#include "session.h"

#include <doodle_core/json_rpc/core/parser_rpc.h>
#include <doodle_core/json_rpc/core/rpc_server.h>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <string>


namespace doodle::json_rpc {

class rpc_server;

class session_manager;
class session::impl {
 public:
  explicit impl(boost::asio::io_context& in_io_context, boost::asio::ip::tcp::socket in_socket)
      : io_context_(in_io_context),
        socket_(std::move(in_socket)),
        data_(),
        rpc_server_(){

        };
  boost::asio::io_context& io_context_;
  boost::asio::ip::tcp::socket socket_;

  std::shared_ptr<rpc_server> rpc_server_;

  boost::asio::streambuf data_{};
  std::string msg_{};

  bool stop_{false};
};

session::session(boost::asio::io_context& in_io_context, boost::asio::ip::tcp::socket in_socket)
    : ptr(std::make_unique<impl>(in_io_context, std::move(in_socket))){

      };

void session::start(std::shared_ptr<rpc_server> in_server) {
  ptr->rpc_server_ = std::move(in_server);
  boost::asio::spawn(ptr->io_context_, [self = shared_from_this(), this](const boost::asio::yield_context& yield) {
    //    using iter_buff = boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type>;
    //    static std::function<
    //        std::pair<iter_buff, bool>(iter_buff, iter_buff)>
    //        l_function{
    //            [](iter_buff in_begin, const iter_buff& in_end)
    //                -> std::pair<iter_buff, bool> {
    //              iter_buff i = std::move(in_begin);
    //              while (i != in_end)
    //                if (std::isspace(*i++))
    //                  return std::make_pair(i, true);
    //              return std::make_pair(i, false);
    //            }};

    while (!ptr->stop_) {
      boost::system::error_code ec{};

      boost::asio::async_read_until(ptr->socket_, ptr->data_, end_string, yield[ec]);
      if (!ec) {
        std::istream l_istream{&ptr->data_};
        std::string l_ine{};
        std::getline(l_istream, l_ine);
        if (l_ine.empty()) continue;


      } else {
        break;
      }
    }
  });
}

void session::stop() {
  ptr->stop_ = true;
  ptr->socket_.close();
}
void session::session_manager_attr(session_manager* in_session_manager) {}
session::~session() = default;

}  // namespace doodle::json_rpc
