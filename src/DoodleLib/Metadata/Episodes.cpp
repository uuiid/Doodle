#include <DoodleLib/Metadata/Episodes.h>
#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Metadata/MetadataFactory.h>

#include <boost/format.hpp>
namespace doodle {

Episodes::Episodes()
    : Metadata(),
      p_episodes(-1) {
}

Episodes::Episodes(std::weak_ptr<Metadata> in_metadata, int64_t in_episodes)
    : Metadata(std::move(in_metadata)),
      p_episodes(in_episodes) {
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

void Episodes::load(const MetadataFactoryPtr& in_factory) {
  Metadata::load(in_factory);
  in_factory->load(this);
}

void Episodes::save(const MetadataFactoryPtr& in_factory) {
  Metadata::load(in_factory);

  in_factory->save(this);
}

}  // namespace doodle
