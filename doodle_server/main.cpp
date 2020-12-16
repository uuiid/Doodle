/*
 * @Author: your name
 * @Date: 2020-12-12 13:21:34
 * @LastEditTime: 2020-12-15 12:00:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\main.cpp
 */

#include <server.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
// #include <boost/network/utils/thread_pool.hpp>

int main(int argc, char const *argv[]) try {
  const std::string endpoint = R"(tcp://*:5555)";

  zmq::context_t context{32, 1023};
  zmq::socket_t socket{context, zmq::socket_type::push};
  socket.bind(endpoint);
  while (true) {
    zmq::message_t message{};
    socket.recv(message, zmq::recv_flags::dontwait);
    std::cout << message << std::endl;

    socket.send(message);
  }

  // auto fileSys = std::make_shared<doodle::fileSystem>();
  // doodle::Handler handle(fileSys);
  // doodle::Server::options options(handle);
  // doodle::Server instance{
  //     options.thread_pool(std::make_shared<boost::network::utils::thread_pool>(4))
  //         .address("127.0.0.1")
  //         .port("8000")};
  // instance.run();
  return 0;
} catch (const std::exception &error) {
  std::cout << error.what() << std::endl;
  return 1;
}
