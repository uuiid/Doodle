//
// Created by TD on 2021/5/26.
//

#include <gtest/gtest.h>
#include <DoodleLib/DoodleLib.h>

class ServerTest: public ::testing::Test{
  protected:
  void SetUp() override;
  void TearDown() override;

};

void ServerTest::SetUp(){
    std::thread{[](){doodle::RpcServer::runServer();}}.detach();
}

void ServerTest::TearDown(){
  doodle::RpcServer::stop();

}

TEST(server, start_stop){
  using namespace std::chrono_literals;
  std::thread{[](){doodle::RpcServer::runServer();}}.detach();
  std::this_thread::sleep_for(2s);
  doodle::RpcServer::stop();
}

TEST_F(ServerTest,createPrj){
  
}