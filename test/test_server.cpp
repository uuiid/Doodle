/*
 * @Author: your name
 * @Date: 2020-12-15 11:42:58
 * @LastEditTime: 2020-12-16 11:25:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\test_server.h
 */
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>

#include <zmq.hpp>
#include <zmq_addon.hpp>

TEST(doodleServer, client_base) {
}

TEST(doodleServer, server_base) {
  zmq::context_t context(1);
  zmq::socket_t socket(context, zmq::socket_type::req);
  std::cout << "Connecting to hello world server…" << std::endl;

  socket.connect("tcp://127.0.0.1:6666");

  zmq::message_t message{"CH015A.usd"};
  socket.send(message, zmq::send_flags::none);

  zmq::message_t reply{};
  socket.recv(reply, zmq::recv_flags::none);

  boost::filesystem::path path{"D:/tmp/test.file"};
  boost::filesystem::ofstream stream(path, std::ios_base::binary);
  stream.write(static_cast<char *>(reply.data()), reply.size());
}

TEST(doodleServer, buffer_stream) {
  // boost::filesystem::path k_path("F:/cppguide.xml");
  // boost::filesystem::path k_path("F:/doodle.exe");
  // boost::filesystem::ifstream stream(k_path, std::ios::in | std::ios::binary);
  // std::cout << stream.is_open() << std::endl;

  // stream.seekg(0, std::ios::end);
  // auto length = stream.tellg();
  // stream.seekg(0, std::ios::beg);
  // if (length > 4096)
  //   length = 4096;

  // char *buffer = new char[length];

  // stream.read(buffer, length);
  // boost::asio::const_buffer buff{buffer, sizeof(buffer)};
  // std::cout << std::string(static_cast<char *>(buffer), length) << std::endl;
}