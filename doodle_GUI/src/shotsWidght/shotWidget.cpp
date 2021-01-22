#include "shotWidget.h"

#include <core_Cpp.h>

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
#include <src/shotsWidght/model/shotTableFilterModel.h>
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
  //设置基本布局
  p_shot_layout_ = new QHBoxLayout(this);
  p_shot_layout_->setSpacing(3);
  p_shot_layout_->setContentsMargins(0, 0, 0, 0);
  p_shot_layout_->setObjectName(QString::fromUtf8("p_shot_layout_"));
  //创建模型
  p_episodes_list_model_ = new shotEpsListModel();
  p_episodes_list_model_->setObjectName(QString::fromUtf8("p_episodes_list_model_"));
  p_shot_list_model_  = new shotListModel();
  p_shot_type_model_  = new shotTypeModel();
  p_shot_class_model_ = new shotClassModel();
  p_shot_table_model_ = new shotTableModel();
  //创建代理模型
  auto k_shot_filter_model_ = new shotTableFilterModel();
  k_shot_filter_model_->setSourceModel(p_shot_table_model_);

  //创建集数小部件
  p_episodes_list_widget_ = new shotEpsListWidget();
  p_episodes_list_widget_->setObjectName(
      QString::fromUtf8("p_episodes_list_widget_"));
  p_episodes_list_widget_->setModel(p_episodes_list_model_);

  //添加镜头小部件
  p_shot_list_widget_ = new shotListWidget();
  p_shot_list_widget_->setObjectName(QString::fromUtf8("p_shot_list_widget_"));
  p_shot_list_widget_->setModel(p_shot_list_model_);

  //添加部门小部件和种类小部件的布局
  auto layout_1 = new QVBoxLayout();
  layout_1->setSpacing(3);
  layout_1->setContentsMargins(0, 0, 0, 0);
  layout_1->setObjectName(QString::fromUtf8("layout_1"));
  //添加shotTable
  p_shot_table_widget_ = new shotTableWidget();
  p_shot_table_widget_->setObjectName("p_shot_table_widget_");
  p_shot_table_widget_->setModel(k_shot_filter_model_);

  //添加部门小部件
  p_shot_class_widget_ = new shotClassWidget();
  p_shot_class_widget_->setObjectName(QString::fromUtf8("p_shot_class_widget_"));
  p_shot_class_widget_->setModel(p_shot_class_model_);
  layout_1->addWidget(p_shot_class_widget_);

  //添加种类小部件
  p_shot_type_widget_ = new shotTypeWidget();
  p_shot_type_widget_->setObjectName("p_shot_type_widget_");
  p_shot_type_widget_->setModel(p_shot_type_model_);
  layout_1->addWidget(p_shot_type_widget_);

  // 连接两个过滤器
  p_shot_class_widget_->doodleUseFilter
      .connect([=](const doodle::shotClassPtr& shot_class, const doodle::filterState& fiter) {
        coreDataManager::get().setShotClassPtr(shot_class);
        k_shot_filter_model_->useFilter(fiter);
      });
  p_shot_type_widget_->doodleUseFilter
      .connect([=](const doodle::shotTypePtr& shot_type, const doodle::filterState& fiter) {
        coreDataManager::get().setShotTypePtr(shot_type);
        k_shot_filter_model_->useFilter(fiter);
      });

  //连接镜头点击
  p_episodes_list_widget_->chickItem.connect(
      [=](const episodesPtr& eps) {
        auto& mData = coreDataManager::get();
        mData.setEpisodesPtr(eps);
        mData.setShotPtr(nullptr);
        mData.setShotClassPtr(nullptr);
        mData.setShotTypePtr(nullptr);
        mData.setShotInfoPtr(nullptr);

        p_shot_type_widget_->clear();
        p_shot_class_widget_->clear();

        k_shot_filter_model_->useFilter(filterState::notFilter);
        p_shot_list_model_->setList(shot::getAll(eps));
        p_shot_table_model_->setList(shotFileSqlInfo::getAll(eps));
      });
  p_shot_list_widget_->chickItem.connect(
      [=](const shotPtr& sh_) {
        auto& mData = coreDataManager::get();
        mData.setShotPtr(sh_);
        mData.setShotClassPtr(nullptr);
        mData.setShotTypePtr(nullptr);
        mData.setShotInfoPtr(nullptr);

        p_shot_type_widget_->clear();
        p_shot_class_widget_->clear();

        k_shot_filter_model_->useFilter(filterState::notFilter);

        p_shot_table_model_->setList(shotFileSqlInfo::getAll(sh_));
      });

  //将小部件添加到布局中
  p_shot_layout_->addWidget(p_episodes_list_widget_, 2);
  p_shot_layout_->addWidget(p_shot_list_widget_, 2);
  p_shot_layout_->addWidget(p_shot_table_widget_, 10);
  //添加布局
  p_shot_layout_->addLayout(layout_1, 3);
  setMinimumWidth(500);
}
void shotWidget::refresh() {
  auto& mData = coreDataManager::get();
  mData.setEpisodesPtr(nullptr);
  mData.setShotPtr(nullptr);
  mData.setShotClassPtr(nullptr);
  mData.setShotTypePtr(nullptr);
  mData.setShotInfoPtr(nullptr);

  p_episodes_list_model_->setList(episodes::getAll());
  p_shot_class_model_->setList(shotClass::getAll());
  p_shot_type_model_->setList(shotType::getAll());

  p_shot_list_model_->clear();
  p_shot_table_model_->clear();

  try {
    auto k_shotClass = shotClass::getCurrentClass();
  } catch (const std::runtime_error& e) {
    auto k_shotClass = std::make_shared<shotClass>();
    k_shotClass->setclass(coreSet::getSet().getDepartmentQ());
    k_shotClass->insert();
    std::cerr << e.what() << '\n';
  }
}

DOODLE_NAMESPACE_E