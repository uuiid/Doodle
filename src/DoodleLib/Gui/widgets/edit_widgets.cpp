//
// Created by TD on 2021/9/23.
//

#include "edit_widgets.h"

#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <DoodleLib/Metadata/Metadata.h>
namespace doodle {
edit_widgets::edit_widgets()
    : p_meta_parent(),
      p_factory(new_object<attr_project>()) {
}

void edit_widgets::frame_render() {
  p_factory->render();
}

void edit_widgets::set_factort(const attribute_factory_ptr& in_factory) 
{
  p_factory = in_factory;
}
}  // namespace doodle