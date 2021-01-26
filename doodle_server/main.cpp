/*
 * @Author: your name
 * @Date: 2020-12-12 13:21:34
 * @LastEditTime: 2020-12-15 12:00:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\main.cpp
 */

#include <zmq.hpp>
#include <src/server.h>

#include <exception>
#include <iostream>
#include <thread>
int main(int argc, char const *argv[]) try {
  zmq::context_t context{7, 1023};

  zmq::socket_t socket{context, zmq::socket_type::router};
  zmq::socket_t proxy_socket{context, zmq::socket_type::dealer};

  socket.bind(doodle::endpoint);
  proxy_socket.bind(doodle::proxy_point);

  auto k_handler = doodle::Handler{};
  std::thread t{
      [=](doodle::Handler h, zmq::context_t *c) {
        h(c);
      },
      k_handler, &context};
  // std::thread t2{
  //     [=](doodle::Handler h, zmq::context_t *c) {
  //       h(c);
  //     },
  //     k_handler, &context};
  zmq::proxy(socket, proxy_socket);

  return 0;
} catch (const std::exception &error) {
  std::cout << error.what() << std::endl;
  return 1;
}
