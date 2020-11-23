#pragma once
#include <doodle_global.h>

DOODLE_NAMESPACE_S

class assWidght : public QWidget {
 public:
  assWidght();
 public slots:
  void refresh();
 private:
//这些都是资产变量
  QHBoxLayout *p_ass_layout_;
  AssDepModel *p_ass_dep_model_;
  assClassModel *p_ass_class_model_;
  assTableModel *p_ass_table_model_;

  AssTypeModel *p_ass_type_model_; //这个作为过滤器

  AssDepWidget *p_ass_dep_widget_;
  assClassWidget *p_ass_class_widget_;
  AssTypeWidget *p_ass_type_widget_;
  assTableWidght *p_ass_info_widght_;
};

DOODLE_NAMESPACE_E