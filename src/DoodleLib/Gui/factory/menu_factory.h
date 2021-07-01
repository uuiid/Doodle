//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <nana/gui.hpp>
#include <nana/gui/widgets/menu.hpp>
namespace doodle {

class DOODLELIB_API menu_factory {
  CoreSet& p_set;
  std::vector<action_ptr> p_action;
  nana::window p_window;
  MetadataPtr p_metadata;

 public:
  explicit menu_factory(nana::window in_window);
  void create_prj_action(MetadataPtr& in_metadata);
  void create_file_action(MetadataPtr& in_metadata);
  void create_ass_action(MetadataPtr& in_metadata);
  void operator()(nana::menu& in_menu);
};
}  // namespace doodle
