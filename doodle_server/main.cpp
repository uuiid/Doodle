/*
 * @Author: your name
 * @Date: 2020-12-12 13:21:34
 * @LastEditTime: 2020-12-15 12:00:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\main.cpp
 */

#include <server.h>
//这是一个zmq包装
#include <zmq.hpp>
#include <zmq_addon.hpp>
// //我们想要使用boost包装 不过看不太懂源码,  先使用其他的..........
// #include <azmq/actor.hpp>
// #include <azmq/socket.hpp>
// #include <azmq/message.hpp>
// #include <azmq/option.hpp>
// #include <azmq/context.hpp>
// #include <azmq/signal.hpp>

#include <thread>

#include <iostream>
#include <server.h>

int main(int argc, char const *argv[]) try {
  //这里是正常的服务器
  zmq::context_t context{7, 1023};

  zmq::socket_t socket{context, zmq::socket_type::router};
  zmq::socket_t proxy_socket{context, zmq::socket_type::dealer};

  socket.bind(doodle::endpoint);
  proxy_socket.bind(doodle::proxy_point);

  //文件处理类
  auto fileSys = std::make_shared<doodle::fileSystem>();
  doodle::Handler h{fileSys};
  std::thread thread{
      [=](doodle::Handler &h_, zmq::context_t *context_k) {
        h_(context_k);
      },
      h, &context};
  //设置代理
  zmq::proxy(socket, proxy_socket);
  return 0;
} catch (const std::exception &error) {
  std::cout << error.what() << std::endl;
  return 1;
}
