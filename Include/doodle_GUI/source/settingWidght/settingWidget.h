//
// Created by teXiao on 2020/10/19.
//

#pragma once
#include <doodle_GUI/doodle_global.h>

#include <loggerlib/Logger.h>
#include <QWidget>
#include <corelib/core/coreset.h>

class QListWidget;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QComboBox;

DOODLE_NAMESPACE_S
class ProjectModel;
class SettingWidget : public QWidget {
  Q_OBJECT
 public:
  explicit SettingWidget(QWidget *parent = nullptr);

  static SettingWidget *Get();

 public Q_SLOTS:

  void setInit();
  void save();
  void setProject(int index);
  void addPriject();
  void deleteProject();

 protected:
  void closeEvent(QCloseEvent *event) override;

 private:
  coreSet &p_set_ = coreSet::getSet();

  QComboBox *p_dep_text;
  QLineEdit *p_user_text;
  QPushButton *p_save;
  QLineEdit *p_ue_path;
  QLineEdit *p_ue_version;
  QSpinBox *p_ue_shot_start;
  QSpinBox *p_ue_shot_end;

  ProjectModel *p_project_model;

  QComboBox *p_project_list;
};

DOODLE_NAMESPACE_E
