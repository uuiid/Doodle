#include "ref_base.h"
#include <metadata/metadata.h>
namespace doodle::gui {

void edit_interface::init(const entt::handle &in) {
  data_->is_modify = false;
  init_(in);
}
void edit_interface::save(const entt::handle &in) {
  if (data_->is_modify)
    save_(in);
  data_->is_modify = false;
}
void edit_interface::set_modify(bool is_modify) {
  data_->is_modify = is_modify;
  data_->edited();
}
edit_interface::edit_interface()
    : data_(std::make_unique<gui_data>()) {}

edit_interface::~edit_interface() = default;

}  // namespace doodle::gui
