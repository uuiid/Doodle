#pragma once

#include <MotionGlobal.h>
#include <lib/ui/model/TreeDirItem.h>

#include <QtCore/QAbstractItemModel>

namespace doodle::motion::ui {
class TreeDirModel : public QAbstractItemModel {
  Q_OBJECT
 private:
  TreeDirItemPtr p_root;

  TreeDirItemPtr getItem(const QModelIndex &index) const;

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

  // bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
  //创建可编辑模型
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  // bool setHeaderData(int section, Qt::Orientation orientation,
  //  const QVariant &value, int role = Qt::EditRole) override;

  bool insertColumns(int position, int columns,
                     const QModelIndex &parent = QModelIndex()) override;
  bool removeColumns(int position, int columns,
                     const QModelIndex &parent = QModelIndex()) override;
  bool insertRows(int position, int rows,
                  const QModelIndex &parent = QModelIndex()) override;
  bool removeRows(int position, int rows,
                  const QModelIndex &parent = QModelIndex()) override;

  void refreshChild(const QModelIndex & index);
};

}  // namespace doodle::motion::ui