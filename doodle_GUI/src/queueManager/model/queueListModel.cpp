#include "queueListModel.h"

#include <core_Cpp.h>

#include <boost/numeric/conversion/cast.hpp>
DOODLE_NAMESPACE_S
queueListModel::queueListModel(QObject *parent)
    : QAbstractListModel(parent) {
  queueManager::appendEnd.connect([this](queueDataPtr ptr) {
    ptr->ProgressChanged.connect([=]() { this->DoodleProgressChanged(ptr); });
    this->init();
  });
  queueManager::removeData.connect([this]() { this->init(); });
}

int queueListModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_updataQueue.size());
}

QVariant queueListModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant();
  //检查有效性
  if (!index.isValid())
    return var;
  if (index.row() >= p_updataQueue.size())
    return var;

  switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
      var = QString::fromStdString(p_updataQueue[index.row()]->Name());
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(p_updataQueue[index.row()]);
      break;
    default:
      break;
  }
  return var;
}

QVariant queueListModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const {
  return QVariant();
}

Qt::ItemFlags queueListModel::flags(const QModelIndex &index) const {
  return QAbstractListModel::flags(index) | Qt::ItemIsEnabled;
}

void queueListModel::init() {
  auto data = queueManager::Get().Queue();

  clear();
  beginInsertRows(QModelIndex(), 0, boost::numeric_cast<int>(data.size()) - 1);
  p_updataQueue = data;
  endInsertRows();
}

void queueListModel::clear() {
  beginResetModel();
  p_updataQueue.clear();
  endResetModel();
}

void queueListModel::DoodleProgressChanged(const queueDataPtr &data) {
  auto rule = std::find(p_updataQueue.begin(), p_updataQueue.end(), data);
  if (rule != p_updataQueue.end()) {
    auto k_row   = std::distance(p_updataQueue.begin(), rule);
    auto k_index = index(k_row);
    dataChanged(k_index, k_index);
  }
}

DOODLE_NAMESPACE_E