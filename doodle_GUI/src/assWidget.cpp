#include <src/assWidget.h>

#include <src/assClass.h>
#include <src/assClassModel.h>
#include <src/assClassWidget.h>
#include <src/assTableModel.h>
#include <src/assTableWidght.h>
#include <src/AssDepModel.h>
#include <src/AssDepWidget.h>
#include <src/AssTypeModel.h>
#include <src/AssTypeWidget.h>
DOODLE_NAMESPACE_S

assWidght::assWidght() {
  p_ass_layout_ = new QHBoxLayout(this);
  p_ass_layout_->setSpacing(3);
  p_ass_layout_->setContentsMargins(0, 0, 0, 0);
  p_ass_layout_->setObjectName(QString::fromUtf8("p_ass_layout_"));

  p_ass_dep_model_ = new AssDepModel(this);
  p_ass_class_model_ = new assClassModel(this);
  p_ass_type_model_ = new AssTypeModel(this);
  p_ass_table_model_ = new assTableModel(this);

  p_ass_dep_widget_ = new AssDepWidget();
  p_ass_dep_widget_->setObjectName("p_file_class_ass_widget_");
  p_ass_dep_widget_->setModel(p_ass_dep_model_);

  p_ass_class_widget_ = new assClassWidget();
  p_ass_class_widget_->setObjectName("p_ass_class_widget_");
  p_ass_class_widget_->setModel(p_ass_class_model_);
  connect(p_ass_dep_widget_, &AssDepWidget::initEmit,
          p_ass_class_model_, &assClassModel::init);

  p_ass_info_widght_ = new assTableWidght();
  p_ass_info_widght_->setObjectName("p_ass_table_widght_");
  p_ass_info_widght_->setModel(p_ass_table_model_);
  connect(p_ass_class_widget_, &assClassWidget::initEmited,
          p_ass_table_model_, &assTableModel::init);
  connect(p_ass_dep_widget_, &AssDepWidget::initEmit,
          p_ass_table_model_, &assTableModel::clear);

  p_ass_type_widget_ = new AssTypeWidget();
  p_ass_type_widget_->setObjectName("p_ass_type_widget_");
  p_ass_type_widget_->setModel(p_ass_type_model_);
  connect(p_ass_type_widget_, &AssTypeWidget::doodleUseFilter,
          p_ass_table_model_, &assTableModel::filter);
  connect(p_ass_class_widget_, &assClassWidget::initEmited,
          p_ass_type_model_, &AssTypeModel::reInit);

  auto class_ass_layout = new QVBoxLayout();
  class_ass_layout->addWidget(p_ass_dep_widget_, 1);
  class_ass_layout->addWidget(p_ass_class_widget_, 20);
  p_ass_layout_->addLayout(class_ass_layout, 4);

  //  p_ass_layout_->addWidget(p_ass_type_widget_,1);
  p_ass_layout_->addWidget(p_ass_info_widght_, 10);
  p_ass_layout_->addWidget(p_ass_type_widget_, 2);
}
void assWidght::refresh() {
  p_ass_dep_model_->init();
  p_ass_type_model_->init();
  p_ass_class_model_->clear();
  p_ass_table_model_->clear();

}

DOODLE_NAMESPACE_E