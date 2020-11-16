//
// Created by teXiao on 2020/10/14.
//

#include <src/ProjectWidget.h>
#include <src/assClass.h>
#include <src/assClassModel.h>
#include <src/assClassWidget.h>
#include <src/AssDepModel.h>
#include <src/AssDepWidget.h>
#include <src/assTableModel.h>
#include <src/assTableWidght.h>
#include <src/AssTypeModel.h>
#include <src/AssTypeWidget.h>
#include <src/episodes.h>
#include <src/shot.h>
#include <src/shotClass.h>
#include <src/ShotClassModel.h>
#include <src/ShotClassWidget.h>
#include <src/shotEpsListModel.h>
#include <src/shotEpsListWidget.h>
#include <src/shotListModel.h>
#include <src/shotListWidget.h>
#include <src/shotTableModel.h>
#include <src/shotTableWidget.h>
#include <src/shottype.h>
#include <src/ShotTypeModel.h>
#include <src/ShotTypeWidget.h>
DOODLE_NAMESPACE_S

ProjectWidget::ProjectWidget(QWidget *parent)
    : QTabWidget(parent),
      p_shot_layout_(nullptr),
      p_episodes_list_widget_(nullptr),
      p_shot_list_widget_(nullptr),
      p_file_class_shot_widget_(nullptr),
      p_file_type_shot_widget_(nullptr),
      p_shot_table_widget_(nullptr),
      p_episodes_list_model_(nullptr),
      p_shot_list_model_(nullptr),
      p_file_class_shot_model_(nullptr),
      p_file_type_shot_model_(nullptr),
      p_shot_table_model_(nullptr),

      p_ass_dep_widget_(nullptr),
      p_ass_class_widget_(nullptr),
      p_file_type_ass_widget_(nullptr),
      p_ass_info_widght_(nullptr),
      p_file_class_ass_model_(nullptr),
      p_ass_class_model_(nullptr),
      p_file_type_ass_model_(nullptr),
      p_ass_table_model_(nullptr) {
  init_();
}
void ProjectWidget::init_() {
  auto ass_parent = new QWidget();
  assInit(ass_parent);
  addTab(ass_parent, QString("ass"));
  setTabText(0, tr("资产"));

  //添加shot页面
  auto shot_anm_parent = new QWidget();
  shotInitAnm(shot_anm_parent);
  addTab(shot_anm_parent, QString("Anm"));
  setTabText(1, tr("动画"));
}

