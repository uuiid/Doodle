#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <DoodleLib/Metadata/Shot.h>
#include <DoodleLib/Metadata/episodes.h>
#include <DoodleLib/Metadata/metadata_factory.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::Shot)
namespace doodle {

Shot::Shot()
    : metadata(),
      p_shot(-1),
      p_shot_ab("None") {
  p_type = meta_type::folder;
}

Shot::Shot(std::weak_ptr<metadata> in_metadata,
           decltype(p_shot) in_shot,
           decltype(p_shot_ab) in_shot_ab)
    : metadata(std::move(in_metadata)),
      p_shot(in_shot),
      p_shot_ab(std::move(in_shot_ab)) {
  p_type = meta_type::folder;
  if (p_shot < 0)
    throw DoodleError{"shot无法为负"};
}

const int64_t& Shot::getShot() const noexcept {
  return p_shot;
}

void Shot::setShot(const int64_t& in_shot) {
  if (in_shot <= 0)
    throw DoodleError{"shot无法为负"};

  p_shot = in_shot;
  saved(true);
}

const std::string& Shot::getShotAb() const noexcept {
  return p_shot_ab;
}

Shot::ShotAbEnum Shot::getShotAb_enum() const noexcept {
  return magic_enum::enum_cast<ShotAbEnum>(p_shot_ab).value_or(ShotAbEnum::None);
}

void Shot::setShotAb(const std::string& ShotAb) noexcept {
  p_shot_ab = ShotAb;
  saved(true);
}
EpisodesPtr Shot::getEpisodesPtr() const {
  auto k_ptr = std::dynamic_pointer_cast<episodes>(getParent());
  if (!k_ptr)
    throw nullptr_error("没有集数");
  return k_ptr;
}

void Shot::setEpisodesPtr(const EpisodesPtr& Episodes_) noexcept {
  Episodes_->child_item.push_back_sig(shared_from_this());
}
std::string Shot::str() const {
  return fmt::format("sc{:04d}{}", p_shot, p_shot_ab == "None" ? "" : p_shot_ab);
}
bool Shot::operator<(const Shot& rhs) const {
  return std::tie(p_shot, p_shot_ab) < std::tie(rhs.p_shot, rhs.p_shot_ab);
}
bool Shot::operator>(const Shot& rhs) const {
  return rhs < *this;
}
bool Shot::operator<=(const Shot& rhs) const {
  return !(rhs < *this);
}
bool Shot::operator>=(const Shot& rhs) const {
  return !(*this < rhs);
}

bool Shot::analysis(const std::string& in_path) {
  static std::regex reg{R"(sc_?(\d+)([a-z])?)", std::regex_constants::icase};
  std::smatch k_match{};
  const auto& k_r = std::regex_search(in_path, k_match, reg);
  if (k_r) {
    p_shot = std::stoi(k_match[1].str());
    if (k_match.size() > 2)
      p_shot_ab = k_match[2].str();
  }
  return k_r;
}

ShotPtr Shot::analysis_static(const std::string& in_path) {
  auto k_shot = new_object<Shot>();
  if (k_shot->analysis(in_path))
    return k_shot;
  else
    return {};
}

void Shot::create_menu(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(std::dynamic_pointer_cast<Shot>(shared_from_this()));
}

}  // namespace doodle
