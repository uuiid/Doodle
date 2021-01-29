//
// Created by teXiao on 2020/10/15.
//

#pragma once

#include "doodle_global.h"
#include "core_global.h"
#include <QListView>
DOODLE_NAMESPACE_S
class assDepWidget : public QListView {
  Q_OBJECT
 public:
  explicit assDepWidget(QWidget *parent = nullptr);
  ~assDepWidget() override;

  void setModel(QAbstractItemModel *model) override;
  boost::signals2::signal<void(const assDepPtr &)> chickItem;

 private:
  assDepModel *p_file_class_ass_model_;

 private Q_SLOTS:
  void _doodle_chicked_emit(const QModelIndex &index);
};
DOODLE_NAMESPACE_E
