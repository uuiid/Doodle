
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/season.h>

namespace doodle {

episodes::episodes() : p_episodes(-1) {}

episodes::episodes(std::int32_t in_episodes) : p_episodes(in_episodes) {
  DOODLE_CHICK(p_episodes >= 0, doodle_error{"集数无法为负"});
}

// Episodes::~Episodes() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

const std::int32_t& episodes::get_episodes() const noexcept { return p_episodes; }

void episodes::set_episodes(const std::int32_t& Episodes_) {
  DOODLE_CHICK(Episodes_ >= 0, doodle_error{"集数无法为负"});
  p_episodes = Episodes_;
}

bool episodes::operator<(const episodes& in_rhs) const {
  //  return std::tie(static_cast<const doodle::metadata&>(*this), p_episodes) < std::tie(static_cast<const
  //  doodle::metadata&>(in_rhs), in_rhs.p_episodes);
  return std::tie(p_episodes) < std::tie(in_rhs.p_episodes);
}
bool episodes::operator>(const episodes& in_rhs) const { return in_rhs < *this; }
bool episodes::operator<=(const episodes& in_rhs) const { return !(in_rhs < *this); }
bool episodes::operator>=(const episodes& in_rhs) const { return !(*this < in_rhs); }

bool episodes::analysis(const std::string& in_path) {
  static std::regex reg{R"(ep_?(\d+))", std::regex_constants::icase};
  std::smatch k_match{};
  const auto& k_r = std::regex_search(in_path, k_match, reg);
  if (k_r) {
    p_episodes = std::stoi(k_match[1].str());
  }
  return k_r;
}

bool episodes::analysis_static(const entt::handle& in_handle, const FSys::path& in_path) {
  episodes k_eps{};
  if (k_eps.analysis(in_path)) {
    in_handle.emplace<episodes>(k_eps);
    return true;
  }
  return false;
}

bool episodes::operator==(const episodes& in_rhs) const { return p_episodes == in_rhs.p_episodes; }
bool episodes::operator!=(const episodes& in_rhs) const { return !(in_rhs == *this); }

}  // namespace doodle
