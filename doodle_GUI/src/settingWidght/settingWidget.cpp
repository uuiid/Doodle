﻿//
// Created by teXiao on 2020/10/19.
//

#include "settingWidget.h"
#include <Logger.h>

#include <core_doQt.h>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QtCore/QDir>

DOODLE_NAMESPACE_S
settingWidget::settingWidget(QWidget *parent)
    : QWidget(parent) {
  setWindowFlags(Qt::Window);
  auto p_layout = new QHBoxLayout(this);

  auto p_dep_label = new QLabel(this);
  p_dep_label->setText(tr("部门:"));
  p_dep_text = new QComboBox(this);
  p_dep_text->addItems({"Executive",
                        "Light", "VFX", "modle", "rig", "Anm", "direct", "paint"});
  auto p_dep_layout = new QHBoxLayout();
  p_dep_layout->addWidget(p_dep_label);
  p_dep_layout->addWidget(p_dep_text);

  auto p_user_label = new QLabel(this);
  p_user_label->setText(tr("用户名:"));
  p_user_text        = new QLineEdit(this);
  auto p_user_layout = new QHBoxLayout();
  p_user_layout->addWidget(p_user_label);
  p_user_layout->addWidget(p_user_text);

  auto p_syn_label = new QLabel(this);
  p_syn_label->setText(tr("同步目录:"));
  p_syn_text        = new QLineEdit(this);
  auto p_syn_layout = new QHBoxLayout();
  p_syn_layout->addWidget(p_syn_label);
  p_syn_layout->addWidget(p_syn_text);

  auto p_eps_label = new QLabel(this);
  p_eps_text       = new QSpinBox(this);
  p_eps_label->setText(tr("同步集数:"));
  auto p_eps_layout = new QHBoxLayout();
  p_eps_layout->addWidget(p_eps_label);
  p_eps_layout->addWidget(p_eps_text);

  auto p_prj_label = new QLabel(this);
  p_prj_label->setText(tr("项目名称:"));
  p_prj_text        = new QComboBox(this);
  auto p_prj_layout = new QHBoxLayout();
  p_prj_layout->addWidget(p_prj_label);
  p_prj_layout->addWidget(p_prj_text);

  auto p_syn_exe_path = new QLabel(this);
  p_syn_exe_path->setText(tr("同步软件安装目录 : %1")
                              .arg(DOTOS( coreSet::getSet().getFreeFileSyn())));

  auto p_save = new QPushButton(this);
  p_save->setText(tr("保存"));
  connect(p_save, &QPushButton::clicked,
          this, &settingWidget::save);

  auto p_same_layout = new QVBoxLayout();
  p_same_layout->addLayout(p_dep_layout);
  p_same_layout->addLayout(p_user_layout);
  p_same_layout->addLayout(p_syn_layout);
  p_same_layout->addLayout(p_eps_layout);
  p_same_layout->addLayout(p_prj_layout);
  p_same_layout->addWidget(p_syn_exe_path);
  p_same_layout->addWidget(p_save);

  p_syn_locale_path = new QListWidget(this);
  p_syn_sever_path  = new QListWidget(this);
  p_layout->addLayout(p_same_layout);
  p_layout->addWidget(p_syn_locale_path);
  p_layout->addWidget(p_syn_sever_path);

  setInit();

  connect(p_dep_text, &QComboBox::currentTextChanged,
          this, &settingWidget::setDepartment);
  connect(p_user_text, &QLineEdit::textChanged,
          this, &settingWidget::setUser);
  connect(p_syn_text, &QLineEdit::textChanged,
          this, &settingWidget::setLocaleSynPath);
  connect(p_prj_text, &QComboBox::currentTextChanged,
          this, &settingWidget::setProject);
  connect(p_eps_text, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &settingWidget::seteps);
}
void settingWidget::setInit() {
  p_prj_text->clear();
  p_prj_text->disconnect(this);

  for (const auto &name : p_set_.getAllPrjName()) {
    p_prj_text->addItem(DOTOS(name));
  }
  p_dep_text->setCurrentText(DOTOS(p_set_.getDepartment()));
  p_user_text->setText(DOTOS(p_set_.getUser()));
  p_syn_text->setText(DOTOS(p_set_.getSynPathLocale().generic_string()));
  p_eps_text->setValue(p_set_.getSyneps());

  DOODLE_LOG_DEBUG(p_set_.getProjectname());
  auto index = p_prj_text->findText(DOTOS(p_set_.getProjectname()));

  p_prj_text->setCurrentIndex(index);
  seteps(p_set_.getSyneps());

  connect(p_prj_text, &QComboBox::currentTextChanged,
          this, &settingWidget::setProject);
}
void settingWidget::setDepartment(const QString &dep) {
  p_set_.setDepartment(dep);
}
void settingWidget::setUser(const QString &user) {
  p_set_.setUser(user);
}
void settingWidget::setLocaleSynPath(const QString &path) {
  if (path.isEmpty()) return;
  auto syn_dir = QDir(path);
  if (syn_dir.exists())
    syn_dir.mkpath(syn_dir.path());
  p_set_.setSynPathLocale(path);
}
void settingWidget::seteps(int eps) {
  if (eps <= 0) return;
  p_set_.setSyneps(eps);
  auto eps_ptr =  episodes::find_by_eps(eps);
  p_syn_locale_path->clear();
  p_syn_sever_path->clear();

  if (eps_ptr) {
    auto syneps =  synData::getAll(eps_ptr);

    auto tmplist = syneps->getSynDir();
    for (auto &&x : tmplist) {
      p_syn_locale_path->addItem(DOTOS(x.local.generic_string()));
      p_syn_sever_path->addItem(DOTOS(x.server.generic_string()));
    }
  }
}
void settingWidget::setProject(const QString &prj) {
  p_set_.setProjectname(prj);
  projectChanged();
}
void settingWidget::closeEvent(QCloseEvent *event) {
  save();
  QWidget::closeEvent(event);
}
void settingWidget::save() {
  p_set_.writeDoodleLocalSet();
}
DOODLE_NAMESPACE_E