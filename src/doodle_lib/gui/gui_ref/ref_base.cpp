#include "ref_base.h"
#include <metadata/metadata.h>
namespace doodle::gui {

void edit_interface::init(const entt::handle &in) {
  data->is_modify = false;
  init_(in);
}
void edit_interface::save(const entt::handle &in) {
  data->is_modify = false;
}
void edit_interface::set_modify(bool is_modify) {
  data->is_modify = is_modify;
}
edit_interface::edit_interface()  = default;
edit_interface::~edit_interface() = default;

void database_edit::save(const entt::handle &in) {
  if (data->is_modify) {
    if (in.all_of<database>()) {
      in.patch<database>(database::save);
    }
    save_(in);
    data->is_modify = false;
  }
}

}  // namespace doodle::gui
