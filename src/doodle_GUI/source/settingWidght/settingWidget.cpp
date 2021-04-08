//
// Created by teXiao on 2020/10/19.
//

#include <doodle_GUI/source/settingWidght/settingWidget.h>
#include <loggerlib/Logger.h>

#include <corelib/core_Cpp.h>
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

  auto p_dep_label = new QLabel();
  p_dep_label->setText(tr("部门:"));
  p_dep_text = new QComboBox();
  p_dep_text->addItems({"Executive",
                        "Light", "VFX", "modle", "rig", "Anm", "direct", "paint"});
  auto p_dep_layout = new QHBoxLayout();
  p_dep_layout->addWidget(p_dep_label);
  p_dep_layout->addWidget(p_dep_text);

  auto p_user_label = new QLabel();
  p_user_label->setText(tr("用户名:"));
  p_user_text        = new QLineEdit();
  auto p_user_layout = new QHBoxLayout();
  p_user_layout->addWidget(p_user_label);
  p_user_layout->addWidget(p_user_text);

  auto p_syn_label = new QLabel();
  p_syn_label->setText(tr("同步目录:"));
  p_syn_text        = new QLineEdit();
  auto p_syn_layout = new QHBoxLayout();
  p_syn_layout->addWidget(p_syn_label);
  p_syn_layout->addWidget(p_syn_text);

  auto p_prj_label = new QLabel();
  p_prj_label->setText(tr("项目名称:"));
  p_prj_text        = new QComboBox();
  auto p_prj_layout = new QHBoxLayout();
  p_prj_layout->addWidget(p_prj_label);
  p_prj_layout->addWidget(p_prj_text);

  auto p_save = new QPushButton();
  p_save->setText(tr("保存"));
  connect(p_save, &QPushButton::clicked,
          this, &settingWidget::save);

  auto p_same_layout = new QVBoxLayout();
  p_same_layout->addLayout(p_dep_layout);
  p_same_layout->addLayout(p_user_layout);
  p_same_layout->addLayout(p_prj_layout);
  p_same_layout->addLayout(p_syn_layout);

  p_same_layout->addWidget(p_save);

  p_layout->addLayout(p_same_layout);

  setInit();

  connect(p_dep_text, &QComboBox::currentTextChanged,
          this, &settingWidget::setDepartment);
  connect(p_user_text, &QLineEdit::textChanged,
          this, &settingWidget::setUser);
  connect(p_syn_text, &QLineEdit::textChanged,
          this, &settingWidget::setLocaleSynPath);
  connect(p_prj_text, &QComboBox::currentTextChanged,
          this, &settingWidget::setProject);
}
void settingWidget::setInit() {
}
void settingWidget::setDepartment(const QString &dep) {
}
void settingWidget::setUser(const QString &user) {
}
void settingWidget::setLocaleSynPath(const QString &path) {
}
void settingWidget::seteps(int eps) {
  // if (eps <= 0) return;
  // p_set_.setSyneps(eps);
  // auto eps_ptr =  episodes::find_by_eps(eps);
  // p_syn_locale_path->clear();
  // p_syn_sever_path->clear();

  // if (eps_ptr) {
  //   auto syneps =  synData::getAll(eps_ptr);

  //   auto tmplist = syneps->getSynDir();
  //   for (auto &&x : tmplist) {
  //     p_syn_locale_path->addItem(DOTOS(x.local.generic_string()));
  //     p_syn_sever_path->addItem(DOTOS(x.server.generic_string()));
  //   }
  // }
}
void settingWidget::setProject(const QString &prj) {
}
void settingWidget::closeEvent(QCloseEvent *event) {
  save();
  QWidget::closeEvent(event);
}
void settingWidget::save() {
  p_set_.writeDoodleLocalSet();
}
DOODLE_NAMESPACE_E