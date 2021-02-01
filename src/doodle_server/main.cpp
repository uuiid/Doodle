/*
 * @Author: your name
 * @Date: 2020-12-12 13:21:34
 * @LastEditTime: 2020-12-15 12:00:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\main.cpp
 */

#include <zmq.hpp>
#include <doodle_server/source/seting.h>
#include <doodle_server/source/server.h>

// #include <Windows.h>
#include <loggerlib/Logger.h>

#include <exception>
#include <iostream>
#include <thread>
#include <queue>
int main(int argc, char const *argv[]) try {
  //初始化log
  Logger::doodle_initLog();

  auto &set = doodle::Seting::Get();
  set.init();

  zmq::context_t context{7, 1023};

  zmq::socket_t socket{context, zmq::socket_type::router};
  zmq::socket_t proxy_socket{context, zmq::socket_type::dealer};

  socket.bind(doodle::endpoint);
  proxy_socket.bind(doodle::proxy_point);

  auto k_handler = doodle::Handler{};

  std::queue<std::thread> thread_pool;
  for (int i = 0; i < 8; ++i) {
    std::thread t{
        [=](doodle::Handler h, zmq::context_t *c) {
          h(c);
        },
        k_handler, &context};
    thread_pool.push(std::move(t));
  }
  zmq::proxy(socket, proxy_socket);
  // boost::log::core::get()->remove_all_sinks();
  return 0;
} catch (const std::exception &error) {
  std::cout << error.what() << std::endl;
  boost::log::core::get()->remove_all_sinks();
  return 1;
}
