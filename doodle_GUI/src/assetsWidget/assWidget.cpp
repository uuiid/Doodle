/*
 * @Author: your name
 * @Date: 2020-11-23 11:36:09
 * @LastEditTime: 2020-11-30 14:15:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\assetsWidget\assWidget.cpp
 */
#include <src/core/coreDataManager.h>

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
#include <src/assetsWidget/model/assSortfilterModel.h>
#include <src/assetsWidget/model/assTableFilterModel.h>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>

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
      p_ass_info_widght_(nullptr),
      p_ass_sortfilter_model_(nullptr) {
  p_ass_layout_ = new QGridLayout(this);
  p_ass_layout_->setObjectName(QString::fromUtf8("ass_layout"));
  p_ass_layout_->setSpacing(3);
  p_ass_layout_->setContentsMargins(0, 0, 0, 0);

  p_ass_dep_model_   = new assDepModel();
  p_ass_class_model_ = new assClassModel();
  p_ass_type_model_  = new assTypeModel();
  p_ass_table_model_ = new assTableModel();

  p_ass_dep_widget_ = new assDepWidget();
  p_ass_dep_widget_->setObjectName("p_file_class_ass_widget_");
  p_ass_dep_widget_->setModel(p_ass_dep_model_);
  //搜索框
  auto k_filterLineEdit      = new QLineEdit();
  auto k_filterLineEditLabel = new QLabel();
  k_filterLineEditLabel->setText(tr("过滤"));

  //代理排序模型
  p_ass_sortfilter_model_ = new assSortfilterModel();
  p_ass_sortfilter_model_->setSourceModel(p_ass_class_model_);
  connect(k_filterLineEdit, &QLineEdit::textChanged,
          this, [=](const QString &filter) {
            p_ass_sortfilter_model_->setFilterRegularExpression(filter);
          });

  //table过滤模型
  auto p_ass_table_filter_model_ = new assTableFilterModel();
  p_ass_table_filter_model_->setSourceModel(p_ass_table_model_);

  //来源模型
  p_ass_class_widget_ = new assClassWidget();
  p_ass_class_widget_->setObjectName("p_ass_class_widget_");
  p_ass_class_widget_->setModel(p_ass_sortfilter_model_);

  p_ass_info_widght_ = new assTableWidght();
  p_ass_info_widght_->setObjectName("p_ass_table_widght_");
  p_ass_info_widght_->setModel(p_ass_table_filter_model_);

  p_ass_type_widget_ = new assTypeWidget();
  p_ass_type_widget_->setObjectName("p_ass_type_widget_");
  p_ass_type_widget_->setModel(p_ass_type_model_);
  //清除过滤器和以前的选择
  connect(p_ass_class_widget_, &assClassWidget::initEmited,
          [=]() {
            auto &mData = coreDataManager::get();
            mData.setAssInfoPtr(nullptr);
            mData.setAssInfoPtr(nullptr);
            p_ass_table_filter_model_->useFilter(filterState::notFilter);
            p_ass_type_model_->reInit();
            p_ass_table_model_->init();
          });
  //开始重新填词模型
  connect(p_ass_type_widget_, &assTypeWidget::doodleUseFilter,
          p_ass_table_filter_model_, &assTableFilterModel::useFilter);

  connect(p_ass_dep_widget_, &assDepWidget::initEmit,
          this, [=]() {
            auto &mData = coreDataManager::get();
            mData.setAssClassPtr(nullptr);
            mData.setAssInfoPtr(nullptr);
            mData.setAssTypePtr(nullptr);
            p_ass_table_model_->clear();
            p_ass_class_model_->init();
            p_ass_type_model_->reInit();
          });

  //布局
  p_ass_layout_->addWidget(p_ass_dep_widget_, 0, 0, 1, 2);
  p_ass_layout_->addWidget(k_filterLineEditLabel, 1, 0, 1, 1);
  p_ass_layout_->addWidget(k_filterLineEdit, 1, 1, 1, 1);
  p_ass_layout_->addWidget(p_ass_class_widget_, 2, 0, 1, 2);
  p_ass_layout_->addWidget(p_ass_info_widght_, 0, 2, -1, 1);
  p_ass_layout_->addWidget(p_ass_type_widget_, 0, 3, -1, 1);

  p_ass_layout_->setRowStretch(0, 1);
  p_ass_layout_->setRowStretch(1, 3);
  p_ass_layout_->setRowStretch(2, 10);
  p_ass_layout_->setColumnStretch(0, 1);
  p_ass_layout_->setColumnStretch(1, 3);
  p_ass_layout_->setColumnStretch(2, 10);
  p_ass_layout_->setColumnStretch(3, 3);

  setMinimumWidth(500);
}

void assWidght::refresh() {
  auto &mData = coreDataManager::get();
  mData.setAssDepPtr(nullptr);
  mData.setAssClassPtr(nullptr);
  mData.setAssInfoPtr(nullptr);
  mData.setAssTypePtr(nullptr);

  p_ass_dep_model_->init();
  p_ass_type_model_->init();
  p_ass_class_model_->clear();
  p_ass_table_model_->clear();
}

DOODLE_NAMESPACE_E