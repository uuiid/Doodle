//
// Created by teXiao on 2020/10/15.
//

#include "assTableModel.h"
#include <core_doQt.h>

#include "Logger.h"
#include <QJsonArray>
#include <memory>
DOODLE_NAMESPACE_S
assTableModel::assTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      p_ass_info_ptr_list_()
      {}
int assTableModel::rowCount(const QModelIndex &parent) const {
  return p_ass_info_ptr_list_.size();
}
int assTableModel::columnCount(const QModelIndex &parent) const {
  return 5;
}
QVariant assTableModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;
  if (index.row() >= p_ass_info_ptr_list_.size()) return var;

  auto ass = p_ass_info_ptr_list_[index.row()];
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (index.column()) {
      case 0:
        var = QString("v%1").arg(ass->getVersionP(),
                                 4, 10, QLatin1Char('0'));
        break;
      case 1:var = DOTOS(ass->getInfoP().back());
        break;
      case 2:var = DOTOS(ass->getUser());
        break;
      case 3:var = DOTOS(ass->getSuffixes());
        break;
      case 4:var = ass->getIdP();
        break;
      default:var = "";
        break;
    }
  } else if (role == Qt::UserRole) {
    var = QVariant::fromValue(ass);
  } else
    var = QVariant();
  return var;
}
QVariant assTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  QString str;
  if (orientation == Qt::Horizontal) {
    switch (section) {
      case 0:str = tr("版本");
        break;
      case 1:str = tr("信息");
        break;
      case 2:str = tr("制作人");
        break;
      case 3:str = tr("后缀");
        break;
      case 4:str = QString("id");
        break;
      default:str = "";
        break;
    }
  } else
    str = QString(section);
  return str;
}
bool assTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  if (index.row() >= p_ass_info_ptr_list_.size()) return false;

  if (index.column() == 1 && role == Qt::EditRole) {
    if (!value.toString().isEmpty() &&
        value.toString().toStdString() != p_ass_info_ptr_list_[index.row()]->getInfoP().back()) {
      DOODLE_LOG_INFO << p_ass_info_ptr_list_[index.row()]->getInfoP().back().c_str();
      p_ass_info_ptr_list_[index.row()]->setInfoP(value.toString().toStdString());
      p_ass_info_ptr_list_[index.row()]->updateSQL();
      dataChanged(index, index);
      return true;
    }
    return false;
  } else if (role == Qt::UserRole) {
    if (!value.canConvert<doCore::assInfoPtr>()) return false;
    p_ass_info_ptr_list_[index.row()] = value.value<doCore::assInfoPtr>();
    p_ass_info_ptr_list_[index.row()]->insert();
    dataChanged(index, index);
    return true;
  } else {
    return false;
  }

}
Qt::ItemFlags assTableModel::flags(const QModelIndex &index) const {
  if (index.column() == 1)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
  else
    return QAbstractTableModel::flags(index);
}
bool assTableModel::insertRows(int position, int rows, const QModelIndex &parent) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  beginInsertColumns(QModelIndex(), 0, 4);
  for (int row = 0; row < rows; ++row) {
    p_ass_info_ptr_list_.insert(p_ass_info_ptr_list_.begin() + position,
                                std::make_shared<doCore::assFileSqlInfo>());
    p_ass_info_ptr_list_[position]->setAssType(doCore::coreDataManager::get().getAssTypePtr());
  }
  endInsertColumns();
  endInsertRows();
  return true;
}
void assTableModel::init(const doCore::assTypePtr &file_type_ptr) {
  auto tmp_list = doCore::assFileSqlInfo::getAll(file_type_ptr);
  clear();
  beginInsertRows(QModelIndex(), 0, tmp_list.size() - 1);
  p_ass_info_ptr_list_ = tmp_list;
  doCore::coreDataManager::get().setAssTypePtr(file_type_ptr);
  endInsertRows();
}
void assTableModel::clear() {
  if (p_ass_info_ptr_list_.empty()) return;
  beginResetModel();
  p_ass_info_ptr_list_.clear();
  endResetModel();
}

DOODLE_NAMESPACE_E

