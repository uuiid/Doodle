//
// Created by teXiao on 2020/10/14.
//

#include "shotClassModel.h"
#include <core_doQt.h>

#include <memory>
#include "Logger.h"

#include <boost/numeric/conversion/cast.hpp>

DOODLE_NAMESPACE_S
shotClassModel::shotClassModel(QObject *parent)
    : QAbstractListModel(parent), list_fileClass() {}

int shotClassModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(list_fileClass.size());
}

QVariant shotClassModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  if (index.row() >= list_fileClass.size()) return QVariant();
  auto var = QVariant();
  switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      var = list_fileClass[index.row()]->getClass_Qstr();
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(list_fileClass[index.row()]);
      break;
    default:
      break;
  }
  return var;
}

doCore::shotClassPtr shotClassModel::dataRow(const QModelIndex &index) const {
  if (!index.isValid()) return nullptr;
  if (index.row() >= list_fileClass.size()) return nullptr;
  return list_fileClass[index.row()];
}

QVariant shotClassModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (role != Qt::DisplayRole) return QVariant();

  if (orientation == Qt::Horizontal)
    return QStringLiteral("Column %1").arg(section);
  else
    return QStringLiteral("Row %1").arg(section);
}

Qt::ItemFlags shotClassModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEnabled;

  if (list_fileClass[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

bool shotClassModel::setData(const QModelIndex &index, const QVariant &value,
                             int role) {
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
      DOODLE_LOG_INFO("注意将fileclass提交到数据库");
      list_fileClass[index.row()]->setclass(value.toString());
      dataChanged(index, index, {role});
      return true;
    }
  }
  return false;
}

bool shotClassModel::insertRows(int position, int rows,
                                const QModelIndex &index) {
  bool isHas = false;
  auto dep   = doCore::coreSet::getSet().getDepartment();
  for (auto &&i : list_fileClass) {
    if (dep == i->getClass_str()) {
      isHas = true;
      break;
    }
  }
  beginInsertRows(index, position, position + rows - 1);
  if (!isHas) {
    for (int row = 0; row < rows; ++row) {
      DOODLE_LOG_INFO("插入新的fileclass镜头");
      list_fileClass.insert(list_fileClass.begin() + position,
                            std::make_shared<doCore::shotClass>());
      list_fileClass[position]->setclass(dep);
    }
  }
  endInsertRows();
  return true;
}

bool shotClassModel::removeRows(int position, int rows,
                                const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);

  for (int row = 0; row < rows; ++row) {
    DOODLE_LOG_INFO("去除队列中的fileclass镜头");
    list_fileClass.erase(list_fileClass.begin() + position);
  }

  endRemoveRows();
  return true;
}

void shotClassModel::init() {
  auto fileClassPtrList = doCore::shotClass::getAll();
  clear();
  if (fileClassPtrList.empty()) return;
  beginInsertRows(QModelIndex(), 0,
                  boost::numeric_cast<int>(fileClassPtrList.size()) - 1);
  list_fileClass = fileClassPtrList;
  endInsertRows();
}

void shotClassModel::clear() {
  if (list_fileClass.empty()) return;
  beginResetModel();
  list_fileClass.clear();
  endResetModel();
}
void shotClassModel::reInit() {
  auto shotClass = doCore::shotClass::Instances();
  doCore::shotClassPtrList shotClassPtrList{};
  for (auto &shot : shotClass) {
    shotClassPtrList.push_back(shot->shared_from_this());
  }
  if (list_fileClass.empty() || shotClassPtrList.empty()) return;
  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(shotClassPtrList.size()) - 1);
  // list_fileClass = shotClassPtrList;
  endInsertRows();
}
DOODLE_NAMESPACE_E