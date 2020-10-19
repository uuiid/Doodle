//
// Created by teXiao on 2020/10/19.
//

#include "systemTray.h"
#include <QMenu>

DOODLE_NAMESPACE_S
systemTray::systemTray(QObject *parent) : QSystemTrayIcon(parent) {
  setToolTip(tr("doodle 文件 %1").arg("2.1"));

  auto menu = new QMenu();
  auto fileSyn = new QAction(menu);
  fileSyn->setText(tr("同步文件"));

  auto prj_widght = new QAction(menu);
  prj_widght->setText(tr("项目管理器"));

  auto install = new QMenu(menu);
  auto install_maya_plug = new QAction(install);
  install_maya_plug->setText(tr("安装maya插件"));
  auto install_ue4_plug_prj = new QAction(install);
  install_ue4_plug_prj->setText(tr("安装ue4插件(项目)"));
  auto install_ue4_plug = new QAction(install);
  install_ue4_plug->setText(tr("安装ue4插件"));
  install->addAction(install_ue4_plug_prj);
  install->addAction(install_ue4_plug);

  auto setting = new QAction(menu);
  setting->setText(tr("打开设置"));
  auto re_user = new QAction(menu);
  re_user->setText(tr("注册"));
  auto k_exit_ = new QAction(menu);
  k_exit_->setText(tr("退出"));

  menu->addAction(fileSyn);
  menu->addAction(prj_widght);
  menu->addMenu(install);
  menu->addAction(setting);
  menu->addAction(re_user);
  menu->addAction(k_exit_);
}
void systemTray::synFile() {

}
void systemTray::openPrj() {

}
void systemTray::installMayaPlug() {

}
void systemTray::installUe4Plug(const systemTray::installModel &model) {

}
void systemTray::openSetting() {

}
void systemTray::register_user() {

}
void systemTray::doodleQuery() {

}

DOODLE_NAMESPACE_E