//
// Created by teXiao on 2020/11/25.
//
#pragma once
#include <doodle_global.h>
#include <core_global.h>
class QPushButton;

DOODLE_NAMESPACE_S

class doodleRigister : public QWidget {
  Q_OBJECT
 public:
  explicit doodleRigister(QWidget* parent);
 private Q_SLOTS:
  void setButten(const QString& text);
  void subUaer();

 private:
   dstringList userList;
  QPushButton* sub_butten;
  QLineEdit* textEdit;
};

DOODLE_NAMESPACE_E