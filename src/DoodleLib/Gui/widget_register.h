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
  widget_map widget;

  widget_register() : widget(){};
  inline widget_map& get() { return widget; };
  inline const widget_map& get() const { return widget; };
};
}  // namespace doodle
