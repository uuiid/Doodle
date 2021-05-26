//
// Created by TD on 2021/5/26.
//

#include <gtest/gtest.h>
#include <DoodleLib/DoodleLib.h>

TEST(server, start_stop){
  using namespace std::chrono_literals;
  std::thread{[](){doodle::RpcServer::runServer();}}.detach();
  std::this_thread::sleep_for(2s);
  doodle::RpcServer::stop();
}

