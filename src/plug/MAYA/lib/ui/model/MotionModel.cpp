#include <lib/ui/model/MotionModel.h>

#include <boost/numeric/conversion/cast.hpp>
#include <QtGui/qcolor.h>

namespace doodle::motion::ui {
MotionModel::MotionModel(QObject *parent) : QAbstractListModel(parent) {
}



int MotionModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_lists.size());
}

QVariant MotionModel::data(const QModelIndex &index, int role) const {
  auto var = QVariant{};
  if (!index.isValid()) return var;
  if (index.row() >= p_lists.size()) return var;

  auto k_data = p_lists.at(index.row());
  switch (role) {
    case Qt::DisplayRole: {
      var = QString::fromStdString(k_data->Title());
      break;
    }
    case Qt::BackgroundRole:
      var = QColor{Qt::darkRed};
      break;
    default:
      break;
  }
  return var;
}

Qt::ItemFlags MotionModel::flags(const QModelIndex &index) const {
  return Qt::ItemIsEditable | Qt::ItemIsEnabled |
         QAbstractListModel::flags(index);
}

bool MotionModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  if (index.row() >= p_lists.size()) return false;
  if (!value.isValid()) return false;

  auto k_data = p_lists.at(index.row());

  if (value.canConvert<QString>()) {
    k_data->setTitle(value.toString().toStdString());
  } else if (value.canConvert<QStringList>()) {
    k_data->setInfo(value.toStringList().first().toStdString());
  }
  return true;
}

bool MotionModel::insertRows(int position, int rows, const QModelIndex &index) {
  return false;
}

bool MotionModel::removeRows(int position, int rows, const QModelIndex &index) {
  return false;
}

void MotionModel::setLists(const std::vector<kernel::MotionFilePtr> &lists) {
  p_lists = lists;
}

}  // namespace doodle::motion::ui