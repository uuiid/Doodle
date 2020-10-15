//
// Created by teXiao on 2020/10/14.
//

#include <iostream>

#include <src/episodes.h>
#include "episodesListModel.h"

DOODLE_NAMESPACE_S

episodesListModel::episodesListModel(QObject *parent)
    : QAbstractListModel(parent), eplist() {
  init();
}
episodesListModel::~episodesListModel()
= default;
int episodesListModel::rowCount(const QModelIndex &parent) const {
  return eplist.size();
}
QVariant episodesListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (index.row() >= eplist.size())
    return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return eplist[index.row()]->getEpisdes_str();
  } else {
    return QVariant();
  }
}
doCore::episodesPtr episodesListModel::dataRaw(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;

  if (index.row() >= eplist.size())
    return nullptr;

  return eplist[index.row()];
}
Qt::ItemFlags episodesListModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  if (eplist[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}
bool episodesListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    //确认镜头不重复和没有提交
    bool isHasEps = false;
    for (auto &&x : eplist) {
      if (value.toInt() == x->getEpisdes() || x->isInsert()) {
        isHasEps = true;
        break;
      }
    }
    if (isHasEps)
      return false;
    else {
      eplist[index.row()]->setEpisdes(value.toInt());
      eplist[index.row()]->insert();
      emit dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}
bool episodesListModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    std::cout << position << " " << row << std::endl;
    eplist.insert(position, doCore::episodesPtr(new doCore::episodes));
  }
  endInsertRows();
  return true;
}
bool episodesListModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    eplist.remove(position);
  }
  endRemoveRows();
  return true;
}
void episodesListModel::init() {
  auto tmp_eps = doCore::episodes::getAll();
  beginInsertRows(QModelIndex(), 0, tmp_eps.size() - 1);
  eplist = tmp_eps;
  endInsertRows();
}
void episodesListModel::clear() {
  if (eplist.isEmpty()) return;
  beginResetModel();
  eplist.clear();
  endResetModel();

}
DOODLE_NAMESPACE_E