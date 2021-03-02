#include <lib/ui/model/TreeDirModel.h>

#include <boost/numeric/conversion/cast.hpp>
namespace doodle::motion::ui {
TreeDirModel::TreeDirModel(QObject *parent)
    : QAbstractItemModel(parent),
      p_root(std::make_shared<TreeDirItem>("")) {
}

QVariant TreeDirModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;

  switch (role) {
    case Qt::DisplayRole: {
      auto k_d = static_cast<TreeDirItem *>(index.internalPointer());
      var      = QString::fromStdString(k_d->Dir());
      break;
    }
    default:
      break;
  }
  return var;
}

Qt::ItemFlags TreeDirModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::NoItemFlags;
  return QAbstractItemModel::flags(index);
}

QVariant TreeDirModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const {
  return QVariant();
}

QModelIndex TreeDirModel::index(int row, int column, const QModelIndex &parent) const {
  auto k_index = QModelIndex();
  if (!this->hasIndex(row, column, parent))
    return k_index;

  TreeDirItem *k_item = nullptr;
  if (!parent.isValid())
    return k_index;
  else
    k_item = static_cast<TreeDirItem *>(parent.internalPointer());

  auto k_child = k_item->GetChild(row);
  if (k_child)
    k_index = this->createIndex(row, column, k_child.get());

  return k_index;
}

QModelIndex TreeDirModel::parent(const QModelIndex &index) const {
  auto k_index = QModelIndex();
  if (!index.isValid()) return k_index;

  auto k_item   = static_cast<TreeDirItem *>(index.internalPointer());
  auto k_parent = k_item->Parent();
  if (k_parent == p_root)
    return k_index;

  return createIndex(boost::numeric_cast<int>(k_parent->row()), 0, k_parent.get());
}

int TreeDirModel::rowCount(const QModelIndex &parent) const {
  if (parent.column() > 0) return 0;

  TreeDirItemPtr k_item{};
  if (!parent.isValid())
    k_item = p_root;
  else
    k_item = static_cast<TreeDirItem *>(parent.internalPointer())->shared_from_this();
  return boost::numeric_cast<int>(k_item->GetChildCount());
}

int TreeDirModel::columnCount(const QModelIndex &parent) const {
  return 1;
}
}  // namespace doodle::motion::ui