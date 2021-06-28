//
// Created by TD on 2021/6/28.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <nana/gui/widgets/listbox.hpp>
namespace doodle{

nana::listbox::oresolver&  operator << (nana::listbox::oresolver& oor,const ProjectPtr& prj);
nana::listbox::iresolver&  operator >> (nana::listbox::iresolver& oor,ProjectPtr& prj);



class DOODLELIB_API project_widget {
  nana::listbox p_list_box;

 public:
  explicit project_widget(nana::window in_window);

  nana::listbox& get_listbox();
};
}
