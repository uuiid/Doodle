/*
 * @Author: your name
 * @Date: 2020-11-16 19:05:15
 * @LastEditTime: 2020-11-27 17:59:40
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\main.cpp
 */
#include "src/coreset.h"

#include "src/mainWindows.h"

#include "Logger.h"
//必要导入
#include <QApplication>
#include <QTextCodec>
#include <iostream>
#include <QtCore/QFile>
int main(int argc, char *argv[]) {
  QApplication q_application(argc, argv);

  //初始化log
  Logger::doodle_initLog();

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
  doCore::coreSet &set = doCore::coreSet::getSet();
  set.init();
  QApplication::setQuitOnLastWindowClosed(false);

  auto mainWin = doodle::mainWindows();
  mainWin.show();

  return q_application.exec();
}