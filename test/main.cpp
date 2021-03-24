/*
 * @Author: your name
 * @Date: 2020-10-11 20:31:57
 * @LastEditTime: 2020-12-15 11:46:03
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\main.cpp
 */
#include <corelib/core_Cpp.h>
#include <corelib/core/coresql.h>

#include <loggerlib/Logger.h>

#include <QtWidgets/qapplication.h>

#include <gtest/gtest.h>
#include <iostream>
#include <locale>
class Environment : public ::testing::Environment {
 public:
  void SetUp() override;
  void TearDown() override;
  doodle::coreSet &set = doodle::coreSet::getSet();
  // doodle::coreSql &sql = doodle::coreSql::getCoreSql();
};

void Environment::SetUp() {
  set.init();
  // sql.initDB(set.getIpMysql(), "test_db");
}

void Environment::TearDown() {}

int main(int argc, char *argv[]) {
  // std::setlocale(LC_ALL, "");
  //初始化log
  Logger::doodle_initLog();

  //创建qt必要的运行事件循环
  QApplication app(argc, argv);

  //初始化测试环境
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new Environment);
  RUN_ALL_TESTS();
  boost::log::core::get()->remove_all_sinks();
  return 0;
}
