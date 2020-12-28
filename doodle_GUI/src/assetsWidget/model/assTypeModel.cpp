//
// Created by teXiao on 2020/10/15.
//

#include "assTypeModel.h"

#include < core_Cpp.h>

#include <boost/numeric/conversion/cast.hpp>
#include <memory>
DOODLE_NAMESPACE_S
assTypeModel::assTypeModel(QObject *parent)
    : QAbstractListModel(parent), p_file_type_ptr_list_() {
   assType::insertChanged.connect(boost::bind(&assTypeModel::reInit, this));
}

int assTypeModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_file_type_ptr_list_.size());
}
QVariant assTypeModel::data(const QModelIndex &index, int role) const {
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
Qt::ItemFlags assTypeModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::ItemIsEditable;
  if (index.row() >= p_file_type_ptr_list_.size()) return Qt::ItemIsEditable;

  if (p_file_type_ptr_list_[index.row()]->isInsert())
    return QAbstractListModel::flags(index);
  else
    return Qt::ItemIsEditable | Qt::ItemIsEnabled |
           QAbstractListModel::flags(index);
}
bool assTypeModel::setData(const QModelIndex &index, const QVariant &value,
                           int role) {
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
    dataChanged(index, index, {role});
    return true;
  }
  return false;
}
bool assTypeModel::insertRows(int position, int rows,
                              const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_file_type_ptr_list_.insert(p_file_type_ptr_list_.begin() + position,
                                 std::make_shared< assType>());
  }
  endInsertRows();
  return true;
}
bool assTypeModel::removeRows(int position, int rows,
                              const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_file_type_ptr_list_.erase(p_file_type_ptr_list_.begin() + position);
  }
  endRemoveRows();
  return false;
}
void assTypeModel::init() {
  clear();
  const auto tmp_list =  assType::getAll();
  if (tmp_list.empty()) return;

  beginInsertRows(QModelIndex(), 0,
                  boost::numeric_cast<int>(tmp_list.size()) - 1);
  p_file_type_ptr_list_ = tmp_list;
  endInsertRows();
}
void assTypeModel::reInit() {
  // if (p_file_type_ptr_list_.empty()) return;
  auto k_instance =  assType::Instances();
   assTypePtrList list;
  for (auto &&i : k_instance) {
    if (i)
      list.push_back(i->shared_from_this());
  }
  if (list.empty())
    return;

  beginInsertRows(QModelIndex(), 0,
                  boost::numeric_cast<int>(list.size()) - 1);
  p_file_type_ptr_list_ = list;
  endInsertRows();
}
void assTypeModel::clear() {
  if (p_file_type_ptr_list_.empty()) return;

  beginResetModel();
  p_file_type_ptr_list_.clear();
  endResetModel();
}
assTypeModel::~assTypeModel() = default;

DOODLE_NAMESPACE_E