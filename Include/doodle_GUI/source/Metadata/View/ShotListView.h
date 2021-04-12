#pragma once
#include <doodle_GUI/doodle_global.h>
#include <QtWidgets/QListView>
#include <QtWidgets/QDialog>

namespace doodle {
class ShotListModel;

class ShotListView : public QListView {
  Q_OBJECT
  ShotListModel* p_shot_model;

 public:
  ShotListView(QWidget* parent = nullptr);

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  void addShot();
  void addShotAb();
  void removeShot();
};

class ShotListDialog : public QDialog {
  Q_OBJECT
  EpisodesPtr p_episodes;
  std::vector<ShotPtr> p_shots;
  ShotListModel* p_shot_model;

 public:
  ShotListDialog(QWidget* parent = nullptr);
  static std::tuple<EpisodesPtr, std::vector<ShotPtr>> getShotList(QWidget* parent);
};
}  // namespace doodle