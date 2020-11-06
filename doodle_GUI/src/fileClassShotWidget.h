/*
 * @Author: your name
 * @Date: 2020-10-10 10:25:56
 * @LastEditTime: 2020-10-10 14:36:12
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\src\fileClassShotWidget.h
 */
#pragma once

#include "doodle_global.h"
#include "core_global.h"

#include <QListView>
#include <QAbstractListModel>
#include <QStyledItemDelegate>

DOODLE_NAMESPACE_S

class fileClassShotWidget : public QListView {
 Q_OBJECT

 public:
  explicit fileClassShotWidget(QWidget *parent = nullptr);
  ~fileClassShotWidget() override = default;

  void setModel(QAbstractItemModel *model) override;
 signals:
  void fileClassShotEmitted(const doCore::shotClassPtr &fc_);

 public slots:
  void init(const doCore::shotPtr &shot);
  void clear();

 private slots:
  //添加fileclass
  void insertFileClass();
  //私有化fileclass发射
  void _doodle_fileclass_emit(const QModelIndex &index);

 protected:
  void contextMenuEvent(QContextMenuEvent *event) override;

 private:
  fileClassShotModel *p_model_;
  //上下文菜单
  QMenu *p_fileClass_menu;

  //保存上一个小部件发射出来的集数指针
  doCore::shotPtr p_shot;
};

DOODLE_NAMESPACE_E