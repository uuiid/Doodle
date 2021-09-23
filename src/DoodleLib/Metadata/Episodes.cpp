#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Metadata/MetadataFactory.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::Episodes)
namespace doodle {

Episodes::Episodes()
    : Metadata(),
      p_episodes(-1) {
  p_type = meta_type::folder;
}

Episodes::Episodes(std::weak_ptr<Metadata> in_metadata, int64_t in_episodes)
    : Metadata(std::move(in_metadata)),
      p_episodes(in_episodes) {
  p_type = meta_type::folder;
  if (p_episodes < 0)
    throw DoodleError("集数无法为负");
}

// Episodes::~Episodes() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

const int64_t& Episodes::getEpisodes() const noexcept {
  return p_episodes;
}

void Episodes::setEpisodes(const int64_t& Episodes_) {
  if (Episodes_ <= 0)
    throw DoodleError("集数无法为负");
  p_episodes = Episodes_;
  saved(true);
  sig_change();
}

std::string Episodes::str() const {
  return fmt::format("ep{:04d}", p_episodes);
}

bool Episodes::operator<(const Episodes& in_rhs) const {
  //  return std::tie(static_cast<const doodle::Metadata&>(*this), p_episodes) < std::tie(static_cast<const doodle::Metadata&>(in_rhs), in_rhs.p_episodes);
  return std::tie(p_episodes) < std::tie(in_rhs.p_episodes);
}
bool Episodes::operator>(const Episodes& in_rhs) const {
  return in_rhs < *this;
}
bool Episodes::operator<=(const Episodes& in_rhs) const {
  return !(in_rhs < *this);
}
bool Episodes::operator>=(const Episodes& in_rhs) const {
  return !(*this < in_rhs);
}

bool Episodes::analysis(const std::string& in_path) {
  static std::regex reg{R"(ep_?(\d+))", std::regex_constants::icase};
  std::smatch k_match{};
  const auto& k_r = std::regex_search(in_path, k_match, reg);
  if (k_r) {
    p_episodes = std::stoi(k_match[1].str());
  }
  return k_r;
}

EpisodesPtr Episodes::analysis_static(const std::string& in_path) {
  auto k_eps = new_object<Episodes>();
  if (k_eps->analysis(in_path))
    return k_eps;
  else
    return {};
}

void Episodes::create_menu(const menu_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(std::dynamic_pointer_cast<Episodes>(shared_from_this()));
}

}  // namespace doodle

