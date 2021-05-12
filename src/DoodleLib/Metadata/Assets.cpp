//
// Created by teXiao on 2021/4/27.
//

#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/PinYin/convert.h>
#include <DoodleLib/Metadata/MetadataFactory.h>

namespace doodle {
Assets::Assets()
    : Metadata(),
      p_name() {
}

Assets::Assets(std::weak_ptr<Metadata> in_metadata, std::string in_name)
    : Metadata(std::move(in_metadata)),
      p_name(std::move(in_name)) {}

std::string Assets::str() const {
  return convert::Get().toEn(this->p_name);
}
std::string Assets::ShowStr() const {
  return p_name;
}

void Assets::load(const MetadataFactoryPtr& in_factory) {
  Metadata::load(in_factory);
  in_factory->load(this);
}

void Assets::save(const MetadataFactoryPtr& in_factory) {
  Metadata::load(in_factory);
  in_factory->save(this);
}

}  // namespace doodle
