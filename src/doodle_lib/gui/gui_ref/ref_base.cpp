#include "ref_base.h"
#include <metadata/metadata.h>
namespace doodle::gui {

void edit_interface::init(const entt::handle &in) {
  is_modify = false;
  init_(in);
}
void edit_interface::save(const entt::handle &in) {
  is_modify = false;
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
