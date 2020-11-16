//
// Created by teXiao on 2020/10/15.
//

#include "AssTypeModel.h"
#include <core_doQt.h>

#include <memory>

DOODLE_NAMESPACE_S
AssTypeModel::AssTypeModel(QObject *parent)
    : QAbstractListModel(parent),
      p_file_type_ptr_list_(),
      p_ass_ptr_(nullptr) {}
int AssTypeModel::rowCount(const QModelIndex &parent) const {
  return p_file_type_ptr_list_.size();
}
QVariant AssTypeModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;
  if (index.row() >= p_file_type_ptr_list_.size()) return var;

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    var = p_file_type_ptr_list_[index.row()]->getTypeQ();
  } else if (role == Qt::UserRole) {
    var = QVariant::fromValue(p_file_type_ptr_list_[index.row()]);
  }

  return var;
}
Qt::ItemFlags AssTypeModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEditable;
  if (index.row() >= p_file_type_ptr_list_.size()) return Qt::ItemIsEditable;

  if (p_file_type_ptr_list_[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}
bool AssTypeModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  bool is_has = false;
  for (auto &&item : p_file_type_ptr_list_) {
    if (value.toString() == item->getTypeQ()) {
      is_has = true;
      break;
    }
  }

  if (!is_has) {
    p_file_type_ptr_list_[index.row()]->setType(value.toString());
    p_file_type_ptr_list_[index.row()]->setAssClassPtr(p_ass_ptr_);
    p_file_type_ptr_list_[index.row()]->insert();
    emit dataChanged(index, index, {role});
    return true;
  }
  return false;

}
bool AssTypeModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_file_type_ptr_list_.insert(p_file_type_ptr_list_.begin() + position,
                                 std::make_shared<doCore::assType>());
  }
  endInsertRows();
  return true;
}
bool AssTypeModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(QModelIndex(),position, position + rows -1);
  for (int row = 0; row < rows; ++row) {
    p_file_type_ptr_list_.erase(p_file_type_ptr_list_.begin() + position);
  }
  endRemoveRows();
  return false;
}
void AssTypeModel::init(const doCore::assClassPtr &ass_type_ptr) {
  clear();
  p_ass_ptr_ = ass_type_ptr;
  auto tmp_list = doCore::assType::getAll();
  beginInsertRows(QModelIndex(),0,tmp_list.size() -1);
  p_file_type_ptr_list_ = tmp_list;
  endInsertRows();
}
void AssTypeModel::clear() {
  if(!p_file_type_ptr_list_.empty()) return;
  p_ass_ptr_ = nullptr;
  beginResetModel();
  p_file_type_ptr_list_.clear();
  endResetModel();
}
doCore::assClassPtr AssTypeModel::getAssTypePtr() const  {
  return p_ass_ptr_;
}
AssTypeModel::~AssTypeModel() = default;

DOODLE_NAMESPACE_E