//
// Created by teXiao on 2020/10/14.
//
#pragma once
#include <QAbstractListModel>

#include "doodle_global.h"
#include "core_global.h"

DOODLE_NAMESPACE_S
/**
 * @description: 自定义集数模型
 */
class shotEpsListModel : public QAbstractListModel {
  Q_OBJECT
 private:
  doCore::episodesPtrList eplist;

 public:
  explicit shotEpsListModel(QObject *parent = nullptr);
  ~shotEpsListModel() override;

  //返回总行数
  int rowCount(const QModelIndex &parent) const override;
  //返回数据
  QVariant data(const QModelIndex &index, int role) const override;

  //设置是否编辑标识
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  //设置数据
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  //添加数据
  bool insertRows(int position, int rows, const QModelIndex &index) override;
  bool removeRows(int position, int rows, const QModelIndex &index) override;

 public Q_SLOTS:
  //刷新函数
  void init();
  void clear();
};
DOODLE_NAMESPACE_E
