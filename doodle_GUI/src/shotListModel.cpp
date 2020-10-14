//
// Created by teXiao on 2020/10/14.
//

#include "shotListModel.h"
#include "src/shot.h"

#include "Logger.h"

DOODLE_NAMESPACE_S

shotListModel::shotListModel(QObject *parent)
    : QAbstractListModel(parent),
      shotlist(),
      p_episodes(nullptr) {
}

shotListModel::~shotListModel() = default;

void shotListModel::init(const doCore::episodesPtr &episodes_) {
  doCore::shotPtrList tmp_shot_list = doCore::shot::getAll(episodes_);

  clear();

  beginInsertRows(QModelIndex(), 0, tmp_shot_list.size());
  shotlist = tmp_shot_list;
  endInsertRows();

  p_episodes = episodes_;
}

int shotListModel::rowCount(const QModelIndex &parent) const {
  return shotlist.size();
}

QVariant shotListModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (index.row() >= shotlist.size())
    return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return shotlist[index.row()]->getShotAndAb_str();
  } else {
    return QVariant();
  }
}

doCore::shotPtr shotListModel::dataRaw(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;

  return shotlist[index.row()];
}

QVariant shotListModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal)
    return QStringLiteral("Column %1").arg(section);
  else
    return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags shotListModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  if (shotlist[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool shotListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  QMap infoMap = value.toMap();
  if (index.isValid() && role == Qt::EditRole) {
    //确认镜头不重复和没有提交
    //这个函数不设置AB镜
    bool isHasShot = false;
    for (auto &&x : shotlist) {
      if ((infoMap["shot"].toInt() == x->getShot()
          && infoMap["shotAb"].toString() == x->getShotAndAb_str())
          || x->isInsert()) {
        isHasShot = true;
        break;
      }
    }

    if (isHasShot)
      return false;
    else {
      shotlist[index.row()]->setShot(infoMap["shot"].toInt(), infoMap["shotAb"].toString());
      shotlist[index.row()]->setEpisodes(p_episodes);
      shotlist[index.row()]->insert();
      emit dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}

bool shotListModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    shotlist.insert(position, doCore::shotPtr(new doCore::shot));
  }
  endInsertRows();
  return true;
}

bool shotListModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(index, position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    shotlist.remove(position);
  }
  endRemoveRows();
  return true;
}
void shotListModel::clear() {
  beginResetModel();
  shotlist.clear();
  endResetModel();
}

DOODLE_NAMESPACE_E