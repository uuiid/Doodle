// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_GUI/source/Metadata/Model/ShotListmodel.h>
#include <boost/numeric/conversion/cast.hpp>
namespace doodle {
ShotListModel::ShotListModel(QWidget *parent)
    : QAbstractListModel(parent),
      p_data(),
      p_episodes() {
}

int ShotListModel::rowCount(const QModelIndex &parent) const {
  return boost::numeric_cast<int>(p_data.size());
}

QVariant ShotListModel::data(const QModelIndex &index, int role) const {
  QVariant var{};
  if (!index.isValid())
    return var;
  if (index.row() >= p_data.size())
    return var;

  auto k_data = p_data[index.row()];
  switch (role) {
    case Qt::DisplayRole:
      var = QString::fromStdString(k_data->str());
      break;
    case Qt::UserRole:
      var = QVariant::fromValue(k_data.get());
    default:

      break;
  }
  return var;
}

Qt::ItemFlags ShotListModel::flags(const QModelIndex &index) const {
  return Qt::ItemIsEditable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}

void ShotListModel::setList(const decltype(p_data) &list, const EpisodesPtr &Episodes_) {
  this->beginResetModel();
  p_data = list;
  this->endResetModel();
  if (Episodes_) {
    p_episodes = Episodes_;
  }
}

void ShotListModel::addShot_(int position, const ShotPtr &in_Shot) {
  beginInsertRows(QModelIndex{}, position, position + 1);
  p_data.insert(p_data.begin() + position, in_Shot);
  endInsertRows();
}

void ShotListModel::setEpisodes_(const EpisodesPtr &Episodes_) {
  p_episodes = Episodes_;
}

EpisodesPtr &ShotListModel::Episodes_() {
  return p_episodes;
}

void ShotListModel::removeShot(int position) {
  beginRemoveRows(QModelIndex{}, position, position + 1);
  p_data.erase(p_data.begin() + position);
  endRemoveRows();
}

std::vector<ShotPtr> ShotListModel::Shots_() const {
  return p_data;
}

}  // namespace doodle