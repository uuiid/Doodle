#include "src/coreset.h"


#include "src/mainWindows.h"
#include "src/systemTray.h"
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
  //初始化设置
  doCore::coreSet &set = doCore::coreSet::getCoreSet();
  set.init();
  QApplication::setQuitOnLastWindowClosed(false);

  auto mainWin = doodle::mainWindows();

  return QApplication::exec();
}