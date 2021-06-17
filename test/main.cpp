/*
 * @Author: your name
 * @Date: 2020-10-11 20:31:57
 * @LastEditTime: 2020-12-15 11:46:03
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\main.cpp
 */
#include <DoodleLib/DoodleLib.h>
#include <gtest/gtest.h>

#include <iostream>
#include <locale>
class Environment : public ::testing::Environment {
 public:
  void SetUp() override;
  void TearDown() override;
  doodle::CoreSet &set = doodle::CoreSet::getSet();
};

void Environment::SetUp() {
  ///初始化log
  doodle::Logger::doodle_initLog();
}

void Environment::TearDown() {
  doodle::CoreSet::getSet().clear();
  boost::log::core::get()->remove_all_sinks();
}

int main(int argc, char *argv[]) {
  //初始化测试环境
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new Environment);
  RUN_ALL_TESTS();
  return 0;
}
