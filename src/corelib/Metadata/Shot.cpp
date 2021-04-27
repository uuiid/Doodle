#include <corelib/Metadata/Shot.h>
#include <corelib/Exception/Exception.h>

#include <boost/format.hpp>
namespace doodle {

Shot::Shot()
    : p_shot(-1),
      p_shot_ab(),
      p_episodes() {
}

Shot::Shot(decltype(p_shot) in_shot,
           decltype(p_shot_ab) in_shot_ab,
           decltype(p_episodes) in_episodes)
    : p_shot(in_shot),
      p_shot_ab(std::move(in_shot_ab)),
      p_episodes(std::move(in_episodes)) {
  if (p_shot < 0)
    throw DoodleError{"shot无法为负"};
}

const int64_t& Shot::Shot_() const noexcept {
  return p_shot;
}

void Shot::setShot_(const int64_t& Shot_) {
  if (Shot_ < 0)
    throw DoodleError{"shot无法为负"};

  p_shot = Shot_;
}

const std::string& Shot::ShotAb() const noexcept {
  return p_shot_ab;
}

void Shot::setShotAb(const std::string& ShotAb) noexcept {
  p_shot_ab = ShotAb;
}
EpisodesPtr Shot::Episodes_() const noexcept {
  return p_episodes.lock();
}

void Shot::setEpisodes_(const EpisodesPtr& Episodes_) noexcept {
  p_episodes = Episodes_;
}
std::string Shot::str() const {
  boost::format str_shot{"sc%04i%s"};
  str_shot % p_shot % p_shot_ab;
  return str_shot.str();
}
bool Shot::operator<(const Shot &rhs) const {
  return std::tie(p_shot, p_shot_ab) < std::tie(rhs.p_shot, rhs.p_shot_ab);
}
bool Shot::operator>(const Shot &rhs) const {
  return rhs < *this;
}
bool Shot::operator<=(const Shot &rhs) const {
  return !(rhs < *this);
}
bool Shot::operator>=(const Shot &rhs) const {
  return !(*this < rhs);
}

}  // namespace doodle