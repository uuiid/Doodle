#pragma once

#include <MotionGlobal.h>
#include <lib/ui/model/TreeDirItem.h>

#include <QtCore/QAbstractItemModel>

namespace doodle::motion::ui {
class TreeDirModel : public QAbstractItemModel {
  Q_OBJECT
 private:
  TreeDirItemPtr p_root;

 public:
  TreeDirModel(QObject *parent = nullptr);

  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const override;
  [[nodiscard]] QModelIndex index(int row, int column,
                                  const QModelIndex &parent = QModelIndex()) const override;
  [[nodiscard]] QModelIndex parent(const QModelIndex &index) const override;
  [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
};

}  // namespace doodle::motion::ui