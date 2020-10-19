//
// Created by teXiao on 2020/10/19.
//
#pragma once
#include "doodle_global.h"
#include "core_global.h"
#include <QSystemTrayIcon>

DOODLE_NAMESPACE_S

class systemTray : public QSystemTrayIcon {
 Q_OBJECT
 public:
  systemTray(QObject *parent = nullptr);
  enum class installModel {
    peject,
    exeFile
  };
 private slots:
  void synFile();
  void openPrj();
  void installMayaPlug();
  void installUe4Plug(const installModel &model);
  void openSetting();
  void register_user();
  void doodleQuery();
};
DOODLE_NAMESPACE_E
