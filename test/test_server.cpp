/*
 * @Author: your name
 * @Date: 2020-12-15 11:42:58
 * @LastEditTime: 2020-12-16 11:25:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\test_server.h
 */
#include <gtest/gtest.h>
#include <fstream>
#include <boost/filesystem.hpp>
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
  root["test"]            = "test_message";
  root["class"]           = "filesystem";
  root["function"]        = "getInfo";
  root["body"]["path"]    = "/cache/BiManMan_UE4.7z";
  root["body"]["project"] = "test";

  zmq::message_t message{root.dump()};
  std::cout << message << std::endl;
  socket.send(message, zmq::send_flags::none);

  zmq::message_t reply{};

  auto r_size = socket.recv(reply, zmq::recv_flags::none);

  std::cout << "size :" << r_size.value_or(0) << "\n"
            << reply << std::endl;
}

TEST(doodleServer, createFolder) {
  zmq::context_t context{1};

  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect(R"(tcp://127.0.0.1:6666)");

  for (size_t i = 0; i < 5; i++) {
    nlohmann::json root;
    root["test"]            = "test_message";
    root["class"]           = "filesystem";
    root["function"]        = "createFolder";
    root["body"]["path"]    = "/cache/tmp/";
    root["body"]["project"] = "test";

    zmq::message_t message{root.dump()};
    std::cout << message << std::endl;
    socket.send(message, zmq::send_flags::none);

    zmq::message_t reply{};

    auto r_size = socket.recv(reply, zmq::recv_flags::none);

    std::cout << "size :" << r_size.value_or(0) << "\n"
              << reply << std::endl;
  }
}

TEST(doodleServer, downFile) {
  zmq::context_t context{1};

  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect(R"(tcp://127.0.0.1:6666)");
  nlohmann::json root;

  zmq::multipart_t k_muMsg{};

  root["test"]            = "test_message";
  root["class"]           = "filesystem";
  root["function"]        = "getInfo";
  root["body"]["path"]    = "/cache/BiManMan_UE4.7z";
  root["body"]["project"] = "test";

  k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
  k_muMsg.send(socket);

  k_muMsg.recv(socket);


  root = nlohmann::json::parse(k_muMsg.pop().to_string());
  std::fstream file{};
  file.open("D:/tmp/test_server/test.7z", std::ios::out | std::ios::binary);

  std::cout << root << std::endl;
  if (root["status"] == "ok") {
    if (!root["body"]["isDirectory"] && root["body"]["exists"]) {
      auto size = root["body"]["size"].get<size_t>();

      const off_t off{8000000};
      const uint64_t period{size / off};

      for (size_t i = 0; i <= period; ++i) {
        auto end = std::min(off * (i + 1), size);
        root.clear();
        root["test"]            = "test_message";
        root["class"]           = "filesystem";
        root["function"]        = "down";
        root["body"]["path"]    = "/cache/BiManMan_UE4.7z";  // "/cache/BiManMan_UE4.7z";
        root["body"]["project"] = "test";
        root["body"]["start"]   = i * off;
        root["body"]["end"]     = end;
        k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
        k_muMsg.send(socket);
        k_muMsg.recv(socket);
        // k_muMsg.pop();  //弹出空帧
        root = nlohmann::json::parse(k_muMsg.pop().to_string());
        ASSERT_TRUE(root["status"] == "ok");
        auto d = k_muMsg.pop();
        file.write((char*)d.data(), d.size());
      }

      // uint64_t pipeline{3};
      // uint64_t off_size{0};
      // while (true) {
      //   while (pipeline) {
      //     auto end = std::min(off * (off_size + 1), size);
      //     root.clear();
      //     root["test"]            = "test_message";
      //     root["class"]           = "filesystem";
      //     root["function"]        = "down";
      //     root["body"]["path"]    = "/cache/BiManMan_UE4.7z";  // "/cache/BiManMan_UE4.7z";
      //     root["body"]["project"] = "test";
      //     root["body"]["start"]   = off_size * off;
      //     root["body"]["end"]     = end;
      //     // k_muMsg.push_back(zmq::message_t{});  //推入空帧
      //     k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
      //     k_muMsg.send(socket);
      //     ++off_size;
      //     --pipeline;
      //   }
      //   ++pipeline;
      //   k_muMsg.recv(socket);
      //   // k_muMsg.pop();  //弹出空帧
      //   root = nlohmann::json::parse(k_muMsg.pop().to_string());
      //   ASSERT_TRUE(root["error"] == "ok");
      //   auto d            = k_muMsg.pop();
      //   const auto k_size = d.size();
      //   size              = size - k_size;

      //   file.write((char*)d.data(), d.size());
      //   if (!size) break;
      // }
    }
  }
}

