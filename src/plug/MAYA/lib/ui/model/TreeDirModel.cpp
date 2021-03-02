#include <maya/MGlobal.h>

#include <lib/ui/model/TreeDirModel.h>

#include <boost/numeric/conversion/cast.hpp>
namespace doodle::motion::ui {
TreeDirItemPtr TreeDirModel::getItem(const QModelIndex &index) const {
  if (index.isValid()) {
    auto k_item = static_cast<TreeDirItem *>(index.internalPointer())->shared_from_this();
    return k_item;
  }
  return p_root;
}

TreeDirModel::TreeDirModel(QObject *parent)
    : QAbstractItemModel(parent),
      p_root(std::make_shared<TreeDirItem>("")) {
  // auto tmp = std::make_shared<TreeDirItem>("test");
  // tmp->setParent(p_root);
  // tmp = std::make_shared<TreeDirItem>("test");
  // tmp->setParent(p_root);
  // tmp = std::make_shared<TreeDirItem>("test");
  // tmp->setParent(p_root);
  // tmp = std::make_shared<TreeDirItem>("test");
  // tmp->setParent(p_root);
  // tmp = std::make_shared<TreeDirItem>("test");
  // tmp->setParent(p_root);

  // tmp = std::make_shared<TreeDirItem>("test");
  // tmp->setParent(p_root->GetChild(1));
  // tmp = std::make_shared<TreeDirItem>("test");
  // tmp->setParent(p_root->GetChild(1));

  // tmp = std::make_shared<TreeDirItem>("test2");
  // tmp->setParent(p_root->GetChild(2));
  // tmp = std::make_shared<TreeDirItem>("test3");
  // tmp->setParent(p_root->GetChild(2));
  // tmp = std::make_shared<TreeDirItem>("test4");
  // tmp->setParent(p_root->GetChild(2));
}

QVariant TreeDirModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;

  auto k_d = getItem(index);

  switch (role) {
    case Qt::DisplayRole: {
      var = QString::fromStdString(k_d->Dir());
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
  return tr("分类");
}

QModelIndex TreeDirModel::index(int row, int column, const QModelIndex &parent) const {
  auto k_index = QModelIndex();
  // if (!parent.isValid())
  //   return k_index;
  // if (parent.column() != 0)
  //   return k_index;
  // MGlobal::displayInfo(MString{"\ncolumn: "} + parent.column() +
  //                      "\nrow: " + parent.row() +
  //                      "\nvalid: " + parent.isValid());
  if (!this->hasIndex(row, column, parent)) {
    MGlobal::displayInfo(MString{"has index"});
    return k_index;
  }

  auto k_d = getItem(parent);

  auto k_child = k_d->GetChild(row);
  if (k_child)
    k_index = this->createIndex(row, column, k_child.get());

  return k_index;
}

QModelIndex TreeDirModel::parent(const QModelIndex &index) const {
  auto k_index = QModelIndex();
  if (!index.isValid()) return k_index;

  auto k_item   = getItem(index);
  auto k_parent = k_item->Parent();

  if (k_parent == p_root || !k_parent)  //
    return k_index;

  return createIndex(boost::numeric_cast<int>(k_parent->row()), 0, k_parent.get());
}

int TreeDirModel::rowCount(const QModelIndex &parent) const {
  if (parent.column() > 0)
    return 0;

  auto k_item = getItem(parent);
  return k_item ? boost::numeric_cast<int>(k_item->GetChildCount()) : 0;
}

int TreeDirModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 1;
  else
    return 1;
}
}  // namespace doodle::motion::ui