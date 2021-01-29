//
// Created by teXiao on 2020/10/19.
//

#pragma once
#include "doodle_global.h"
#include "core_global.h"
#include <loggerlib/Logger.h>
#include <QWidget>
#include <corelib/core/coreset.h>
class QListWidget;

DOODLE_NAMESPACE_S

class settingWidget : public QWidget {
  Q_OBJECT
 public:
  explicit settingWidget(QWidget *parent = nullptr);

 Q_SIGNALS:
  void projectChanged();

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
  coreSet &p_set_ = coreSet::getSet();

  QComboBox *p_dep_text;
  QLineEdit *p_user_text;
  QLineEdit *p_syn_text;
  QComboBox *p_prj_text;
};

DOODLE_NAMESPACE_E
