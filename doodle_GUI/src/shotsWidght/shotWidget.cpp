#include "shotWidget.h"

#include <core_doQt.h>

#include <src/shotsWidght/model/shotEpsListModel.h>
#include <src/shotsWidght/model/shotListModel.h>
#include <src/shotsWidght/model/shotTableModel.h>
#include <src/shotsWidght/model/shotTypeModel.h>
#include <src/shotsWidght/model/shotClassModel.h>
#include <src/shotsWidght/veiw/shotEpsListWidget.h>
#include <src/shotsWidght/veiw/shotListWidget.h>
#include <src/shotsWidght/veiw/shotTableWidget.h>
#include <src/shotsWidght/veiw/shotClassWidget.h>
#include <src/shotsWidght/veiw/shotTypeWidget.h>
DOODLE_NAMESPACE_S

shotWidget::shotWidget(QWidget* parent)
    : QWidget(parent),
      p_shot_table_model_(),
      p_shot_type_widget_(),
      p_shot_class_widget_(),
      p_shot_list_model_(),
      p_episodes_list_model_(),
      p_shot_table_widget_(),
      p_shot_list_widget_(),
      p_episodes_list_widget_(),
      p_shot_class_model_(),
      p_shot_layout_(),
      p_shot_type_model_() {
  //创建模型
  p_episodes_list_model_ = new shotEpsListModel(this);
  p_episodes_list_model_->setObjectName(QString::fromUtf8("p_episodes_list_model_"));
  p_shot_list_model_  = new shotListModel(this);
  p_shot_type_model_  = new shotTypeModel(this);
  p_shot_class_model_ = new shotClassModel(this);
  p_shot_table_model_ = new shotTableModel(this);

  //设置基本布局
  p_shot_layout_ = new QHBoxLayout(this);
  p_shot_layout_->setSpacing(3);
  p_shot_layout_->setContentsMargins(0, 0, 0, 0);
  p_shot_layout_->setObjectName(QString::fromUtf8("p_shot_layout_"));

  //创建集数小部件
  p_episodes_list_widget_ = new shotEpsListWidget();
  p_episodes_list_widget_->setObjectName(
      QString::fromUtf8("p_episodes_list_widget_"));
  p_episodes_list_widget_->setModel(p_episodes_list_model_);

  //添加镜头小部件
  p_shot_list_widget_ = new shotListWidget();
  p_shot_list_widget_->setObjectName(QString::fromUtf8("p_shot_list_widget_"));
  p_shot_list_widget_->setModel(p_shot_list_model_);
  //连接集数和镜头的更新
  connect(p_episodes_list_widget_, &shotEpsListWidget::initEmit,
          p_shot_list_model_, &shotListModel::init);

  //添加部门小部件和种类小部件的布局
  auto layout_1 = new QVBoxLayout();
  layout_1->setSpacing(3);
  layout_1->setContentsMargins(0, 0, 0, 0);
  layout_1->setObjectName(QString::fromUtf8("layout_1"));
  //添加shotTable
  p_shot_table_widget_ = new shotTableWidget();
  p_shot_table_widget_->setObjectName("p_shot_table_widget_");
  p_shot_table_widget_->setModel(p_shot_table_model_);
  connect(p_episodes_list_widget_, &shotEpsListWidget::initEmit,
          p_shot_table_model_, &shotTableModel::init);
  connect(p_shot_list_widget_, &shotListWidget::initEmit,
          p_shot_table_model_, &shotTableModel::init);

  //添加部门小部件
  p_shot_class_widget_ = new shotClassWidget();
  p_shot_class_widget_->setObjectName(QString::fromUtf8("p_shot_class_widget_"));
  p_shot_class_widget_->setModel(p_shot_class_model_);
  layout_1->addWidget(p_shot_class_widget_);

  connect(p_shot_class_widget_, &shotClassWidget::doodleUseFilter,
          p_shot_table_model_, &shotTableModel::filter);
  connect(p_episodes_list_widget_, &shotEpsListWidget::initEmit,
          p_shot_class_model_, &shotClassModel::reInit);
  connect(p_shot_list_widget_, &shotListWidget::initEmit,
          p_shot_class_model_, &shotClassModel::reInit);

  //添加种类小部件
  p_shot_type_widget_ = new shotTypeWidget();
  p_shot_type_widget_->setObjectName("p_shot_type_widget_");
  p_shot_type_widget_->setModel(p_shot_type_model_);
  layout_1->addWidget(p_shot_type_widget_);
  connect(p_shot_type_widget_, &shotTypeWidget::doodleUseFilter,
          p_shot_table_model_, &shotTableModel::filter);
  connect(p_episodes_list_widget_, &shotEpsListWidget::initEmit,
          p_shot_type_model_, &shotTypeModel::reInit);
  connect(p_shot_list_widget_, &shotListWidget::initEmit,
          p_shot_type_model_, &shotTypeModel::reInit);

  //将table中发出的刷新部门和种类的信号连接一下
  connect(p_shot_table_widget_, &shotTableWidget::refreshClassAndType,
          p_shot_class_model_, &shotClassModel::init);
  connect(p_shot_table_widget_, &shotTableWidget::refreshClassAndType,
          p_shot_type_model_, &shotTypeModel::init);

  //将小部件添加到布局中
  p_shot_layout_->addWidget(p_episodes_list_widget_, 2);
  p_shot_layout_->addWidget(p_shot_list_widget_, 2);
  p_shot_layout_->addWidget(p_shot_table_widget_, 10);
  //添加布局
  p_shot_layout_->addLayout(layout_1, 3);
  setMinimumWidth(500);
}
void shotWidget::refresh() {
  p_episodes_list_model_->init();
  p_shot_class_model_->init();
  p_shot_type_model_->init();

  p_shot_list_model_->clear();
  p_shot_table_model_->clear();

  try {
    auto shotClass = doCore::shotClass::getCurrentClass();
  } catch (const std::runtime_error& e) {
    auto shotClass = std::make_shared<doCore::shotClass>();
    shotClass->setclass(doCore::coreSet::getSet().getDepartmentQ());
    shotClass->insert();
    p_shot_class_model_->init();
    p_shot_type_model_->init();
    std::cerr << e.what() << '\n';
  }
}

DOODLE_NAMESPACE_E