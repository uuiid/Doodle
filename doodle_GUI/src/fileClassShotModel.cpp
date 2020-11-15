//
// Created by teXiao on 2020/10/14.
//

#include "fileClassShotModel.h"
#include <core_doQt.h>

#include <memory>
#include "Logger.h"

DOODLE_NAMESPACE_S
fileClassShotModel::fileClassShotModel(QObject *parent)
    : QAbstractListModel(parent),
      list_fileClass(),
      p_shot(nullptr) {
}

int fileClassShotModel::rowCount(const QModelIndex &parent) const {
  return list_fileClass.size();
}

QVariant fileClassShotModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= list_fileClass.size())
    return QVariant();
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return list_fileClass[index.row()]->getClass_Qstr();
  } else {
    return QVariant();
  }
}

doCore::shotClassPtr fileClassShotModel::dataRow(const QModelIndex &index) const {
  if (!index.isValid())
    return nullptr;
  if (index.row() >= list_fileClass.size())
    return nullptr;
  return list_fileClass[index.row()];
}

QVariant fileClassShotModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal)
    return QStringLiteral("Column %1").arg(section);
  else
    return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags fileClassShotModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  if (list_fileClass[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool fileClassShotModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    //确认没有重复的fileclass
    bool isHas = false;
    for (auto &&i : list_fileClass) {
      if (value.toString() == i->getClass_Qstr() || i->isInsert()) {
        isHas = true;
        break;
      }
    }

    if (isHas)
      return false;
    else {
      DOODLE_LOG_CRIT << "注意将fileclass提交到数据库";
      list_fileClass[index.row()]->setclass(value.toString());
      list_fileClass[index.row()]->setShot(p_shot);
      list_fileClass[index.row()]->insert();

      emit dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}

bool fileClassShotModel::insertRows(int position, int rows, const QModelIndex &index) {
  bool isHas = false;
  auto dep = doCore::coreSet::getSet().getDepartment();
  for (auto &&i : list_fileClass) {
    if (dep == i->getClass_str()) {
      isHas = true;
      break;
    }
  };
  beginInsertRows(index, position, position + rows - 1);
  if (!isHas) {
    for (int row = 0; row < rows; ++row) {
      DOODLE_LOG_INFO << "插入新的fileclass镜头";
      list_fileClass.insert(list_fileClass.begin() +  position,
                            std::make_shared<doCore::shotClass>());
      list_fileClass[position]->setclass(dep);
      list_fileClass[position]->setShot(p_shot);
      list_fileClass[position]->insert();
    }
  }
  endInsertRows();
  return true;
}

bool fileClassShotModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    DOODLE_LOG_INFO << "去除队列中的fileclass镜头";
    list_fileClass.erase(list_fileClass.begin() + position);
  }

  endRemoveRows();
  return true;
}

void fileClassShotModel::init(const doCore::shotPtr &shot) {
  p_shot = shot;

  doCore::shotClassPtrList fileClassPtrList = doCore::shotClass::getAll(shot);
  clear();
  beginInsertRows(QModelIndex(), 0, fileClassPtrList.size());
  list_fileClass = fileClassPtrList;
  endInsertRows();
}

void fileClassShotModel::clear() {
  p_shot = nullptr;
  if (list_fileClass.empty()) return;
  beginResetModel();
  list_fileClass.clear();
  endResetModel();
}
DOODLE_NAMESPACE_E