//
// Created by TD on 2021/5/14.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {
class ContextMenu {
  wxMenu* p_menu;
  Metadata* p_Metadata;
  friend Project;
  friend Episodes;
  friend Shot;
  friend Assets;
  friend AssetsFile;

  void createMenu(const ProjectPtr      &in_data) const;
  void createMenu(const EpisodesPtr     &in_data) const;
  void createMenu(const ShotPtr         &in_data) const;
  void createMenu(const AssetsPtr       &in_data) const;
  void createMenu(const AssetsFilePtr   &in_data) const;

 public:
  explicit ContextMenu(wxMenu* in_menu, Metadata* in_metadata);
  void createMenu() ;

};

}  // namespace doodle
