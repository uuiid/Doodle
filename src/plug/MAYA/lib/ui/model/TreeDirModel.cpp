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
      p_root(std::make_shared<TreeDirItem>("etc")) {
  p_root->recursiveRefreshChild();

  // auto tmp = p_root->MakeChild(0, "test");
  // p_root->MakeChild(0, "test");
  // p_root->MakeChild(0, "test");
  // p_root->MakeChild(0, "test");
  // p_root->MakeChild(0, "test");

  // tmp->MakeChild(0, "test2");
  // tmp->MakeChild(0, "test2");
  // tmp->MakeChild(0, "test2");
  // auto tmp2 = tmp->MakeChild(0, "test2");

  // tmp2->MakeChild(0, "test3");
  // tmp2->MakeChild(0, "test3");
  // tmp2->MakeChild(0, "test3");
  // tmp2->MakeChild(0, "test3");
  // tmp2->MakeChild(0, "test3");
}

QVariant TreeDirModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;

  auto k_d = getItem(index);

  switch (role) {
    case Qt::DisplayRole: {
      var = QString::fromStdString(std::get<std::string>(k_d->Data(index.column())));
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
  return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsEditable;
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
    MGlobal::displayInfo(MString{"not has index"});
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

  return createIndex(boost::numeric_cast<int>(k_parent->ChildNumber()), 0, k_parent.get());
}

int TreeDirModel::rowCount(const QModelIndex &parent) const {
  if (parent.column() > 0)
    return 0;

  auto k_item = getItem(parent);
  return k_item ? boost::numeric_cast<int>(k_item->GetChildCount()) : 0;
}

int TreeDirModel::columnCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_root->columnCount());
}

// bool TreeDirModel::hasChildren(const QModelIndex &parent) const {
//   auto k_ = getItem(parent);
//   k_->refreshChild();
//   // return k_->GetChildCount() > 0;
//   return true;
// }

bool TreeDirModel::setData(const QModelIndex &index, const QVariant &value,
                           int role) {
  auto k_item = getItem(index);

  auto k_r = true;
  switch (role) {
    case Qt::EditRole: {
      if (value.toString().isEmpty()) {
        k_r = false;
        break;
      }
      k_item->setData(index.column(), value.toString().toStdString());
      dataChanged(index, index, {Qt::EditRole});
      break;
    }
    default:
      k_r = false;
      break;
  }
  return k_r;
}

bool TreeDirModel::insertColumns(int position, int columns, const QModelIndex &parent) {
  return false;
}
bool TreeDirModel::removeColumns(int position, int columns, const QModelIndex &parent) {
  return false;
}
bool TreeDirModel::insertRows(int position, int rows, const QModelIndex &parent) {
  auto k_parent  = getItem(parent);
  auto k_request = true;
  beginInsertRows(parent, position, position + rows - 1);
  for (auto i = 0; i < rows; ++i) {
    auto k_ptr = k_parent->MakeChild(position + i, "");
    k_request &= (bool)k_ptr;
  }
  endInsertRows();

  return k_request;
}
bool TreeDirModel::removeRows(int position, int rows, const QModelIndex &parent) {
  auto k_parent  = getItem(parent);
  auto k_request = true;
  beginRemoveRows(parent, position, position + rows - 1);
  for (auto i = 0; i < rows; ++i) {
    auto k_data = k_parent->GetChild(position + i);
    if (k_data) {
      auto k_rus = k_parent->removeChild(k_data);
      k_request &= k_rus;
    } else
      k_request &= false;
  }
  endRemoveRows();
  return k_request;
}
void TreeDirModel::refreshChild(const QModelIndex &index) {
  auto k_data = getItem(index);

  auto k_sigCon_bi = k_data->benignInsertRows.connect([=](const int start, const int size) {
    this->beginInsertRows(index, start, size);
  });
  auto k_sigCon_ei = k_data->endInsertRows.connect([=]() {
    this->endInsertRows();
  });
  auto k_sigCon_br = k_data->benignRemoveRows.connect([=](const int start, const int size) {
    this->beginRemoveRows(index, start, size);
  });
  auto k_sigCon_er = k_data->endRemoveRows.connect([=]() {
    this->endRemoveRows();
  });

  k_data->refreshChild();

  k_sigCon_bi.connected();
  k_sigCon_br.connected();
  k_sigCon_ei.connected();
  k_sigCon_er.connected();
}
}  // namespace doodle::motion::ui