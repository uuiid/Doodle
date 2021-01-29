//
// Created by teXiao on 2020/10/14.
//

#pragma once

#include "doodle_global.h"
#include "core_global.h"

#include <QAbstractListModel>

DOODLE_NAMESPACE_S
class assDepModel : public QAbstractListModel {
  Q_OBJECT
 public:
  explicit assDepModel(QObject *parent = nullptr);
  ~assDepModel() override;

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;


  void clear();
  void setList(const assDepPtrList &setList);

 private:
  assDepPtrList p_class_ptr_list_;
};

DOODLE_NAMESPACE_E