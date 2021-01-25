/*
 * @Author: your name
 * @Date: 2020-12-12 13:21:34
 * @LastEditTime: 2020-12-15 12:00:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_server\main.cpp
 */

#include <server.h>
#include <exception>
#include <iostream>
#include <src/server.h>
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4996)
#include <grpc/grpc.h>
#include <grpc++/grpc++.h>
#pragma warning(pop)
// #include <boost/network/utils/thread_pool.hpp>

int main(int argc, char const *argv[]) try {
  std::string server{"127.0.0.1:50051"};
  doodle::fileSystem filesystem{};

  grpc::ServerBuilder builder;

  builder.AddListeningPort(server, grpc::InsecureServerCredentials());
  builder.RegisterService(&filesystem);

  std::unique_ptr<grpc::Server> s{builder.BuildAndStart()};
  s->Wait();

  return 0;
} catch (const std::exception &error) {
  std::cout << error.what() << std::endl;
  return 1;
}
