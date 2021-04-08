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


DOODLE_NAMESPACE_S

class SettingWidget : public QWidget {
  Q_OBJECT
 public:
  explicit SettingWidget(QWidget *parent = nullptr);

 Q_SIGNALS:
  void projectChanged();

 public Q_SLOTS:

  void setInit();
  void save();

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
};

DOODLE_NAMESPACE_E
