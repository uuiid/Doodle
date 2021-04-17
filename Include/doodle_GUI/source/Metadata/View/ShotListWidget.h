#pragma once
#include <doodle_GUI/doodle_global.h>
#include <wx/listctrl.h>

namespace doodle {

class ShotListWidget : public wxListCtrl {
  EpisodesPtr p_episodes;
  std::vector<ShotPtr> p_shots;

 public:
  explicit ShotListWidget(wxWindow *parent, wxWindowID id);

  virtual wxString OnGetItemText(long item, long column) const override;

 private:
  void addShot();
  void addShotAb();
  void removeShot();
};

class ShotListDialog {
  EpisodesPtr p_episodes;
  std::vector<ShotPtr> p_shots;

 public:
  ShotListDialog();
  static std::tuple<EpisodesPtr, std::vector<ShotPtr>> getShotList();
};
}  // namespace doodle