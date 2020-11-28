#include <src/assetsWidget/assWidget.h>

#include <src/assets/assClass.h>
#include <src/assetsWidget/model/assClassModel.h>
#include <src/assetsWidget/view/assClassWidget.h>
#include <src/assetsWidget/model/assTableModel.h>
#include <src/assetsWidget/view/assTableWidght.h>
#include <src/assetsWidget/model/assDepModel.h>
#include <src/assetsWidget/view/assDepWidget.h>
#include <src/assetsWidget/model/assTypeModel.h>
#include <src/assetsWidget/view/assTypeWidget.h>
DOODLE_NAMESPACE_S

assWidght::assWidght(QWidget *parent)
    : QWidget(parent),
      p_ass_class_widget_(nullptr),
      p_ass_type_widget_(nullptr),
      p_ass_type_model_(nullptr),
      p_ass_table_model_(nullptr),
      p_ass_class_model_(nullptr),
      p_ass_dep_model_(nullptr),
      p_ass_layout_(nullptr),
      p_ass_dep_widget_(nullptr),
      p_ass_info_widght_(nullptr) {
  p_ass_layout_ = new QHBoxLayout(this);
  p_ass_layout_->setSpacing(3);
  p_ass_layout_->setContentsMargins(0, 0, 0, 0);
  p_ass_layout_->setObjectName(QString::fromUtf8("p_ass_layout_"));

  p_ass_dep_model_ = new assDepModel(this);
  p_ass_class_model_ = new assClassModel(this);
  p_ass_type_model_ = new assTypeModel(this);
  p_ass_table_model_ = new assTableModel(this);

  p_ass_dep_widget_ = new assDepWidget();
  p_ass_dep_widget_->setObjectName("p_file_class_ass_widget_");
  p_ass_dep_widget_->setModel(p_ass_dep_model_);

  p_ass_class_widget_ = new assClassWidget();
  p_ass_class_widget_->setObjectName("p_ass_class_widget_");
  p_ass_class_widget_->setModel(p_ass_class_model_);
  connect(p_ass_dep_widget_, &assDepWidget::initEmit,
          p_ass_class_model_, &assClassModel::init);

  p_ass_info_widght_ = new assTableWidght();
  p_ass_info_widght_->setObjectName("p_ass_table_widght_");
  p_ass_info_widght_->setModel(p_ass_table_model_);
  connect(p_ass_class_widget_, &assClassWidget::initEmited,
          p_ass_table_model_, &assTableModel::init);
  connect(p_ass_dep_widget_, &assDepWidget::initEmit,
          p_ass_table_model_, &assTableModel::clear);

  p_ass_type_widget_ = new assTypeWidget();
  p_ass_type_widget_->setObjectName("p_ass_type_widget_");
  p_ass_type_widget_->setModel(p_ass_type_model_);
  connect(p_ass_type_widget_, &assTypeWidget::doodleUseFilter,
          p_ass_table_model_, &assTableModel::filter);
  connect(p_ass_class_widget_, &assClassWidget::initEmited,
          p_ass_type_model_, &assTypeModel::reInit);
  connect(p_ass_dep_widget_, &assDepWidget::initEmit,
          p_ass_type_model_, &assTypeModel::reInit);
  connect(p_ass_class_widget_, &assClassWidget::initEmited,
          p_ass_type_model_, &assTypeModel::reInit);

  auto class_ass_layout = new QVBoxLayout();
  class_ass_layout->addWidget(p_ass_dep_widget_, 1);
  class_ass_layout->addWidget(p_ass_class_widget_, 10);
  p_ass_layout_->addLayout(class_ass_layout, 4);

  //  p_ass_layout_->addWidget(p_ass_type_widget_,1);
  p_ass_layout_->addWidget(p_ass_info_widght_, 10);
  p_ass_layout_->addWidget(p_ass_type_widget_, 2);
  setMinimumWidth(500);
//  setMinimumSize(100,150);
//  setBaseSize(800,650);
}
void assWidght::refresh() {
  p_ass_dep_model_->init();
  p_ass_type_model_->init();
  p_ass_class_model_->clear();
  p_ass_table_model_->clear();

}

DOODLE_NAMESPACE_E