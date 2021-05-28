//
// Created by TD on 2021/5/26.
//

#include <gtest/gtest.h>
#include <DoodleLib/DoodleLib.h>

#include <grpcpp/grpcpp.h>
class ServerTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;
};

void ServerTest::SetUp() {
  using namespace std::chrono_literals;
  std::thread{[]() { doodle::RpcServer::runServer(); }}.detach();
  std::this_thread::sleep_for(1s);
}

void ServerTest::TearDown() {
  doodle::RpcServer::stop();
}

TEST(server, start_stop) {
  using namespace std::chrono_literals;
  std::thread{[]() { doodle::RpcServerHelper::runServer(); }}.detach();
  std::this_thread::sleep_for(2s);
  doodle::RpcServerHelper::stop();
}
TEST(server, start_sercer){
  doodle::RpcServerHelper::runServer();
}

TEST_F(ServerTest, createPrj) {
  auto k_f = std::make_shared<doodle::MetadataFactory>();
  auto prj = std::make_shared<doodle::Project>("D:/","测试");
  prj->insert_into(k_f);
  std::cout << prj->getId() << std::endl;
  prj = std::make_shared<doodle::Project>("D:/","测试2");
  prj->insert_into(k_f);

//  auto& k_m = doodle::MetadataSet::Get();
//  k_m.Init();
//  ASSERT_TRUE(k_m.getAllProjects().size() == 2);
}
