//
// Created by TD on 2021/5/14.
//

#include <DoodleLib/Metadata/ContextMenu.h>
#include <DoodleLib/Metadata/Metadata.h>

namespace doodle{

ContextMenu::ContextMenu(wxMenu* in_menu, Metadata* in_metadata)
  :p_menu(std::move(in_menu)),
   p_Metadata(std::move(in_metadata)){
}

void ContextMenu::createMenu(const ProjectPtr& in_data)const {
}

void ContextMenu::createMenu(const EpisodesPtr& in_data) const{
}

void ContextMenu::createMenu(const ShotPtr& in_data) const{
  auto k_add_shot = p_menu->Append(wxID_ANY, ConvStr<wxString>("添加镜头"));
  auto k_DeleteShot = p_menu->Append(wxID_ANY, ConvStr<wxString>("删除镜头"));
  auto k_add_shotab = p_menu->Append(wxID_ANY, ConvStr("添加ab镜头"));
  p_menu->Bind(
      wxEVT_MENU,[](wxCommandEvent&in_event){},k_add_shot->GetId()
      );
}

void ContextMenu::createMenu(const AssetsPtr& in_data) const{
}

void ContextMenu::createMenu(const AssetsFilePtr& in_data) const{
}

void ContextMenu::createMenu() {
  p_Metadata->createMenu(this);
}

}
