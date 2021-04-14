#pragma once

#include <doodle_GUI/doodle_global.h>

namespace doodle {
class ShotListModel {
  std::vector<ShotPtr> p_data;

  EpisodesPtr p_episodes;

 public:
  void setList(const decltype(p_data) &list, const EpisodesPtr &Episodes_ = {});

  void addShot_(int position, const ShotPtr &in_Shot);
  void setEpisodes_(const EpisodesPtr &Episodes_);
  EpisodesPtr &Episodes_();
  void removeShot(int position);

  std::vector<ShotPtr> Shots_() const;
};

}  // namespace doodle