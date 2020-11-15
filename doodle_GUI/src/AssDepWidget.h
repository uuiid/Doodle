//
// Created by teXiao on 2020/10/15.
//

#pragma once

#include "doodle_global.h"
#include "core_global.h"
#include <QListView>
DOODLE_NAMESPACE_S
class AssDepWidget : public QListView {
 Q_OBJECT
 public:
  explicit AssDepWidget(QWidget *parent = nullptr);
  ~AssDepWidget() override;

  void setModel(QAbstractItemModel *model) override;
 public slots:
  void init();

 signals:
  void fileClassEmit(const doCore::shotClassPtr &file_class_ptr);

 private:
  AssDepModel * p_file_class_ass_model_;

 private slots:
  void _doodle_emit(const  QModelIndex & index);

};
DOODLE_NAMESPACE_E
