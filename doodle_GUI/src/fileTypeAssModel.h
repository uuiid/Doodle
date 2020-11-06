//
// Created by teXiao on 2020/10/15.
//
#pragma once
#include "doodle_global.h"
#include "core_global.h"

#include <QAbstractListModel>

DOODLE_NAMESPACE_S

class fileTypeAssModel : public QAbstractListModel {
  Q_OBJECT
 public:
  explicit fileTypeAssModel(QObject * parent = nullptr);
  ~fileTypeAssModel() override;

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

  void init(const doCore::assClassPtr & ass_type_ptr);
  void clear();

  doCore::assClassPtr getAssTypePtr() const;
 private:
  doCore::fileTypePtrList p_file_type_ptr_list_;
  doCore::assClassPtr p_ass_ptr_;
};
DOODLE_NAMESPACE_E
