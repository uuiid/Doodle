#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Metadata/ContextMenu.h>
#include <DoodleLib/Metadata/Episodes.h>
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

// Episodes::~Episodes() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

const int64_t& Episodes::getEpisodes() const noexcept {
  return p_episodes;
}

void Episodes::setEpisodes(const int64_t& Episodes_) {
  if (Episodes_ < 0)
    throw DoodleError("集数无法为负");
  p_episodes = Episodes_;
  saved(true);
}

std::string Episodes::str() const {
  boost::format eps_str{"ep%04i"};

  eps_str % p_episodes;
  return eps_str.str();
}

void Episodes::select_indb(const MetadataFactoryPtr& in_factory) {
  if (isLoaded())
    return;
  p_metadata_flctory_ptr_ = in_factory;
  in_factory->select_indb(this);
  loaded();
}

void Episodes::updata_db(const MetadataFactoryPtr& in_factory) {
  p_metadata_flctory_ptr_ = in_factory;

  if (isSaved())
    return;

  if (isInstall())
    in_factory->updata_db(this);
  else
    in_factory->insert_into(this);
  saved();
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
void Episodes::createMenu(ContextMenu* in_contextMenu) {
  in_contextMenu->createMenu(std::dynamic_pointer_cast<Episodes>(shared_from_this()));
}
void Episodes::deleteData(const MetadataFactoryPtr& in_factory) {
  in_factory->deleteData(this);
}
void Episodes::insert_into(const MetadataFactoryPtr& in_factory) {
  in_factory->insert_into(this);
  saved();
}

}  // namespace doodle
