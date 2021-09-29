//
// Created by TD on 2021/9/29.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/base_windwos.h>

namespace doodle {

class DOODLELIB_API widget_register : public details::no_copy {
 public:
  using widget_map = std::map<std::type_index, std::weak_ptr<base_widget> >;
  widget_map p_widget;

  widget_register() : p_widget(){};
  inline widget_map& get() { return p_widget; };
  inline const widget_map& get() const { return p_widget; };

  template <class widget_type>
  std::shared_ptr<widget_type> get_widget() {
    auto ptr = p_widget.at(std::type_index{typeid(widget_type)});
    if (ptr.expired())
      return nullptr;
    return std::dynamic_pointer_cast<widget_type>(ptr.lock());
  };
};
}  // namespace doodle
