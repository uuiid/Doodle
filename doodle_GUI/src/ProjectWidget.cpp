//
// Created by teXiao on 2020/10/14.
//

#include "src/episodes.h"
#include "src/shot.h"
#include "src/shotClass.h"
#include "src/shottype.h"

#include "src/assClass.h"

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

#include "fileClassAssModel.h"
#include "assClassModel.h"
#include "fileTypeAssModel.h"
#include "assTableModel.h"

#include "fileClassAssWidget.h"
#include "assClassWidget.h"
#include "fileTypeAssWidget.h"
#include "assTableWidght.h"


#include "ProjectWidget.h"

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

  p_file_class_ass_widget_(nullptr),
  p_ass_class_widget_(nullptr),
  p_file_type_ass_widget_(nullptr),
  p_ass_table_widght_(nullptr),
  p_file_class_ass_model_(nullptr),
  p_ass_class_model_(nullptr),
  p_file_type_ass_model_(nullptr),
  p_ass_table_model_(nullptr){
  init_();
}
void ProjectWidget::init_() {
  auto ass_parent = new QWidget();
  assInit(ass_parent);
  addTab(ass_parent,QString("ass"));
  setTabText(0,tr("资产"));

  //添加shot页面
  auto shot_anm_parent = new QWidget();
  shotInitAnm(shot_anm_parent);
  addTab(shot_anm_parent,QString("Anm"));
  setTabText(1,tr("动画"));
}

void ProjectWidget::shotInitAnm(QWidget *parent) {
  //创建模型
  p_episodes_list_model_ = new episodesListModel(this);
  p_shot_list_model_ = new shotListModel(this);
  p_file_type_shot_model_ = new fileTypeShotModel(this);
  p_file_class_shot_model_ = new fileClassShotModel(this);
  p_shot_table_model_ = new shotTableModel(this);

  //设置基本布局
  p_shot_layout_ = new QHBoxLayout(parent);
  p_shot_layout_->setSpacing(3);
  p_shot_layout_->setContentsMargins(0, 0, 0, 0);
  p_shot_layout_->setObjectName(QString::fromUtf8("p_shot_layout_"));

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
  p_shot_layout_->addWidget(p_episodes_list_widget_, 1);
  p_shot_layout_->addWidget(p_shot_list_widget_, 1);
  p_shot_layout_->addWidget(p_file_class_shot_widget_, 1);
  p_shot_layout_->addWidget(p_file_type_shot_widget_, 1);
  p_shot_layout_->addWidget(p_shot_table_widget_, 5);
}
void ProjectWidget::assInit(QWidget *parent) {
  p_ass_layout_ = new QHBoxLayout(parent);
  p_ass_layout_->setSpacing(3);
  p_ass_layout_->setContentsMargins(0, 0, 0, 0);
  p_ass_layout_->setObjectName(QString::fromUtf8("p_ass_layout_"));

  p_file_class_ass_model_ = new fileClassAssModel(this);
  p_ass_class_model_ = new assClassModel(this);
  p_file_type_ass_model_ = new fileTypeAssModel(this);
  p_ass_table_model_ = new assTableModel(this);

  p_file_class_ass_widget_ = new fileClassAssWidget(parent);
  p_file_class_ass_widget_->setObjectName("p_file_class_ass_widget_");
  p_file_class_ass_widget_->setModel(p_file_class_ass_model_);


  p_ass_class_widget_ = new assClassWidget(parent);
  p_ass_class_widget_->setObjectName("p_ass_class_widget_");
  p_ass_class_widget_->setModel(p_ass_class_model_);
  connect(p_file_class_ass_widget_, &fileClassAssWidget::fileClassEmit,
          p_ass_class_widget_, &assClassWidget::init);


  p_file_type_ass_widget_ = new fileTypeAssWidget(parent);
  p_file_type_ass_widget_->setObjectName("p_file_type_ass_widget_");
  p_file_type_ass_widget_->setModel(p_file_type_ass_model_);
  connect(p_ass_class_widget_, &assClassWidget::assClassEmited,
          p_file_type_ass_widget_,&fileTypeAssWidget::init);
  connect(p_file_class_ass_widget_, & fileClassAssWidget::fileClassEmit,
          p_file_type_ass_widget_,&fileTypeAssWidget::clear);

  p_ass_table_widght_ = new assTableWidght(parent);
  p_ass_table_widght_->setObjectName("p_ass_table_widght_");
  p_ass_table_widght_->setModel(p_ass_table_model_);
  connect(p_file_type_ass_widget_,&fileTypeAssWidget::filetypeEmited,
          p_ass_table_widght_, &assTableWidght::init);
  connect(p_ass_class_widget_, &assClassWidget::assClassEmited,
          p_ass_table_widght_, &assTableWidght::clear);
  connect(p_file_class_ass_widget_, &fileClassAssWidget::fileClassEmit,
          p_ass_table_widght_, &assTableWidght::clear);

  auto class_ass_layout = new QVBoxLayout();
  class_ass_layout->addWidget(p_file_class_ass_widget_,1);
  class_ass_layout->addWidget(p_ass_class_widget_,50);
  p_ass_layout_->addLayout(class_ass_layout,2);

  p_ass_layout_->addWidget(p_file_type_ass_widget_,1);
  p_ass_layout_->addWidget(p_ass_table_widght_,3);
}
void ProjectWidget::refresh() {
  p_file_class_ass_widget_->init();
  p_ass_class_widget_->clear();
  p_file_type_ass_widget_->clear();
  p_ass_table_widght_->clear();

  p_episodes_list_widget_->init();
  p_shot_list_widget_->clear();
  p_file_class_shot_widget_->clear();
  p_file_type_shot_widget_->clear();
  p_shot_table_widget_->clear();
}
DOODLE_NAMESPACE_E