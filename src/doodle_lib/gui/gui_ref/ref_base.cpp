#include "ref_base.h"
#include <metadata/metadata.h>
#include <metadata/time_point_wrap.h>
namespace doodle::gui {

void edit_interface::init(const entt::handle &in) {
  data_->is_modify = false;
  init_(in);
}
void edit_interface::save(const entt::handle &in) {
  data_->is_modify = false;
  save_(in);
}
void edit_interface::set_modify(bool is_modify) {
  data_->is_modify = is_modify;
}
edit_interface::edit_interface()
    : data_(std::make_unique<gui_data_interface>()) {}

edit_interface::~edit_interface() = default;

void database_edit::save(const entt::handle &in) {
  if (data_->is_modify) {
    if (in.all_of<database>()) {
      in.patch<database>(database::save);
    }
  }
  edit_interface::save(in);
}

}  // namespace doodle::gui