void ProjectWidget::shotInitAnm(QWidget *parent) {
  //创建模型
  p_episodes_list_model_ = new shotEpsListModel(this);
  p_shot_list_model_ = new shotListModel(this);
  p_file_type_shot_model_ = new ShotTypeModel(this);
  p_file_class_shot_model_ = new ShotClassModel(this);
  p_shot_table_model_ = new shotTableModel(this);

  //设置基本布局
  p_shot_layout_ = new QHBoxLayout(parent);
  p_shot_layout_->setSpacing(3);
  p_shot_layout_->setContentsMargins(0, 0, 0, 0);
  p_shot_layout_->setObjectName(QString::fromUtf8("p_shot_layout_"));

  //创建集数小部件
  p_episodes_list_widget_ = new shotEpsListWidget(parent);
  p_episodes_list_widget_->setObjectName(
      QString::fromUtf8("p_episodes_list_widget_"));
  p_episodes_list_widget_->setModel(p_episodes_list_model_);

  //添加镜头小部件
  p_shot_list_widget_ = new shotListWidget(parent);
  p_shot_list_widget_->setObjectName(QString::fromUtf8("p_shot_list_widget_"));
  p_shot_list_widget_->setModel(p_shot_list_model_);
  //连接集数和镜头的更新
  connect(p_episodes_list_widget_, &shotEpsListWidget::initEmit,
          p_shot_list_model_, &shotListModel::init);

  //  //添加部门小部件
  //  p_file_class_shot_widget_ = new ShotClassWidget(parent);
  //  p_file_class_shot_widget_->setObjectName(QString::fromUtf8("p_file_class_shot_widget_"));
  //  p_file_class_shot_widget_->setModel(p_file_class_shot_model_);
  //  connect(p_shot_list_widget_, &shotListWidget::initEmit,
  //          p_file_class_shot_widget_, &ShotClassWidget::init);
  //  //连接刷新函数
  //  connect(p_episodes_list_widget_,&shotEpsListWidget::initEmit,
  //          p_file_class_shot_widget_,&ShotClassWidget::clear);
  //
  //  //添加种类小部件
  //  p_file_type_shot_widget_ = new ShotTypeWidget(parent);
  //  p_file_type_shot_widget_->setObjectName("p_file_type_shot_widget_");
  //  p_file_type_shot_widget_->setModel(p_file_type_shot_model_);
  //  connect(p_file_class_shot_widget_, &ShotClassWidget::fileClassShotEmitted,
  //          p_file_type_shot_widget_, &ShotTypeWidget::init);
  //  connect(p_episodes_list_widget_,&shotEpsListWidget::initEmit,
  //          p_file_type_shot_widget_,&ShotTypeWidget::clear);
  //  connect(p_shot_list_widget_, &shotListWidget::initEmit,
  //          p_file_type_shot_widget_, &ShotTypeWidget::clear);

  //添加shotTable
  p_shot_table_widget_ = new shotTableWidget(parent);
  p_shot_table_widget_->setObjectName("p_shot_table_widget_");
  p_shot_table_widget_->setModel(p_shot_table_model_);
  connect(p_episodes_list_widget_, &shotEpsListWidget::initEmit,
          p_shot_table_model_, &shotTableModel::clear);
  connect(p_shot_list_widget_, &shotListWidget::initEmit, p_shot_table_model_,
          &shotTableModel::init);
  //  connect(p_file_class_shot_widget_,&ShotClassWidget::fileClassShotEmitted,
  //          p_shot_table_widget_, &shotTableWidget::clear);
  //  connect(p_file_type_shot_widget_,&ShotTypeWidget::typeEmit,
  //          p_shot_table_widget_, &shotTableWidget::init);

  //将小部件添加到布局中
  p_shot_layout_->addWidget(p_episodes_list_widget_, 1);
  p_shot_layout_->addWidget(p_shot_list_widget_, 1);
  //  p_shot_layout_->addWidget(p_file_class_shot_widget_, 1);
  //  p_shot_layout_->addWidget(p_file_type_shot_widget_, 1);
  p_shot_layout_->addWidget(p_shot_table_widget_, 5);
}
void ProjectWidget::assInit(QWidget *parent) {
  p_ass_layout_ = new QHBoxLayout(parent);
  p_ass_layout_->setSpacing(3);
  p_ass_layout_->setContentsMargins(0, 0, 0, 0);
  p_ass_layout_->setObjectName(QString::fromUtf8("p_ass_layout_"));

  p_file_class_ass_model_ = new AssDepModel(this);
  p_ass_class_model_ = new assClassModel(this);
  p_file_type_ass_model_ = new AssTypeModel(this);
  p_ass_table_model_ = new assTableModel(this);

  p_ass_dep_widget_ = new AssDepWidget(parent);
  p_ass_dep_widget_->setObjectName("p_file_class_ass_widget_");
  p_ass_dep_widget_->setModel(p_file_class_ass_model_);

  p_ass_class_widget_ = new assClassWidget(parent);
  p_ass_class_widget_->setObjectName("p_ass_class_widget_");
  p_ass_class_widget_->setModel(p_ass_class_model_);
  connect(p_ass_dep_widget_, &AssDepWidget::initEmit, p_ass_class_model_,
          &assClassModel::init);

  //  p_file_type_ass_widget_ = new AssTypeWidget(parent);
  //  p_file_type_ass_widget_->setObjectName("p_file_type_ass_widget_");
  //  p_file_type_ass_widget_->setModel(p_file_type_ass_model_);
  //  connect(p_ass_class_widget_, &assClassWidget::initEmited,
  //          p_file_type_ass_widget_,&AssTypeWidget::init);
  //  connect(p_ass_dep_widget_, & AssDepWidget::fileClassEmit,
  //          p_file_type_ass_widget_,&AssTypeWidget::clear);

  p_ass_info_widght_ = new assTableWidght(parent);
  p_ass_info_widght_->setObjectName("p_ass_table_widght_");
  p_ass_info_widght_->setModel(p_ass_table_model_);
  //  connect(p_file_type_ass_widget_,&AssTypeWidget::filetypeEmited,
  //          p_ass_info_widght_, &assTableWidght::init);
  connect(p_ass_class_widget_, &assClassWidget::initEmited, p_ass_table_model_,
          &assTableModel::init);
  connect(p_ass_dep_widget_, &AssDepWidget::initEmit, p_ass_table_model_,
          &assTableModel::clear);

  auto class_ass_layout = new QVBoxLayout();
  class_ass_layout->addWidget(p_ass_dep_widget_, 1);
  class_ass_layout->addWidget(p_ass_class_widget_, 50);
  p_ass_layout_->addLayout(class_ass_layout, 2);

  //  p_ass_layout_->addWidget(p_file_type_ass_widget_,1);
  p_ass_layout_->addWidget(p_ass_info_widght_, 3);
}
void ProjectWidget::refresh() {
  p_file_class_ass_model_->init();
  p_ass_class_model_->clear();
  //  p_file_type_ass_widget_->clear();
  p_ass_table_model_->clear();

  p_episodes_list_model_->init();
  p_shot_list_model_->clear();
  //  p_file_class_shot_widget_->clear();
  //  p_file_type_shot_widget_->clear();
  p_shot_table_model_->clear();
}
DOODLE_NAMESPACE_E