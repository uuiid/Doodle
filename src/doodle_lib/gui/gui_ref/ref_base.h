#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API base_render {
 public:
  base_render() = default;
  virtual bool operator()(const entt::handle &in_handle){};
};

template <class T>
base_render make_render() {
}

class base_edit {
 protected:
  virtual void init_(const entt::handle &in)       = 0;
  virtual void save_(const entt::handle &in) const = 0;

 public:
  bool is_modify{false};
  void init(const entt::handle &in);
  virtual void render(const entt::handle &in) = 0;
  void save(const entt::handle &in);
};
}  // namespace doodle::gui
