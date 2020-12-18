/*
 * @Author: your name
 * @Date: 2020-12-15 11:42:58
 * @LastEditTime: 2020-12-16 11:25:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\test_server.h
 */
#include <gtest/gtest.h>
#include <boost/network/protocol.hpp>
#include <boost/filesystem.hpp>
TEST(doodleServer, client_base) {
  boost::network::http::client client{};
  // boost::network::http::client::request req("http://127.0.0.1:8000/ZhuiZhuXiaoXiang_ZL?project=dubuxiaoyao3");
  boost::network::http::client::request req("http://www.boost.org");
  req << boost::network::header("Connection", "close");
  auto response = client.get(req);
  //   std::cout << boost::network::http::headers(response.headers) << std::endl;
  // response.status_message();
  // response.status();
  // auto handle2 = boost::network::http::headers(response);
  auto headers = response.headers();
  // auto body = response.body();
  for (auto &&i : headers) {
    std::cout << i.first << ": " << i.second << std::endl;
  }
}

TEST(doodleServer, server_base) {
  boost::network::http::client client{};
  boost::network::http::client::request req("http://127.0.0.1:8000/");
  req << boost::network::header("Connection", "close");
  auto response = client.get(req);
  auto headers = response.headers();
  auto body = response.body();
  for (auto &&i : headers) {
    std::cout << i.first << ": " << i.second << std::endl;
  }
  std::cout << body << "\n"
            << std::endl;
}

TEST(doodleServer, buffer_stream) {
  // boost::filesystem::path k_path("F:/cppguide.xml");
  boost::filesystem::path k_path("F:/doodle.exe");
  boost::filesystem::ifstream stream(k_path, std::ios::in | std::ios::binary);
  std::cout << stream.is_open() << std::endl;

  stream.seekg(0, std::ios::end);
  auto length = stream.tellg();
  stream.seekg(0, std::ios::beg);
  if (length > 4096)
    length = 4096;

  char *buffer = new char[length];

  stream.read(buffer, length);
  boost::asio::const_buffer buff{buffer, sizeof(buffer)};
  std::cout << std::string(static_cast<char *>(buffer), length) << std::endl;
}