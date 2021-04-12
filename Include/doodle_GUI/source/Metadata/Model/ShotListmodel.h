#pragma once

#include <doodle_GUI/doodle_global.h>
#include <QtCore/QAbstractListModel>
#include <QtWidgets/QDialog>

namespace doodle {
class ShotListModel : public QAbstractListModel {
  Q_OBJECT
  std::vector<ShotPtr> p_data;

  EpisodesPtr p_episodes;

 public:
  ShotListModel(QWidget *parent = nullptr);

  [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

  //编辑标识
  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

  void setList(const decltype(p_data) &list, const EpisodesPtr &Episodes_ = {});

  void addShot_(int position, const ShotPtr &in_Shot);
  void setEpisodes_(const EpisodesPtr &Episodes_);
  EpisodesPtr &Episodes_();
  void removeShot(int position);

  std::vector<ShotPtr> Shots_() const;
};

}  // namespace doodle