//
// Created by TD on 2022/4/29.
//

#include "rpc_client.h"
#include <json_rpc/core/server.h>

namespace doodle::json_rpc {
std::string rpc_client::call_server(const std::string& in_string, bool is_notice) {
  boost::asio::write(client_socket, boost::asio::buffer(in_string + session::end_string));
  if (is_notice)
    return {};
  boost::asio::streambuf l_r{};
  auto l_size = boost::asio::read_until(client_socket, l_r, session::end_string);
  std::string l_str{
      boost::asio::buffers_begin(l_r.data()),
      boost::asio::buffers_begin(l_r.data()) + l_size - session::end_string.size()};
  return l_str;
}
rpc_client::~rpc_client() {
  close();
}
}  // namespace doodle::json_rpc
