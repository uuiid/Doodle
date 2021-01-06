#pragma once
#include <doodle_global.h>
#include <core_global.h>

#include <QtCore/QAbstractListModel>
#include <future>
DOODLE_NAMESPACE_S

class queueListModel : public QAbstractListModel {
  Q_OBJECT
 public:
  explicit queueListModel(QObject *parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

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

 private:
  std::vector<std::pair<std::future<bool>, std::string>> p_updataQueue;
};

DOODLE_NAMESPACE_E