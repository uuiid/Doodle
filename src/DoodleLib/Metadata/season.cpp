//
// Created by TD on 2021/7/29.
//

#include "season.h"

#include <Gui/factory/menu_factory.h>
namespace doodle {
season::season()
    : Metadata(),
      p_int(0) {
}
season::season(std::weak_ptr<Metadata> in_metadata, std::int32_t in_)
    : Metadata(in_metadata),
      p_int(in_) {
}

void season::set_season(std::int32_t in_) {
  p_int = in_;
}

std::int32_t season::get_season(std::int32_t in_) const {
  return p_int;
}
std::string season::str() const {
  return fmt::format("seas_{}", p_int);
}
void season::create_menu(const menu_factory_ptr& in_factoryPtr) {
  in_factoryPtr->create_menu(std::dynamic_pointer_cast<season>(shared_from_this()));
}
}  // namespace doodle
