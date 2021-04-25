#include <corelib/Metadata/Episodes.h>
#include <corelib/Exception/Exception.h>

#include <boost/format.hpp>
namespace doodle {

Episodes::Episodes()
    : p_episodes(-1) {
}

Episodes::Episodes(int64_t in_episodes)
    : p_episodes(in_episodes) {
  if (p_episodes < 0)
    throw DoodleError("集数无法为负");
}

const int64_t& Episodes::Episodes_() const noexcept {
  return p_episodes;
}

void Episodes::setEpisodes_(const int64_t& Episodes_) {
  if (Episodes_ < 0)
    throw DoodleError("集数无法为负");
  p_episodes = Episodes_;
}

std::string Episodes::str() const {
  boost::format eps_str{"ep%04i"};

  eps_str % p_episodes;
  return eps_str.str();
}

}  // namespace doodle