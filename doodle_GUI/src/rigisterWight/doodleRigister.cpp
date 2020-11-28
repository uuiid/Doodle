//
// Created by teXiao on 2020/11/25.
//

#include "doodleRigister.h"
#include <core_doQt.h>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/qpushbutton.h>
DOODLE_NAMESPACE_S
doodleRigister::doodleRigister(QWidget *parent)
    : QWidget(parent),
      sub_butten(new QPushButton()),
      textEdit(new QLineEdit()),
      userList(doCore::coreSet::getAllUser()) {
  auto layer = new QVBoxLayout(this);
  auto layer2 = new QHBoxLayout();
  auto quit_butten = new QPushButton();
  sub_butten->setText(tr("注册"));
  sub_butten->setEnabled(false);
  quit_butten->setText(tr("取消"));


  connect(textEdit, &QLineEdit::textChanged,
          this, &doodleRigister::setButten);
  connect(quit_butten, &QPushButton::clicked,
          this, [=]() { this->close(); });
  connect(sub_butten, &QPushButton::clicked,
          this, &doodleRigister::subUaer);
  layer2->addWidget(sub_butten);
  layer2->addWidget(quit_butten);

  layer->addWidget(textEdit);
  layer->addLayout(layer2);


}
void doodleRigister::setButten(const QString &text) {
  if ((std::find(userList.begin(), userList.end(), text.toStdString()) != userList.end())
  || text.isEmpty() || text.isEmpty()) {
    sub_butten->setEnabled(false);
  } else {
    sub_butten->setEnabled(true);
  }
}
void doodleRigister::subUaer() {
  auto user = textEdit->text().toStdString();
  if (user.empty()) return;
  doCore::coreSet::subUser(user);
  close();
}

DOODLE_NAMESPACE_E
