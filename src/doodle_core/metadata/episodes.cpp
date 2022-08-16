
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/project.h>

namespace doodle {

episodes::episodes()
    : p_episodes(-1) {
}

episodes::episodes(int64_t in_episodes)
    : p_episodes(in_episodes) {
  DOODLE_CHICK(p_episodes >= 0,doodle_error{"集数无法为负"});
}

// Episodes::~Episodes() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

const int64_t& episodes::get_episodes() const noexcept {
  return p_episodes;
}

void episodes::set_episodes(const int64_t& Episodes_) {
  DOODLE_CHICK(Episodes_ >= 0,doodle_error{"集数无法为负"});
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
bool episodes::conjecture_season(const entt::handle& in_handle) {
  if (in_handle.all_of<season>())
    return true;

  chick_true<doodle_error>(
      g_reg()->ctx().contains<project_config::base_config>(), "缺失上下文组件");

  if (in_handle.all_of<episodes>()) {
    auto l_count  = g_reg()->ctx().at<project_config::base_config>().season_count;
    auto l_eps    = in_handle.get<episodes>().p_episodes;
    auto l_season = boost::numeric_cast<std::float_t>(l_eps) / boost::numeric_cast<std::float_t>(l_count);
    in_handle.emplace<season>(std::ceil(l_season));
    return true;
  }

  return false;
}
bool episodes::analysis_static(const entt::handle& in_handle,
                               const FSys::path& in_path) {
  episodes k_eps{};
  if (k_eps.analysis(in_path)) {
    in_handle.emplace<episodes>(k_eps);
    return true;
  }
  return false;
}

bool episodes::operator==(const episodes& in_rhs) const {
  return p_episodes == in_rhs.p_episodes;
}
bool episodes::operator!=(const episodes& in_rhs) const {
  return !(in_rhs == *this);
}

}  // namespace doodle
