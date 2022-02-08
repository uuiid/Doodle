#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API base_render {
 public:
  base_render() = default;
  virtual bool operator()(const entt::handle& in_handle) {};
};

template <class T>
base_render make_render() {
}

}  // namespace doodle::gui
