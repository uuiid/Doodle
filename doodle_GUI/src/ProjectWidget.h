//
// Created by teXiao on 2020/10/14.
//

#pragma once
#include "doodle_global.h"
#include <QtWidgets>
#include <QTabWidget>

DOODLE_NAMESPACE_S

class ProjectWidget : public QTabWidget{
 Q_OBJECT
 public:
  explicit ProjectWidget(QWidget *parent = nullptr);
 public slots:
  void init();

 private:
  void init_();
  void shotInitAnm(QWidget *parent);
  void assInit(QWidget *parent);
 private:
  episodesListModel* p_episodes_list_model_{};
  shotListModel* p_shot_list_model_{};
  fileTypeShotModel * p_file_type_shot_model_{};
  fileClassShotModel * p_file_class_shot_model_{};
  shotTableModel * p_shot_table_model_{};

  QHBoxLayout *p_b_box_layout_; //布局

  episodesListWidget *p_episodes_list_widget_; //集数小部件
  shotListWidget *p_shot_list_widget_; //镜头小部件
  fileClassShotWidget *p_file_class_shot_widget_;//部门小部件
  fileTypeShotWidget *p_file_type_shot_widget_;//种类小部件
  shotTableWidget *p_shot_table_widget_;//文件小部件
};
DOODLE_NAMESPACE_E

