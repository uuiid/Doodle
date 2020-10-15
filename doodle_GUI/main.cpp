#include "src/coreset.h"

//测试时导入
#include "src/mainWindows.h"
#include "resource/DarkStyle.h"

#include "Logger.h"
//必要导入
#include <QApplication>
#include <QTextCodec>
#include <iostream>

int main(int argc, char *argv[]) {
  QApplication q_application(argc, argv);

  //初始化log
  Logger::doodle_initLog();

//  //设置本地编码
//  QTextCodec *codec = QTextCodec::codecForName("GBK");
//  QTextCodec::setCodecForLocale(codec);
  //设置主题
  QApplication::setStyle(new DarkStyle);

  doCore::coreSet &set = doCore::coreSet::getCoreSet();
  set.init();

  doodle::mainWindows main_doodle = doodle::mainWindows();
  main_doodle.show();

  return QApplication::exec();
}