TEST(doodleServer, updataFile) {
  zmq::context_t context{1};

  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect(R"(tcp://127.0.0.1:6666)");
  nlohmann::json root;

  zmq::multipart_t k_muMsg{};

  root["test"]            = "test_message";
  root["class"]           = "filesystem";
  root["function"]        = "getInfo";
  root["body"]["path"]    = "/cache/tset_updata_server.7z";
  root["body"]["project"] = "test";

  k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
  k_muMsg.send(socket);

  k_muMsg.recv(socket);

  root = nlohmann::json::parse(k_muMsg.pop().to_string());
  std::fstream file{};
  file.open("D:/tmp/test_server/test.7z", std::ios::in | std::ios::binary);
  if (root["status"] == "ok") {
    if (true /* !root["body"]["exists"] */) {
      auto size = boost::filesystem::file_size("D:/tmp/test_server/test.7z");

      const off_t off{8000000};
      const uint64_t period{size / off};

      for (size_t i = 0; i <= period; ++i) {
        auto end = std::min(off * (i + 1), size);
        root.clear();
        root["class"]           = "filesystem";
        root["function"]        = "update";
        root["body"]["path"]    = "/cache/tset_updata_server.7z";
        root["body"]["project"] = "test";
        root["body"]["start"]   = i * off;
        root["body"]["end"]     = end;
        k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));

        auto data = zmq::message_t{off};
        file.seekg(i * off);
        file.read((char*)data.data(), off);
        k_muMsg.push_back(std::move(data));

        k_muMsg.send(socket);
        k_muMsg.recv(socket);
        // k_muMsg.pop();  //弹出空帧
        root = nlohmann::json::parse(k_muMsg.pop().to_string());
        ASSERT_TRUE(root["status"] == "ok");
      }
    }
  }
}

TEST(doodleServer, rename) {
  zmq::context_t context{1};

  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect(R"(tcp://127.0.0.1:6666)");
  nlohmann::json root;

  zmq::multipart_t k_muMsg{};

  root["test"]                      = "test_message";
  root["class"]                     = "filesystem";
  root["function"]                  = "rename";
  root["body"]["source"]["path"]    = "/cache/tset_updata_server.7z";
  root["body"]["source"]["project"] = "test";
  root["body"]["target"]["path"]    = "/cache/tmp/tset_updata_server.7z";
  root["body"]["target"]["project"] = "test";

  k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
  k_muMsg.send(socket);

  k_muMsg.recv(socket);

  root = nlohmann::json::parse(k_muMsg.pop().to_string());
  std::cout << root << std::endl;
  ASSERT_TRUE(root["status"] == "ok");
}

TEST(doodleServer, list) {
  zmq::context_t context{1};

  zmq::socket_t socket{context, zmq::socket_type::req};
  socket.connect(R"(tcp://127.0.0.1:6666)");
  nlohmann::json root;

  zmq::multipart_t k_muMsg{};

  root["test"]            = "test_message";
  root["class"]           = "filesystem";
  root["function"]        = "list";
  root["body"]["path"]    = "/cache/";
  root["body"]["project"] = "test";

  k_muMsg.push_back(std::move(zmq::message_t{root.dump()}));
  k_muMsg.send(socket);

  k_muMsg.recv(socket);

  root = nlohmann::json::parse(k_muMsg.pop().to_string());
  for (auto&& it : root["body"]) {
    std::cout << it << std::endl;
  }
  std::cout << root["status"] << std::endl;
  ASSERT_TRUE(root["status"] == "ok");
}