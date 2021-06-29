//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <nana/gui.hpp>
namespace doodle {

class DOODLELIB_API menu_factory {
  CoreSet& p_set;
  std::vector<action_ptr> p_action;
  nana::window p_window;
  void create_prj_action(MetadataPtr& in_metadata);
  void create_file_action(MetadataPtr& in_metadata);
  void create_ass_action(MetadataPtr& in_metadata);
 public:
  explicit  menu_factory(nana::window in_window);
  void create_menu(MetadataPtr& in_metadata);
  std::vector<action_ptr> operator()(MetadataPtr& in_metadata);
};
}  // namespace doodle
