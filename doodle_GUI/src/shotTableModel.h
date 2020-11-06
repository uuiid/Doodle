//
// Created by teXiao on 2020/10/14.
//
#pragma once

#include "doodle_global.h"

#include "core_global.h"
#include <QAbstractTableModel>


DOODLE_NAMESPACE_S
class shotTableModel : public QAbstractTableModel {
 Q_OBJECT
 public:
  explicit shotTableModel(QObject *parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] int columnCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role) const override;
  [[nodiscard]] QVariant headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const override;
  //设置数据
  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
  //修改数据
  bool insertRows(int position, int rows, const QModelIndex &parent) override;

  void init(const doCore::shotTypePtr &file_type_ptr);
  void clear();

 private:
  doCore::shotInfoPtrList p_shot_info_ptr_list_;
  doCore::shotTypePtr p_type_ptr_;
};

DOODLE_NAMESPACE_E
