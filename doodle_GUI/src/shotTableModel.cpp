//
// Created by teXiao on 2020/10/14.
//

#include "shotTableModel.h"
#include "Logger.h"
#include "src/filetype.h"
#include "src/shotfilesqlinfo.h"
#include <QJsonArray>

DOODLE_NAMESPACE_S
shotTableModel::shotTableModel(QObject *parent)
    : QAbstractTableModel(parent), p_shot_info_ptr_list_(), p_type_ptr_(nullptr) {}
int shotTableModel::rowCount(const QModelIndex &parent) const {
  return p_shot_info_ptr_list_.size();
}
int shotTableModel::columnCount(const QModelIndex &parent) const {
  return 5;
}
QVariant shotTableModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  if (!index.isValid()) return var;
  if (index.row() >= p_shot_info_ptr_list_.size()) return var;

  auto shot = p_shot_info_ptr_list_[index.row()];
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (index.column()) {
      case 0:var = QString("v%1").arg(shot->getVersionP(), 4, 10, QLatin1Char('0'));
        break;
      case 1:var = shot->getInfoP().last().toVariant();
        break;
      case 2:var = shot->getUserP();
        break;
      case 3:
        var = shot->getSuffixes();
        break;
      case 4:var = shot->getIdP();
        break;
      default:var = QVariant();
        break;
    }
  } else if (role == Qt::UserRole) {
    switch (index.column()) {
      case 0:var = shot->getVersionP();
        break;
      case 1:var = shot->getInfoP().last().toVariant();
        break;
      case 2:var = shot->getUserP();
        break;
      case 3:
        var = shot->getSuffixes();
        break;
      case 4:var = QVariant::fromValue(shot);
        break;
      default:var = QVariant();
        break;
    }
  } else {
    var =  QVariant();
  }
  return var;
}
QVariant shotTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
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

bool shotTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  if (index.row() >= p_shot_info_ptr_list_.size()) return false;

  if (index.column() == 1 && role == Qt::EditRole) {
    if (!value.toString().isEmpty() &&
        value.toString() != p_shot_info_ptr_list_[index.row()]->getInfoP().last().toString()) {
      DOODLE_LOG_INFO << p_shot_info_ptr_list_[index.row()]->getInfoP().last().toString();
      p_shot_info_ptr_list_[index.row()]->setInfoP(value.toString());
      p_shot_info_ptr_list_[index.row()]->updateSQL();
      dataChanged(index, index);
      return true;
    }
    return false;
  } else if (role == Qt::UserRole) {
    if (!value.canConvert<doCore::shotInfoPtr>()) return false;
    p_shot_info_ptr_list_[index.row()] = value.value<doCore::shotInfoPtr>();
    p_shot_info_ptr_list_[index.row()]->insert();
    dataChanged(index, index);
    return true;
  } else {
    return false;
  }

}
Qt::ItemFlags shotTableModel::flags(const QModelIndex &index) const {
  if (index.column() == 1)
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
  else
    return QAbstractTableModel::flags(index);
}

bool shotTableModel::insertRows(int position, int rows, const QModelIndex &parent) {
//  auto version = p_shot_info_ptr_list_[0]->getVersionP();
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_shot_info_ptr_list_.insert(position,
                                 doCore::shotInfoPtr(new doCore::shotFileSqlInfo));
    p_shot_info_ptr_list_[position]->setFileType(p_type_ptr_);
  }
  endInsertRows();
  return true;
}
void shotTableModel::init(const doCore::fileTypePtr &file_type_ptr) {
  auto tmp_list = doCore::shotFileSqlInfo::getAll(file_type_ptr);
  clear();
  beginInsertRows(QModelIndex(), 0, tmp_list.size() - 1);
  p_type_ptr_ = file_type_ptr;
  p_shot_info_ptr_list_ = tmp_list;
  endInsertRows();
}
void shotTableModel::clear() {
  p_type_ptr_ = nullptr;
  if (p_shot_info_ptr_list_.isEmpty()) return;
  beginResetModel();
  p_shot_info_ptr_list_.clear();
  endResetModel();

}

DOODLE_NAMESPACE_E
