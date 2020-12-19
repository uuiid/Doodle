//
// Created by teXiao on 2020/10/19.
//

#pragma once
#include "doodle_global.h"
#include "core_global.h"

#include <QWidget>
#include <src/core/coreset.h>
class QListWidget;

DOODLE_NAMESPACE_S

class settingWidget : public QWidget {
  Q_OBJECT
 public:
  explicit settingWidget(QWidget *parent = nullptr);

 public Q_SLOTS:
  void setDepartment(const QString &dep);
  void setUser(const QString &user);
  void setLocaleSynPath(const QString &path);
  void seteps(int eps);
  void setProject(const QString &prj);
  void save();
  void setInit();

 protected:
  void closeEvent(QCloseEvent *event) override;

 private:
  doCore::coreSet &p_set_ = doCore::coreSet::getSet();

  QComboBox *p_dep_text;
  QLineEdit *p_user_text;
  QLineEdit *p_syn_text;
  QSpinBox *p_eps_text;
  QComboBox *p_prj_text;
  QListWidget *p_syn_locale_path;
  QListWidget *p_syn_sever_path;
};

DOODLE_NAMESPACE_E
