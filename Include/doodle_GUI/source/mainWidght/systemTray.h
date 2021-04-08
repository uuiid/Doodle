/*
 * @Author: your name
 * @Date: 2020-10-19 19:24:47
 * @LastEditTime: 2020-11-27 11:27:23
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\systemTray.h
 */
//
// Created by teXiao on 2020/10/19.
//
#pragma once
#include <doodle_GUI/doodle_global.h>
#include <QSystemTrayIcon>

DOODLE_NAMESPACE_S

class systemTray : public QSystemTrayIcon {
  Q_OBJECT
 public:
  explicit systemTray(mainWindows *parent = nullptr);
  enum class installModel { peject,
                            exeFile };

 Q_SIGNALS:
  void quit();

 private Q_SLOTS:
  void installMayaPlug();
  void installUe4Plug(const installModel &model);
  void doodleQuery();
  void showRigister();
  void upDoodle();

 private:
  QAction *setting;
};
DOODLE_NAMESPACE_E
