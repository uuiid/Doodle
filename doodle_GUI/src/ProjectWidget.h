//
// Created by teXiao on 2020/10/14.
//

#pragma once
#include "doodle_global.h"

#include <QTabWidget>

DOODLE_NAMESPACE_S

class ProjectWidget : public QTabWidget {
 Q_OBJECT
 public:
  explicit ProjectWidget(QWidget *parent = nullptr);

 public slots:
  void refresh();

 private:
  void init_();
  void shotInitAnm(QWidget *parent);
  void assInit(QWidget *parent);
 private:
//这些都是镜头变量
  QHBoxLayout *p_shot_layout_; //布局
  episodesListModel *p_episodes_list_model_;//集数模型
  shotListModel *p_shot_list_model_;//镜头模型
  fileTypeShotModel *p_file_type_shot_model_;//类型模型
  fileClassShotModel *p_file_class_shot_model_;//种类模型

  shotTableModel *p_shot_table_model_;//镜头信息模型

  episodesListWidget *p_episodes_list_widget_; //集数小部件
  shotListWidget *p_shot_list_widget_; //镜头小部件
  fileClassShotWidget *p_file_class_shot_widget_;//部门小部件
  fileTypeShotWidget *p_file_type_shot_widget_;//种类小部件
  shotTableWidget *p_shot_table_widget_;//文件小部件

//这些都是资产变量
  QHBoxLayout *p_ass_layout_;
  AssDepModel *p_file_class_ass_model_;
  assClassModel *p_ass_class_model_;
  fileTypeAssModel *p_file_type_ass_model_;
  assTableModel *p_ass_table_model_;

  fileClassAssWidget *p_file_class_ass_widget_;
  assClassWidget *p_ass_class_widget_;
  fileTypeAssWidget *p_file_type_ass_widget_;
  assTableWidght *p_ass_table_widght_;
};
DOODLE_NAMESPACE_E

