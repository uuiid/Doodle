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

void Episodes::SetPParent(const std::shared_ptr<Metadata>& in_parent) {
  auto old_p = p_parent;
  Metadata::SetPParent(in_parent);
  //在这里， 如果已经保存过或者已经是从磁盘中加载来时， 必然会持有工厂， 这个时候我们就要告诉工厂， 我们改变了父子关系
  if (p_metadata_flctory_ptr_)
    p_metadata_flctory_ptr_->modifyParent(this, old_p.lock().get());
}

void Episodes::load(const MetadataFactoryPtr& in_factory) {
  in_factory->load(this);
  Metadata::load(in_factory);
}

void Episodes::save(const MetadataFactoryPtr& in_factory) {
  Metadata::load(in_factory);

  in_factory->save(this);
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

bool Episodes::sort(const Metadata& in_rhs) const {
  if (typeid(in_rhs) == typeid(*this)) {
    return *this < (dynamic_cast<const Episodes&>(in_rhs));
  } else {
    return str() < in_rhs.str();
  }
}

}  // namespace doodle
