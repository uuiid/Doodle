// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_GUI/source/SettingWidght/Model/ProjectModel.h>
#include <boost/numeric/conversion/cast.hpp>

namespace doodle {

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractListModel(parent),
      p_data(coreSet::getSet().getAllProjects()) {
}

int ProjectModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_data.size());
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  if (index.row() >= p_data.size())
    return QVariant();

  QVariant qvar{};
  switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole: {
      qvar = QString::fromStdString(p_data[index.row()]->Name());
      break;
    }
    case Qt::UserRole:
      qvar = QVariant::fromValue(p_data[index.row()].get());
    default:
      break;
  }
  return qvar;
}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEditable;
  if (index.row() >= p_data.size())
    return Qt::ItemIsEditable;

  return Qt::ItemIsEditable | Qt::ItemIsEnabled |
         QAbstractListModel::flags(index);
}

bool ProjectModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid())
    return false;
  if (index.row() >= p_data.size())
    return false;

  switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
      p_data[index.row()]->setName(value.toString().toStdString());
      break;

    default:
      return false;
      break;
  }
  return true;
}

bool ProjectModel::insertRows(int position, int rows, const QModelIndex &index) {
  beginInsertRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    p_data.insert(p_data.begin() + position,
                  std::make_shared<Project>());
  }
  endInsertRows();
  return true;
}

bool ProjectModel::removeRows(int position, int rows, const QModelIndex &index) {
  beginRemoveRows(QModelIndex(), position, position + rows - 1);
  for (int row = 0; row < rows; ++row) {
    auto data = p_data[position];
    if (data) {
      coreSet::getSet().deleteProject(data.get());
      p_data.erase(p_data.begin() + position);
    }
  }
  endRemoveRows();
  return true;
}

void ProjectModel::init(const decltype(p_data) &data) {
  beginResetModel();
  p_data = data;
  endResetModel();
}

}  // namespace doodle