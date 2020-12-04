//
// Created by teXiao on 2020/10/19.
//

#include "systemTray.h"
#include "mainWindows.h"
#include "src/settingWidght/settingWidget.h"

#include <core_doQt.h>
#include <QApplication>

#include <QMenu>
#include <src/toolkit/toolkit.h>
#include <boost/format.hpp>
#include <QtWidgets/qfiledialog.h>
#include <src/rigisterWight/doodleRigister.h>
#include <src/toolkit/updataManager.h>

#include <boost/process.hpp>

#include <future>
DOODLE_NAMESPACE_S
systemTray::systemTray(mainWindows *parent) : QSystemTrayIcon(parent) {
  setToolTip(tr("doodle 文件 %1.%2.%3")
                 .arg(Doodle_VERSION_MINOR)
                 .arg(Doodle_VERSION_MAJOR)
                 .arg(Doodle_VERSION_REVIS));

  auto menu = new QMenu(parent);
  auto fileSyn = new QAction(menu);
  fileSyn->setText(tr("同步文件"));

  auto prj_widght = new QAction(menu);
  prj_widght->setText(tr("项目管理器"));

  auto install = new QMenu(menu);
  install->setTitle(tr("安装插件"));
  auto install_maya_plug = new QAction(install);
  install_maya_plug->setText(tr("安装maya插件"));
  connect(install_maya_plug, &QAction::triggered, this,
          &systemTray::installMayaPlug);

  auto install_ue4_plug_prj = new QAction(install);
  install_ue4_plug_prj->setText(tr("安装ue4插件(项目)"));
  connect(install_ue4_plug_prj, &QAction::triggered, this,
          [=]() { this->installUe4Plug(systemTray::installModel::peject); });

  auto install_ue4_plug = new QAction(install);
  install_ue4_plug->setText(tr("安装ue4插件"));
  connect(install_ue4_plug, &QAction::triggered, this,
          [=]() { this->installUe4Plug(systemTray::installModel::exeFile); });

  install->addAction(install_maya_plug);
  install->addAction(install_ue4_plug_prj);
  install->addAction(install_ue4_plug);

  setting = new QAction(menu);
  setting->setText(tr("设置"));
  auto re_user = new QAction(menu);
  re_user->setText(tr("注册"));
  connect(re_user, &QAction::triggered, this, &systemTray::showRigister);

  auto update = new QAction(menu);
  update->setText(tr("更新"));  // menu->addAction("更新");
  update->connect(update, &QAction::triggered, this, &systemTray::upDoodle);

  auto k_exit_ = new QAction(menu);
  k_exit_->setText(tr("退出"));

  connect(fileSyn, &QAction::triggered, this, &systemTray::synFile);
  connect(k_exit_, &QAction::triggered, this, &systemTray::doodleQuery);
  connect(prj_widght, &QAction::triggered, parent, &mainWindows::show);
  connect(setting, &QAction::triggered, parent, &mainWindows::openSetting);
  menu->addAction(fileSyn);
  menu->addAction(prj_widght);
  menu->addMenu(install);
  menu->addAction(setting);
  menu->addAction(update);

  menu->addSeparator();
  menu->addAction(re_user);
  menu->addAction(k_exit_);

  setContextMenu(menu);
}
void systemTray::synFile() {
  auto syn = std::make_unique<doCore::ueSynArchive>();
  syn->syn(nullptr);
}

void systemTray::installMayaPlug() {
  auto maya_plug = doCore::coreSet::getSet().program_location().parent_path() /
                   "plug/maya_plug";
  boost::format k_string{R"(+ doodle_main.py 1.1 %1%
MYMODULE_LOCATION:= %1%
PATH+:= %1%/scripts;%1%/plug-ins
PYTHONPATH+:= %1%/scripts
)"};
  k_string % maya_plug.generic_path().generic_string();
  auto docPath = doCore::coreSet::getSet().getDoc().parent_path() / "maya" /
                 "modules" / "Doodle.mod";
  if (!boost::filesystem::exists(docPath.parent_path())) {
    boost::filesystem::create_directories(docPath.parent_path());
  }

  boost::filesystem::ofstream k_out{};
  k_out.open(docPath);
  k_out << k_string.str();
  k_out.close();
}
void systemTray::installUe4Plug(const systemTray::installModel &model) {
  if (model == systemTray::installModel::exeFile) {
    toolkit::installUePath(std::string{});
  } else {
    auto path = QFileDialog::getOpenFileName(nullptr, "选择ue项目", "",
                                             "files (*.uproject)");
    if (path.isEmpty() || path.isNull()) return;
    toolkit::installUePath(path.toStdString());
  }
}
void systemTray::doodleQuery() {
  dynamic_cast<mainWindows *>(parent())->close();
  setVisible(false);
  qApp->quit();
}
void systemTray::showRigister() {
  auto rigister = new doodleRigister(nullptr);
  rigister->show();
}

void systemTray::upDoodle() {
  auto fun = std::async(std::launch::async, &toolkit::update);
  auto &manager = updataManager::get();
  manager.addQueue(fun, "正在下载中", 100);
  manager.run();
  dynamic_cast<mainWindows *>(parent())->close();
  setVisible(false);
}

DOODLE_NAMESPACE_E