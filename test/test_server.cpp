/*
 * @Author: your name
 * @Date: 2020-12-15 11:42:58
 * @LastEditTime: 2020-12-16 11:25:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\test_server.h
 */
#include <gtest/gtest.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <nlohmann/json.hpp>
TEST(doodleServer, client_base) {
  zmq::context_t context{1};

  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect(R"(tcp://127.0.0.1:6666)");
  nlohmann::json root;
  root["test"] = "test_message";
  root.cbegin();
  zmq::message_t message{root.dump()};
  std::cout << message << std::endl;
  socket.send(message, zmq::send_flags::none);

  zmq::message_t reply{};

  auto r_size = socket.recv(reply, zmq::recv_flags::none);

  std::cout << "size :" << r_size.value_or(0) << "\n"
            << reply << std::endl;
}

TEST(doodleServer, server_base) {
  zmq::context_t context{1};

  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect(R"(tcp://127.0.0.1:6666)");
  nlohmann::json root;
  root["test"]         = "test_message";
  root["class"]        = "filesystem";
  root["function"]     = "getInfo";
  root["body"]["path"] = "D:/tmp/DBXY_041_017_AN.mov";

  zmq::message_t message{root.dump()};
  std::cout << message << std::endl;
  socket.send(message, zmq::send_flags::none);

  zmq::message_t reply{};

  auto r_size = socket.recv(reply, zmq::recv_flags::none);

  std::cout << "size :" << r_size.value_or(0) << "\n"
            << reply << std::endl;
}

TEST(doodleServer, createFolder) {
}
