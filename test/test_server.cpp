/*
 * @Author: your name
 * @Date: 2020-12-15 11:42:58
 * @LastEditTime: 2020-12-15 11:56:37
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\test_server.h
 */
#include <gtest/gtest.h>
#include <boost/network/protocol.hpp>

TEST(doodleServer, server_base) {
  boost::network::http::client client;
  boost::network::http::client::request req("http://127.0.0.1:66636/");
  auto response = client.get(req);
  //   std::cout << boost::network::http::headers(response.headers) << std::endl;
  std::cout << body(response) << std::endl;
}