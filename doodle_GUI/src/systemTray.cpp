//
// Created by teXiao on 2020/10/19.
//

#include "systemTray.h"
#include "mainWindows.h"
#include "settingWidget.h"

#include <core_doQt.h>
#include <QApplication>

#include <QMenu>
DOODLE_NAMESPACE_S
systemTray::systemTray(mainWindows *parent) : QSystemTrayIcon(parent) {
  setToolTip(tr("doodle 文件 %1").arg("2.1"));

  auto menu = new QMenu(parent);
  auto fileSyn = new QAction(menu);
  fileSyn->setText(tr("同步文件"));

  auto prj_widght = new QAction(menu);
  prj_widght->setText(tr("项目管理器"));

  auto install = new QMenu(menu);
  install->setTitle(tr("安装插件"));
  auto install_maya_plug = new QAction(install);
  install_maya_plug->setText(tr("安装maya插件"));
  auto install_ue4_plug_prj = new QAction(install);
  install_ue4_plug_prj->setText(tr("安装ue4插件(项目)"));
  auto install_ue4_plug = new QAction(install);
  install_ue4_plug->setText(tr("安装ue4插件"));
  install->addAction(install_ue4_plug_prj);
  install->addAction(install_ue4_plug);

  setting = new QAction(menu);
  setting->setText(tr("设置"));
  auto re_user = new QAction(menu);
  re_user->setText(tr("注册"));
  auto k_exit_ = new QAction(menu);
  k_exit_->setText(tr("退出"));

  connect(fileSyn, &QAction::triggered,
          this, &systemTray::synFile);
  connect(k_exit_, &QAction::triggered,
          this, &systemTray::doodleQuery);
  connect(prj_widght,&QAction::triggered,
          parent,&mainWindows::show);
  connect(setting, &QAction::triggered,
          parent,&mainWindows::openSetting);
  menu->addAction(fileSyn);
  menu->addAction(prj_widght);
  menu->addMenu(install);
  menu->addAction(setting);
  menu->addAction(re_user);
  menu->addAction(k_exit_);

  setContextMenu(menu);
}
void systemTray::synFile() {
  auto syn = std::make_unique<doCore::ueSynArchive>();
  syn->syn(nullptr);
}

void systemTray::installMayaPlug() {

}
void systemTray::installUe4Plug(const systemTray::installModel &model) {

}
void systemTray::doodleQuery() {
  setVisible(false);
  qApp->quit();
}

DOODLE_NAMESPACE_E