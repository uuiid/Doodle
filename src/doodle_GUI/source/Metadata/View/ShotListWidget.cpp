// Fill out your copyright notice in the Description page of Project Settings.
#include <doodle_GUI/source/Metadata/View/ShotListWidget.h>

#include <corelib/core_Cpp.h>

#include <magic_enum.hpp>

namespace doodle {

ShotListWidget::ShotListWidget(wxWindow *parent, wxWindowID id)
    : wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize,
                 wxLC_VIRTUAL |
                     wxLC_REPORT |
                     wxLC_SINGLE_SEL |
                     wxLC_HRULES,
                 wxDefaultValidator, wxString{"ShotLists"}),
      p_episodes(),
      p_shots() {
  this->AppendColumn("test");
  this->SetItemCount(10);
}

wxString ShotListWidget::OnGetItemText(long item, long column) const {
  if (column == 0)
    return wxString::FromUTF8("test");
  else
    return {};
}

void ShotListWidget::addShot() {
}

void ShotListWidget::addShotAb() {
}

void ShotListWidget::removeShot() {
}

}  // namespace doodle