/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-30 10:53:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include <corelib/core/coreset.h>

#include <doodle_GUI/source/mainWidght/mainWindows.h>
#include <loggerlib/Logger.h>
#include <corelib/filesystem/FileSystem.h>
//必要导入
#include <QApplication>
#include <QTextCodec>
#include <iostream>
#include <QtCore/QFile>
#include <QtGui/QWindow>
#include <QTextStream>

#include <exception>
// DOODLE_NAMESPACE_S
// void doodleQuitClear() {
//   coreSet::getSet().writeDoodleLocalSet();
//   boost::log::core::get()->remove_all_sinks();
// }
// DOODLE_NAMESPACE_E

int main(int argc, char *argv[]) try {
  //初始化log
  Logger::doodle_initLog();
  //初始化文件管理器
  auto filesystem = doodle::DfileSyntem::create();

  QApplication q_application(argc, argv);

  
  //  //设置本地编码
  //  QTextCodec *codec = QTextCodec::codecForName("GBK");
  //  QTextCodec::setCodecForLocale(codec);

  //设置主题
  QFile darkStyle{":qdarkstyle/style.qss"};
  if (darkStyle.exists()) {
    darkStyle.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts{&darkStyle};
    q_application.setStyleSheet(ts.readAll());
  }
  q_application.setWindowIcon(QIcon(":/resource/icon.png"));
  //初始化设置
  auto &set = doodle::coreSet::getSet();
  set.init();
  QApplication::setQuitOnLastWindowClosed(false);

  auto mainWin = std::make_unique<doodle::mainWindows>();
  mainWin->showMaximized();

  q_application.exec();
  boost::log::core::get()->remove_all_sinks();
  return 0;
} catch (const std::exception &err) {
  DOODLE_LOG_ERROR(err.what());
  doodle::coreSet::getSet().writeDoodleLocalSet();
  boost::log::core::get()->remove_all_sinks();
  return 1;
} catch (...) {
  doodle::coreSet::getSet().writeDoodleLocalSet();
  boost::log::core::get()->remove_all_sinks();
  return 1;
}