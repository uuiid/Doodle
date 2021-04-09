#include <corelib/Metadata/Shot.h>
#include <corelib/Exception/Exception.h>

#include <boost/format.hpp>
namespace doodle {

Shot::Shot()
    : p_shot(-1),
      p_shot_ab() {
}

Shot::Shot(int64_t in_shot, std::string in_shot_ab)
    : p_shot(in_shot),
      p_shot_ab(in_shot_ab) {
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

std::string Shot::str() const {
  boost::format str_shot{"sc%04i%s"};
  str_shot % p_shot % p_shot_ab;
  return str_shot.str();
}

}  // namespace doodle