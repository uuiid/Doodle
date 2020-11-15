//
// Created by teXiao on 2020/10/14.
//
#include "fileTypeShotModel.h"

#include <memory>

#include "src/coreset.h"
#include "src/shottype.h"

DOODLE_NAMESPACE_S
fileTypeShotModel::fileTypeShotModel(QObject *parent) : QAbstractListModel(parent) {}

int fileTypeShotModel::rowCount(const QModelIndex &parent) const {
  return p_type_ptr_list_.size();
}

QVariant fileTypeShotModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  if (index.row() >= p_type_ptr_list_.size()) return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return p_type_ptr_list_[index.row()]->getTypeQ();
  } else {
    return QVariant();
  }
}

doCore::shotTypePtr fileTypeShotModel::daraRow(const QModelIndex &index) const {
  if (!index.isValid()) return nullptr;
  return p_type_ptr_list_[index.row()];
}
QVariant fileTypeShotModel::headerData(int section, Qt::Orientation Orientation, int role) const {
  return QAbstractItemModel::headerData(section, Orientation, role);
}
Qt::ItemFlags fileTypeShotModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEnabled;

  if (p_type_ptr_list_[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEnabled | Qt::ItemIsEditable | QAbstractListModel::flags(index);
}

void fileTypeShotModel::init(const doCore::shotClassPtr &file_class_ptr) {
  p_class_ptr_ = file_class_ptr;
  auto tmp_fileTypeList = doCore::shotType::getAll(file_class_ptr);
  clear();
  beginInsertRows(QModelIndex(), 0, tmp_fileTypeList.size());
  p_type_ptr_list_ = tmp_fileTypeList;
  endInsertRows();
}
void fileTypeShotModel::clear() {
  p_class_ptr_ = nullptr;
  beginResetModel();
  p_type_ptr_list_.clear();
  endResetModel();
}
bool fileTypeShotModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    bool isHas = false;
    for (const auto &item : p_type_ptr_list_) {
      if (item->isInsert() || value.toString() == item->getTypeQ()) {
        isHas = true;
        break;
      }
    }

    if (!isHas) {
      p_type_ptr_list_[index.row()]->setType(value.toString());
      p_type_ptr_list_[index.row()]->setShotClass(p_class_ptr_);
      p_type_ptr_list_[index.row()]->insert();
    } else {
      return false;
    }
  }
  return true;
}
bool fileTypeShotModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_type_ptr_list_.insert(p_type_ptr_list_.begin() + position,
                            std::make_shared<doCore::shotType>());
  }
  endInsertRows();
  return true;
}
bool fileTypeShotModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_type_ptr_list_.erase(p_type_ptr_list_.begin() + position);
  }
  endRemoveRows();
  return true;
}

fileTypeShotModel::~fileTypeShotModel() = default;;

DOODLE_NAMESPACE_E