//
// Created by teXiao on 2020/10/14.
//

#include <iostream>
#include <memory>

#include < core_Cpp.h>
#include "shotEpsListModel.h"

#include <boost/numeric/conversion/cast.hpp>
DOODLE_NAMESPACE_S

shotEpsListModel::shotEpsListModel(QObject *parent)
    : QAbstractListModel(parent), p_episodes_ptr_list() {
}
shotEpsListModel::~shotEpsListModel() = default;
int shotEpsListModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_episodes_ptr_list.size());
}
QVariant shotEpsListModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;

  if (index.row() >= p_episodes_ptr_list.size()) return var;

  switch (role) {
    case Qt::DisplayRole:
      var = p_episodes_ptr_list[index.row()]->getEpisdes_QStr();
      break;
    case Qt::EditRole:
      var = p_episodes_ptr_list[index.row()]->getEpisdes();
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(p_episodes_ptr_list[index.row()].get());
      break;
    default:
      break;
  }

  return var;
}
Qt::ItemFlags shotEpsListModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEnabled;

  // if (p_episodes_ptr_list[index.row()]->isInsert())
  //   return QAbstractListModel::flags(index);
  // else
  return Qt::ItemIsEditable | Qt::ItemIsEnabled |
         QAbstractListModel::flags(index);
}
bool shotEpsListModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) {
  if (index.isValid() && role == Qt::EditRole) {
    //确认镜头不重复和没有提交
    bool isHasEps = false;
    for (auto &&x : p_episodes_ptr_list) {
      if (value.toInt() == x->getEpisdes() && x->isInsert()) {
        isHasEps = true;
        break;
      }
    }
    if (isHasEps)
      return false;
    else {
      p_episodes_ptr_list[index.row()]->setEpisdes(value.toInt());
      p_episodes_ptr_list[index.row()]->updateSQL();
      dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}
bool shotEpsListModel::insertRows(int position, int rows,
                                  const QModelIndex &index) {
  beginInsertRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    auto eps = std::make_shared<episodes>();
    eps->insert();
    p_episodes_ptr_list.insert(p_episodes_ptr_list.begin() + position,
                               eps);
  }
  endInsertRows();
  return true;
}
bool shotEpsListModel::removeRows(int position, int rows,
                                  const QModelIndex &index) {
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    auto eps = p_episodes_ptr_list[position];
    eps->deleteSQL();
    p_episodes_ptr_list.erase(p_episodes_ptr_list.begin() + position);
  }
  endRemoveRows();
  return true;
}

void shotEpsListModel::clear() {
  if (p_episodes_ptr_list.empty()) return;
  beginResetModel();
  p_episodes_ptr_list.clear();
  endResetModel();
}

void shotEpsListModel::setList(const episodesPtrList &list) {
  clear();
  if (list.empty()) return;
  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(list.size()) - 1);
  p_episodes_ptr_list = list;
  endInsertRows();
}
DOODLE_NAMESPACE_E