#pragma once
#include <doodle_GUI/doodle_global.h>
#include <wx/listctrl.h>

namespace doodle {

class ShotListWidget : public wxListCtrl {
  EpisodesPtr p_episodes;
  std::vector<ShotPtr> p_shots;

  wxWindowIDRef p_addShot_id;
  wxWindowIDRef p_addShotAb_id;
  wxWindowIDRef p_removeShot_id;

 public:
  explicit ShotListWidget(wxWindow *parent, wxWindowID id = wxID_ANY);

  virtual wxString OnGetItemText(long item, long column) const override;

  const EpisodesPtr &Episodes_() const noexcept;
  void setEpisodes(const EpisodesPtr &in_Episodes) noexcept;

  const std::vector<ShotPtr> &Shots_() const noexcept;
  void setShots(const std::vector<ShotPtr> &in_Shots) noexcept;

 private:
  void addShot();
  void addShotAb();
  void removeShot();
};

class ShotListDialog : public wxDialog {
  EpisodesPtr p_episodes;
  std::vector<ShotPtr> p_shots;
  ShotListWidget *p_shots_widget;

 public:
  ShotListDialog(wxWindow *parent, wxWindowID id = wxID_ANY);
  static std::tuple<EpisodesPtr, std::vector<ShotPtr>> getShotList();
};
}  // namespace doodle