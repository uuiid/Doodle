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
  explicit ProjectWidget(QWidget* parent = nullptr);

public slots:
  void refresh();

private:
  void init_();
  void shotInitAnm(QWidget* parent);
  void assInit(QWidget* parent);
  void lightInit(QWidget* parent);
private:
  //这些都是镜头变量
  QHBoxLayout* p_shot_layout_; //布局
  shotEpsListModel* p_episodes_list_model_;//集数模型
  shotListModel* p_shot_list_model_;//镜头模型

  ShotTypeModel* p_shot_type_model_;//类型模型 这个作为过滤器
  ShotClassModel* p_shot_class_model_;//种类模型 这个作为过滤器

  shotTableModel* p_shot_table_model_;//镜头信息模型

  shotEpsListWidget* p_episodes_list_widget_; //集数小部件
  shotListWidget* p_shot_list_widget_; //镜头小部件
  ShotClassWidget* p_shot_class_widget_;//部门小部件
  ShotTypeWidget* p_shot_type_widget_;//种类小部件
  shotTableWidget* p_shot_table_widget_;//文件小部件

//这些都是资产变量
  QHBoxLayout* p_ass_layout_;
  assDepModel* p_ass_dep_model_;
  assClassModel* p_ass_class_model_;
  assTableModel* p_ass_table_model_;

  AssTypeModel* p_ass_type_model_; //这个作为过滤器

  AssDepWidget* p_ass_dep_widget_;
  assClassWidget* p_ass_class_widget_;
  AssTypeWidget* p_ass_type_widget_;
  assTableWidght* p_ass_info_widght_;
};







DOODLE_NAMESPACE_E

