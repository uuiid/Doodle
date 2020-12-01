/*
 * @Author: your name
 * @Date: 2020-12-01 15:47:51
 * @LastEditTime: 2020-12-01 19:17:58
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \tools\src\main.cpp
 */
//
// Created by teXiao on 2020/12/1.
//

#include "main.h"
#include <sourePy.cpp>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>

#include <QtWidgets/QFileDialog>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QThreadPool>
//必要导入
#include <QApplication>

#include <iostream>

#include <boost/process.hpp>

#include <Windows.h>
namespace doodle {
mayaAbcExport::mayaAbcExport(QWidget *parent)
    : QWidget(parent),
      p_lineEdit_(new QLineEdit()),
      p_get_path_button_(new QPushButton()),
      p_export_button_(new QPushButton()),
      p_dir(),
      p_pyPath_(QDir::tempPath() + "/abc_XXXXXX.py") {
  auto layout = new QGridLayout(this);
  p_lineEdit_->setText(QString{});
  p_get_path_button_->setText(tr("选择路径"));
  p_export_button_->setText(tr("导出"));

  layout->addWidget(p_lineEdit_, 0, 0, 1, 1);
  layout->addWidget(p_get_path_button_, 0, 1, 1, 1);
  layout->addWidget(p_export_button_, 1, 0, 1, 2);

  connect(p_get_path_button_, &QPushButton::clicked,
          this, &mayaAbcExport::openDialogGetDir);
  connect(p_export_button_, &QPushButton::clicked,
          this, &mayaAbcExport::exportFbxFile);
  connect(p_lineEdit_, &QLineEdit::textChanged,
          this, &mayaAbcExport::setDirString);
  setWindowTitle(tr("导出abc文件"));
  if (p_pyPath_.open()) {
    p_pyPath_.write(pyFile.toUtf8());
    p_pyPath_.close();
  }
  resize(500, 50);
}

void mayaAbcExport::openDialogGetDir() {
  auto dir = QFileDialog::getExistingDirectory(this, tr("选择maya文件所在路径"));
  if (dir.isEmpty()) return;
  p_lineEdit_->setText(dir);
  p_dir = dir;
}

void mayaAbcExport::setDirString(const QString &text) {
  p_dir = text;
}

void mayaAbcExport::exportFbxFile() {
  qDebug() << p_dir;
  QString com_arg("mayapy.exe %1 --name=%2 --path=%3 --exportpath=%4 --suffix=%5");
  com_arg = com_arg.arg(p_pyPath_.fileName());

  auto fileList = p_dir.entryInfoList({"*.ma", "*.mb"});
  for (auto &file : fileList) {
    auto com = com_arg
                   .arg(file.baseName())                                    //name
                   .arg(file.dir().absolutePath())                          //path
                   .arg(file.dir().absolutePath() + "/" + file.baseName())  //exportpath
                   .arg("." + file.suffix());                               //suffix
    qDebug() << com;
    QThreadPool::globalInstance()->start(new runExport(com));
  }
}

runExport::runExport(const QString &command)
    : QRunnable(),
      p_command_(command) {
}

void runExport::run() {
  auto env = boost::this_process::environment();
  // auto run = QProcess();
  // auto env = QProcessEnvironment::systemEnvironment();
  // if (QDir(R"(C:\Program Files\Autodesk\Maya2018\bin\)").exists())
  //   env.insert("PATH", R"(C:\Program Files\Autodesk\Maya2018\bin\)");
  // else if (QDir(R"(C:\Program Files\Autodesk\Maya2019\bin\)").exists())
  //   env.insert("PATH", R"(C:\Program Files\Autodesk\Maya2019\bin\)");
  // else if (QDir(R"(C:\Program Files\Autodesk\Maya2020\bin\)").exists())
  //   env.insert("PATH", R"(C:\Program Files\Autodesk\Maya2020\bin\)");
  // run.setProcessEnvironment(env);
  // run.start(p_command_);
  // run.waitForFinished(-1);
  auto str_path = std::string{};
  if (QDir(R"(C:\Program Files\Autodesk\Maya2018\bin\)").exists()) {
    str_path = R"(C:\Program Files\Autodesk\Maya2018\bin\)";
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2018\bin\)";
  } else if (QDir(R"(C:\Program Files\Autodesk\Maya2019\bin\)").exists()) {
    str_path = R"(C:\Program Files\Autodesk\Maya2019\bin\)";
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2019\bin\)";
  } else if (QDir(R"(C:\Program Files\Autodesk\Maya2020\bin\)").exists()) {
    str_path = R"(C:\Program Files\Autodesk\Maya2020\bin\)";
    env["PATH"] += R"(C:\Program Files\Autodesk\Maya2020\bin\)";
  }

  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));

  //使用windowsIPA创建子进程
  CreateProcess(
      NULL,
      (char *)p_command_.toStdString().c_str(),
      NULL,
      NULL,
      false,
      0,  //CREATE_NEW_CONSOLE
      NULL,
      str_path.c_str(),  //R"(C:\Program Files\Autodesk\Maya2018\bin\)"
      &si,
      &pi);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}
}  // namespace doodle

int main(int argc, char *argv[]) {
  QApplication q_application(argc, argv);
  //实例化窗口
  auto window = doodle::mayaAbcExport();
  window.show();
  return q_application.exec();
}