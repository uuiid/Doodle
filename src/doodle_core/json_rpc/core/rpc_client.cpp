//
// Created by TD on 2022/4/29.
//

#include "rpc_client.h"
#include <json_rpc/core/server.h>
#include <boost/asio.hpp>
#include <utility>
#include <json_rpc/core/rpc_reply.h>
#include <doodle_core/json_rpc/exception/json_rpc_error.h>
#include <json_rpc/core/session.h>

namespace doodle::json_rpc {

class rpc_client::impl {
 public:
  explicit impl(boost::asio::io_context &in_context)
      : client_socket(in_context){};
  boost::asio::ip::tcp::socket client_socket;
};

rpc_client::rpc_client(boost::asio::io_context &in_context, const std::string &in_host, std::uint16_t in_post)
    : ptr(std::make_unique<impl>(in_context)) {
  ptr->client_socket.connect(boost::asio::ip::tcp::endpoint{
      boost::asio::ip::address::from_string(in_host),
      in_post});
};

std::string rpc_client::call_server(const std::string &in_string, bool is_notice) {
  boost::asio::write(ptr->client_socket, boost::asio::buffer(in_string + session::division_string));
  if (is_notice)
    return {};
  boost::asio::streambuf l_r{};
  boost::asio::read_until(ptr->client_socket, l_r, session::end_string);

  std::istream l_istream{&l_r};
  std::string l_out{};
  std::getline(l_istream, l_out);
  return l_out;
}
void rpc_client::call_server(const std::string &in_string, const string_sig &in_skin) {
  boost::asio::write(ptr->client_socket, boost::asio::buffer(in_string + session::division_string));

  boost::asio::streambuf l_r{};
  //  using iter_buff = boost::asio::buffers_iterator<boost::asio::streambuf::const_buffers_type>;

  //  std::function<std::pair<iter_buff, bool>(iter_buff in_begen, iter_buff in_end)> l_function{
  //      [](iter_buff in_begin, const iter_buff &in_end) -> std::pair<iter_buff, bool> {
  //        iter_buff i = std::move(in_begin);
  //        while (i != in_end)
  //          if (std::isspace(*i++))
  //            return std::make_pair(i, true);
  //        return std::make_pair(i, false);
  //      }};

  std::istream l_istream{&l_r};
  while (true) {
    boost::asio::read_until(ptr->client_socket, l_r, session::end_string);
    std::string l_out{};
    std::getline(l_istream, l_out);
    if (l_out.empty())
      break;
    in_skin(l_out);
  }
}
void rpc_client::close() {
  return this->call_fun<void, true>("rpc.close"s);
}

rpc_client::~rpc_client() {
  close();
}

}  // namespace doodle::json_rpc
