/*
 * @Author: your name
 * @Date: 2020-10-11 20:31:57
 * @LastEditTime: 2020-12-15 11:46:03
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\test\main.cpp
 */
#include <core_doQt.h>
#include <src/core/coresql.h>
#include <Logger.h>

#include <QCoreApplication>
#include <QTextCodec>
#include <QSqlQuery>
#include <QTextStream>

#include <gtest/gtest.h>
#include <iostream>
class Environment : public ::testing::Environment {
 public:
  void SetUp() override;
  void TearDown() override;
  doCore::coreSet &set = doCore::coreSet::getSet();
  doCore::coreSql &sql = doCore::coreSql::getCoreSql();
};

void Environment::SetUp() {
  set.init();
  // sql.initDB(set.getIpMysql(), "test_db");
}

void Environment::TearDown() {}

int main(int argc, char *argv[]) {
  //初始化log
  Logger::doodle_initLog();

  //创建qt必要的运行事件循环
  QCoreApplication app(argc, argv);
  //设置本地编码
  QTextCodec *codec = QTextCodec::codecForName("GBK");
  QTextCodec::setCodecForLocale(codec);

  //初始化测试环境
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new Environment);
  RUN_ALL_TESTS();
  boost::log::core::get()->remove_all_sinks();
  return 0;
}
