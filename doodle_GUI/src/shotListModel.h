//
// Created by teXiao on 2020/10/14.
//

#pragma once

#include "doodle_global.h"

#include "core_global.h"

#include <QAbstractListModel>

DOODLE_NAMESPACE_S

/**
 * @description: 这个时镜头的自定义模型
 */
class shotListModel : public QAbstractListModel {
 Q_OBJECT
 private:
  doCore::shotPtrList shotlist;

 public:
  explicit shotListModel(QObject *parent = nullptr);
  ~shotListModel() override;

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
  [[nodiscard]] doCore::shotPtr dataRaw(const QModelIndex &index) const;

  [[nodiscard]] QVariant headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const override;

  //设置编辑标识
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
  //设置数据(内部使用 QMap[shot] 和 QMap[shotAb] 获得传入信息)
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  //插入数据
  bool insertRows(int position, int rows, const QModelIndex &index) override;
  //删除数据
  bool removeRows(int position, int rows, const QModelIndex &index) override;
 public slots:
  //自定义创建函数
  void init(const doCore::episodesPtr &episodes_);
  void clear();
 private:
  doCore::episodesPtr p_episodes;
};
DOODLE_NAMESPACE_E