//
// Created by teXiao on 2020/10/14.
//

#include "src/episodes.h"
#include "src/shot.h"
#include "src/fileclass.h"
#include "src/filetype.h"

#include "episodesListWidget.h"
#include "shotListWidget.h"
#include "fileClassShotWidget.h"
#include "fileTypeShotWidget.h"
#include "shotTableWidget.h"

#include "episodesListModel.h"
#include "shotListModel.h"
#include "fileClassShotModel.h"
#include "fileTypeShotModel.h"
#include "shotTableModel.h"

#include "ProjectWidget.h"

DOODLE_NAMESPACE_S

ProjectWidget::ProjectWidget(QWidget *parent)
: QTabWidget(parent),
  p_b_box_layout_(nullptr),
  p_episodes_list_widget_(nullptr),
  p_shot_list_widget_(nullptr),
  p_file_class_shot_widget_(nullptr),
  p_file_type_shot_widget_(nullptr),
  p_shot_table_widget_(nullptr){
  init_();
}
void ProjectWidget::init_() {
  auto ass_parent = new QWidget();
  assInit(ass_parent);
  addTab(ass_parent,QString("ass"));
  setTabText(0,"ass");

  //添加shot页面
  auto shot_anm_parent = new QWidget();
  shotInitAnm(shot_anm_parent);
  addTab(shot_anm_parent,QString("Anm"));
  setTabText(1,"Anm");
}

void ProjectWidget::init() {
  p_episodes_list_widget_->init();
}
void ProjectWidget::shotInitAnm(QWidget *parent) {
  //创建模型
  p_episodes_list_model_ = new episodesListModel(this);
  p_shot_list_model_ = new shotListModel(this);
  p_file_type_shot_model_ = new fileTypeShotModel(this);
  p_file_class_shot_model_ = new fileClassShotModel(this);
  p_shot_table_model_ = new shotTableModel(this);

  //设置基本布局
  p_b_box_layout_ = new QHBoxLayout(parent);
  p_b_box_layout_->setSpacing(3);
  p_b_box_layout_->setContentsMargins(0, 0, 0, 0);
  p_b_box_layout_->setObjectName(QString::fromUtf8("p_b_box_layout_"));

  //创建集数小部件
  p_episodes_list_widget_ = new episodesListWidget(parent);
  p_episodes_list_widget_->setObjectName(QString::fromUtf8("p_episodes_list_widget_"));
  p_episodes_list_widget_->setModel(p_episodes_list_model_);

  //添加镜头小部件
  p_shot_list_widget_ = new shotListWidget(parent);
  p_shot_list_widget_->setObjectName(QString::fromUtf8("p_shot_list_widget_"));
  p_shot_list_widget_->setModel(p_shot_list_model_);
  //连接集数和镜头的更新
  connect(p_episodes_list_widget_, &episodesListWidget::episodesEmit,
          p_shot_list_widget_, &shotListWidget::init);


  //添加部门小部件
  p_file_class_shot_widget_ = new fileClassShotWidget(parent);
  p_file_class_shot_widget_->setObjectName(QString::fromUtf8("p_file_class_shot_widget_"));
  p_file_class_shot_widget_->setModel(p_file_class_shot_model_);
  connect(p_shot_list_widget_, &shotListWidget::shotEmit,
          p_file_class_shot_widget_, &fileClassShotWidget::init);
  //连接刷新函数
  connect(p_episodes_list_widget_,&episodesListWidget::episodesEmit,
          p_file_class_shot_widget_,&fileClassShotWidget::clear);

  //添加种类小部件
  p_file_type_shot_widget_ = new fileTypeShotWidget(parent);
  p_file_type_shot_widget_->setObjectName("p_file_type_shot_widget_");
  p_file_type_shot_widget_->setModel(p_file_type_shot_model_);
  connect(p_file_class_shot_widget_, &fileClassShotWidget::fileClassShotEmitted,
          p_file_type_shot_widget_, &fileTypeShotWidget::init);
  connect(p_episodes_list_widget_,&episodesListWidget::episodesEmit,
          p_file_type_shot_widget_,&fileTypeShotWidget::clear);
  connect(p_shot_list_widget_, &shotListWidget::shotEmit,
          p_file_type_shot_widget_, &fileTypeShotWidget::clear);

  //添加shotTable
  p_shot_table_widget_ = new shotTableWidget(parent);
  p_shot_table_widget_->setObjectName("p_shot_table_widget_");
  p_shot_table_widget_->setModel(p_shot_table_model_);
  connect(p_file_type_shot_widget_,&fileTypeShotWidget::typeEmit,
          p_shot_table_widget_, &shotTableWidget::init);
  connect(p_episodes_list_widget_,&episodesListWidget::episodesEmit,
          p_shot_table_widget_, &shotTableWidget::clear);
  connect(p_shot_list_widget_, &shotListWidget::shotEmit,
          p_shot_table_widget_, &shotTableWidget::clear);
  connect(p_file_class_shot_widget_,&fileClassShotWidget::fileClassShotEmitted,
          p_shot_table_widget_, &shotTableWidget::clear);

  //将小部件添加到布局中
  p_b_box_layout_->addWidget(p_episodes_list_widget_,1);
  p_b_box_layout_->addWidget(p_shot_list_widget_, 1);
  p_b_box_layout_->addWidget(p_file_class_shot_widget_,1);
  p_b_box_layout_->addWidget(p_file_type_shot_widget_,1);
  p_b_box_layout_->addWidget(p_shot_table_widget_,5);
}
void ProjectWidget::assInit(QWidget *parent) {
  auto test =  new QHBoxLayout(parent);
  auto but = new QLabel("科目");
  test->addWidget(but);
}
DOODLE_NAMESPACE_E