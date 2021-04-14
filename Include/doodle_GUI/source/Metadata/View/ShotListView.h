#pragma once
#include <doodle_GUI/doodle_global.h>

namespace doodle {
class ShotListModel;

class ShotListView {
 public:
  ShotListView();

 private:
  void addShot();
  void addShotAb();
  void removeShot();
};

class ShotListDialog {
  EpisodesPtr p_episodes;
  std::vector<ShotPtr> p_shots;
  ShotListModel* p_shot_model;

 public:
  ShotListDialog();
  static std::tuple<EpisodesPtr, std::vector<ShotPtr>> getShotList();
};
}  // namespace doodle