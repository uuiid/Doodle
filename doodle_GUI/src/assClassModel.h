//
// Created by teXiao on 2020/10/15.
//

#pragma once
#include "doodle_global.h"
#include "core_global.h"

#include <QAbstractListModel>
DOODLE_NAMESPACE_S
class assClassModel : public QAbstractListModel {
 Q_OBJECT
 public:
  explicit assClassModel(QObject *parent = nullptr);
  ~assClassModel() override;

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
  //设置编辑标识
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
  //设置数据
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  //插入数据
  bool insertRows(int position, int rows, const QModelIndex &index) override;
  //删除数据
  bool removeRows(int position, int rows, const QModelIndex &index) override;

  void init(const doCore::shotClassPtr & file_class_ptr);
  void clear();

 private:
  doCore::assClassPtrList p_ass_info_ptr_list_;
  doCore::shotClassPtr p_class_ptr_;

};
DOODLE_NAMESPACE_E