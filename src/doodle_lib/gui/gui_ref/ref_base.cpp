#include "ref_base.h"
#include <metadata/metadata.h>
namespace doodle::gui {
void database_edit::init(const entt::handle &in) {
  is_modify = false;
  init_(in);
}
void database_edit::save(const entt::handle &in) {
  if (is_modify) {
    if (in.all_of<database>()) {
      in.patch<database>(database::save);
    }
    save_(in);
    is_modify = false;
  }
}
}  // namespace doodle::gui
