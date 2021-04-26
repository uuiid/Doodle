// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_GUI/source/Metadata/View/ShotListWidget.h>

#include <corelib/core_Cpp.h>

#include <magic_enum.hpp>
#include <wx/spinctrl.h>
#include <doodle_GUI/source/mainWidght/mainWindows.h>
#include <wx/numdlg.h>


namespace doodle {

ShotListWidget::ShotListWidget(wxWindow *parent, wxWindowID id)
    : wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize,
                 wxLC_HRULES | wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_VIRTUAL | wxLB_SINGLE),
      p_episodes(),
      p_shots(),
      p_addShot_id(NewControlId()),
      p_addShotAb_id(NewControlId()),
      p_removeShot_id(NewControlId()) {
  this->AppendColumn(L"镜头");
  this->SetItemCount(p_shots.size());

  this->Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent &event) {
    auto menu = wxMenu{};
    menu.Append(p_addShot_id, wxString::FromUTF8("添加镜头"));
    menu.Append(p_addShotAb_id, wxString::FromUTF8("添加Ab镜头"));
    menu.Append(p_removeShot_id, wxString::FromUTF8("删除镜头"));

    menu.Bind(
        wxEVT_MENU,
        [this](wxCommandEvent &event) {
          this->addShot();
        },
        p_addShot_id);
    menu.Bind(
        wxEVT_MENU,
        [this](wxCommandEvent &event) {
          this->addShotAb();
        },
        p_addShotAb_id);
    menu.Bind(
        wxEVT_MENU,
        [this](wxCommandEvent &event) {
          this->removeShot();
        },
        p_removeShot_id);
    PopupMenu(&menu);
  });
}

wxString ShotListWidget::OnGetItemText(long item, long column) const {
  if (column == 0)
    return wxString::FromUTF8(p_shots[item]->str());
  else
    return {};
}

void ShotListWidget::setShots(const std::vector<ShotPtr> &in_Shots) noexcept {
  p_shots = in_Shots;
  this->SetItemCount(p_shots.size());
}

const std::vector<ShotPtr> &ShotListWidget::Shots_() const noexcept {
  return p_shots;
}

void ShotListWidget::setEpisodes(const EpisodesPtr &in_Episodes) noexcept {
  p_episodes = in_Episodes;
}

const EpisodesPtr &ShotListWidget::Episodes_() const noexcept {
  return p_episodes;
}

void ShotListWidget::addShot() {
  if (GetSelectedItemCount() != 1) return;

  auto shot_dig = wxNumberEntryDialog{
      this,
      wxString::FromUTF8("选择镜头号"),
      wxString::FromUTF8("选择镜头号"),
      wxString::FromUTF8("镜头号"),
      1, 0, 9999};

  if (shot_dig.ShowModal() == wxID_OK) {
    auto shot = shot_dig.GetValue();
    p_shots.emplace_back(std::make_shared<Shot>(shot, std::string{}, p_episodes));
    std::sort(p_shots.begin(), p_shots.end(), [](ShotPtr &L, ShotPtr &R) { return *L < *R; });
  }
  SetItemCount(p_shots.size());
  this->Refresh();
}

void ShotListWidget::addShotAb() {
  if (GetSelectedItemCount() != 1) return;

  long itemIndex = -1;
  while ((itemIndex = GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND) {
    wxArrayString wxarrstr{};
    for (auto e : magic_enum::enum_names<Shot::ShotAbEnum>()) {
      wxarrstr.Add(std::string{e});
    }

    auto shot_dig = wxSingleChoiceDialog{
        this,
        wxString::FromUTF8("选择A镜号"),
        wxString::FromUTF8("AB号"),
        wxarrstr};

    if (shot_dig.ShowModal() == wxID_OK) {
      auto shotAB = shot_dig.GetStringSelection();
      auto shot   = p_shots[itemIndex]->Shot_();
      p_shots.emplace_back(std::make_shared<Shot>(shot, shotAB.ToStdString(wxConvUTF8), p_episodes));
      std::sort(p_shots.begin(), p_shots.end(), [](ShotPtr &L, ShotPtr &R) { return *L < *R; });
    }
    break;
  }
  SetItemCount(p_shots.size());
  this->Refresh();
}

void ShotListWidget::removeShot() {
  if (GetSelectedItemCount() != 1) return;

  long itemIndex = -1;
  while ((itemIndex = GetNextItem(itemIndex, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND) {
    p_shots.erase(p_shots.begin() + itemIndex);
    break;
  }
  SetItemCount(p_shots.size());
  this->Refresh();
}

ShotListDialog::ShotListDialog(wxWindow *parent, wxWindowID id)
    : wxDialog(parent, id, wxString::FromUTF8("镜头信息"),
               wxDefaultPosition,
               wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      p_episodes(),
      p_shots() {
  auto layout = new wxBoxSizer{wxVERTICAL};
  auto k_eps  = new wxSpinCtrl{this};
  k_eps->SetValue(1);
  k_eps->SetRange(0, 9999);
  layout->Add(k_eps, wxSizerFlags{0}.Expand());

  p_shots_widget = new ShotListWidget{this};
  layout->Add(p_shots_widget, wxSizerFlags{0}.Expand())->SetProportion(1);

  auto sub_layout = new wxBoxSizer{wxHORIZONTAL};

  auto k_ok = new wxButton{this, wxID_ANY, wxString::FromUTF8("确认")};
  auto k_no = new wxButton{this, wxID_ANY, wxString::FromUTF8("取消")};
  sub_layout->Add(k_ok, wxSizerFlags{0}.Expand())->SetProportion(1);
  sub_layout->Add(k_no, wxSizerFlags{0}.Expand())->SetProportion(1);

  layout->Add(sub_layout, wxSizerFlags{0}.Expand())->SetProportion(0);

  // this->CreateStdDialogButtonSizer(wxOK | wxCANCEL);

  this->SetSizer(layout);
  layout->SetSizeHints(this);

  k_ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) { this->EndModal(wxID_OK); });
  k_no->Bind(wxEVT_BUTTON, [this](wxCommandEvent &event) { this->EndModal(wxID_CANCEL); });
  k_eps->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent &event) {
    auto value = event.GetInt();
    this->p_episodes->setEpisodes_(value);
  });
}

std::tuple<EpisodesPtr, std::vector<ShotPtr>> ShotListDialog::getShotList() {
  auto shotDig = ShotListDialog{wxGetApp().GetTopWindow()};

  shotDig.p_episodes = std::make_shared<Episodes>(1);
  auto &set          = coreSet::getSet();
  const auto k_len   = set.gettUe4Setting().ShotEnd();
  for (auto i = set.gettUe4Setting().ShotStart(); i < k_len; ++i) {
    shotDig.p_shots.emplace_back(std::make_shared<Shot>(i, std::string{}, shotDig.p_episodes));
  }

  shotDig.p_shots_widget->setEpisodes(shotDig.p_episodes);
  shotDig.p_shots_widget->setShots(shotDig.p_shots);

  if (shotDig.ShowModal() == wxID_OK) {
    return {shotDig.p_shots_widget->Episodes_(), shotDig.p_shots_widget->Shots_()};
  } else {
    return {};
  }
}

}  // namespace doodle