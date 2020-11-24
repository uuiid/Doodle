#pragma once

#include <doodle_global.h>

DOODLE_NAMESPACE_S
class shotWidget : public QWidget {
 public:
  shotWidget(QWidget *parent = nullptr);
 public slots:
  void refresh();
 private:
  //这些都是镜头变量
  QHBoxLayout *p_shot_layout_; //布局
  shotEpsListModel *p_episodes_list_model_;//集数模型
  shotListModel *p_shot_list_model_;//镜头模型

  shotTypeModel *p_shot_type_model_;//类型模型 这个作为过滤器
  shotClassModel *p_shot_class_model_;//种类模型 这个作为过滤器

  shotTableModel *p_shot_table_model_;//镜头信息模型

  shotEpsListWidget *p_episodes_list_widget_; //集数小部件
  shotListWidget *p_shot_list_widget_; //镜头小部件
  shotClassWidget *p_shot_class_widget_;//部门小部件
  shotTypeWidget *p_shot_type_widget_;//种类小部件
  shotTableWidget *p_shot_table_widget_;//文件小部件
};

DOODLE_NAMESPACE_E