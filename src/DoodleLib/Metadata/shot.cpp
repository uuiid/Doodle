#include <DoodleLib/Exception/exception.h>
#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <DoodleLib/Metadata/episodes.h>
#include <DoodleLib/Metadata/metadata_factory.h>
#include <DoodleLib/Metadata/shot.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::shot)
namespace doodle {

shot::shot()
    : metadata(),
      p_shot(-1),
      p_shot_ab("None") {
  p_type = meta_type::folder;
}

shot::shot(std::weak_ptr<metadata> in_metadata,
           decltype(p_shot) in_shot,
           decltype(p_shot_ab) in_shot_ab)
    : metadata(std::move(in_metadata)),
      p_shot(in_shot),
      p_shot_ab(std::move(in_shot_ab)) {
  p_type = meta_type::folder;
  if (p_shot < 0)
    throw DoodleError{"shot无法为负"};
}

const int64_t& shot::getShot() const noexcept {
  return p_shot;
}

void shot::setShot(const int64_t& in_shot) {
  if (in_shot <= 0)
    throw DoodleError{"shot无法为负"};

  p_shot = in_shot;
  saved(true);
}

const std::string& shot::getShotAb() const noexcept {
  return p_shot_ab;
}

shot::shot_ab_enum shot::getShotAb_enum() const noexcept {
  return magic_enum::enum_cast<shot_ab_enum>(p_shot_ab).value_or(shot_ab_enum::None);
}

void shot::setShotAb(const std::string& ShotAb) noexcept {
  p_shot_ab = ShotAb;
  saved(true);
}
EpisodesPtr shot::getEpisodesPtr() const {
  auto k_ptr = std::dynamic_pointer_cast<episodes>(getParent());
  if (!k_ptr)
    throw nullptr_error("没有集数");
  return k_ptr;
}

void shot::setEpisodesPtr(const EpisodesPtr& Episodes_) noexcept {
  Episodes_->child_item.push_back_sig(shared_from_this());
}
std::string shot::str() const {
  return fmt::format("sc{:04d}{}", p_shot, p_shot_ab == "None" ? "" : p_shot_ab);
}
bool shot::operator<(const shot& rhs) const {
  return std::tie(p_shot, p_shot_ab) < std::tie(rhs.p_shot, rhs.p_shot_ab);
}
bool shot::operator>(const shot& rhs) const {
  return rhs < *this;
}
bool shot::operator<=(const shot& rhs) const {
  return !(rhs < *this);
}
bool shot::operator>=(const shot& rhs) const {
  return !(*this < rhs);
}

bool shot::analysis(const std::string& in_path) {
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

ShotPtr shot::analysis_static(const std::string& in_path) {
  auto k_shot = new_object<shot>();
  if (k_shot->analysis(in_path))
    return k_shot;
  else
    return {};
}

void shot::create_menu(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(std::dynamic_pointer_cast<shot>(shared_from_this()));
}

}  // namespace doodle
