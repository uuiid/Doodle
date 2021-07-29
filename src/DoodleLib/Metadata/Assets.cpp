//
// Created by teXiao on 2021/4/27.
//

#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/PinYin/convert.h>
#include <Gui/factory/menu_factory.h>

namespace doodle {
Assets::Assets()
    : Metadata(),
      p_name(),
      p_name_enus() {
  p_type = meta_type::folder;
}

Assets::Assets(std::weak_ptr<Metadata> in_metadata, std::string in_name)
    : Metadata(std::move(in_metadata)),
      p_name(std::move(in_name)),
      p_name_enus(convert::Get().toEn(p_name)) {
  p_type = meta_type::folder;
}

// Assets::~Assets() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

std::string Assets::str() const {
  if (p_name_enus.empty())
    return convert::Get().toEn(p_name);
  return p_name_enus;
}
std::string Assets::showStr() const {
  return p_name;
}

bool Assets::operator<(const Assets& in_rhs) const {
  //  return std::tie(static_cast<const doodle::Metadata&>(*this), p_name) < std::tie(static_cast<const doodle::Metadata&>(in_rhs), in_rhs.p_name);
  return std::tie(p_name) < std::tie(in_rhs.p_name);
}
bool Assets::operator>(const Assets& in_rhs) const {
  return in_rhs < *this;
}
bool Assets::operator<=(const Assets& in_rhs) const {
  return !(in_rhs < *this);
}
bool Assets::operator>=(const Assets& in_rhs) const {
  return !(*this < in_rhs);
}


const std::string& Assets::getName1() const {
  return p_name;
}
void Assets::setName1(const std::string& in_name) {
  p_name = in_name;
  if (p_name_enus.empty())
    p_name_enus = convert::Get().toEn(p_name);
  saved(true);
  sig_change();
}
const std::string& Assets::getNameEnus() const {
  return p_name_enus;
}
void Assets::setNameEnus(const std::string& in_nameEnus) {
  p_name_enus = in_nameEnus;
  saved(true);
  sig_change();
}
void Assets::create_menu(const menu_factory_ptr& in_factoryPtr) {
  in_factoryPtr->create_menu(std::dynamic_pointer_cast<Assets>(shared_from_this()));
}
}  // namespace doodle
