#include "ref_base.h"

namespace doodle::gui {
void base_edit::init(const entt::handle &in) {
  is_modify = false;
  init_(in);
}
void base_edit::save(const entt::handle &in)  {
  if (is_modify) {
    save_(in);
    is_modify = false;
  }
}
}  // namespace doodle::gui
