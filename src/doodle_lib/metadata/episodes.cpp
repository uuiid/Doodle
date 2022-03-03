#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/metadata/episodes.h>

namespace doodle {

episodes::episodes()
    : p_episodes(-1) {
}

episodes::episodes(int64_t in_episodes)
    : p_episodes(in_episodes) {
  chick_true<doodle_error>(p_episodes >= 0, DOODLE_LOC, "集数无法为负");
}

// Episodes::~Episodes() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

const int64_t& episodes::get_episodes() const noexcept {
  return p_episodes;
}

void episodes::set_episodes(const int64_t& Episodes_) {
  chick_true<doodle_error>(Episodes_ >= 0, DOODLE_LOC, "集数无法为负");
  p_episodes = Episodes_;
}

std::string episodes::str() const {
  return fmt::format("ep{:04d}", p_episodes);
}

bool episodes::operator<(const episodes& in_rhs) const {
  //  return std::tie(static_cast<const doodle::metadata&>(*this), p_episodes) < std::tie(static_cast<const doodle::metadata&>(in_rhs), in_rhs.p_episodes);
  return std::tie(p_episodes) < std::tie(in_rhs.p_episodes);
}
bool episodes::operator>(const episodes& in_rhs) const {
  return in_rhs < *this;
}
bool episodes::operator<=(const episodes& in_rhs) const {
  return !(in_rhs < *this);
}
bool episodes::operator>=(const episodes& in_rhs) const {
  return !(*this < in_rhs);
}

bool episodes::analysis(const std::string& in_path) {
  static std::regex reg{R"(ep_?(\d+))", std::regex_constants::icase};
  std::smatch k_match{};
  const auto& k_r = std::regex_search(in_path, k_match, reg);
  if (k_r) {
    p_episodes = std::stoi(k_match[1].str());
  }
  return k_r;
}

void episodes::analysis_static(const entt::handle& in_handle,
                               const FSys::path& in_path) {
  episodes k_eps{};
  if (k_eps.analysis(in_path))
    in_handle.emplace<episodes>(k_eps);
}

bool episodes::operator==(const episodes& in_rhs) const {
  return p_episodes == in_rhs.p_episodes;
}
bool episodes::operator!=(const episodes& in_rhs) const {
  return !(in_rhs == *this);
}

}  // namespace doodle
