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

  void init();

 private:
  void clear();
  void DoodleProgressChanged(const queueDataPtr &data);

  std::vector<queueDataPtr> p_updataQueue;
};

DOODLE_NAMESPACE_E