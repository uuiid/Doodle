#pragma once
#include <doodle_global.h>

DOODLE_NAMESPACE_S

class assWidght : public QWidget {
  Q_OBJECT
 public:
  explicit assWidght(QWidget *parent = nullptr);
 public slots:
  void refresh();
 private:
//这些都是资产变量
  QHBoxLayout *p_ass_layout_;
  assDepModel *p_ass_dep_model_;
  assClassModel *p_ass_class_model_;
  assTableModel *p_ass_table_model_;

  assTypeModel *p_ass_type_model_; //这个作为过滤器

  assDepWidget *p_ass_dep_widget_;
  assClassWidget *p_ass_class_widget_;
  assTypeWidget *p_ass_type_widget_;
  assTableWidght *p_ass_info_widght_;
};

DOODLE_NAMESPACE_E