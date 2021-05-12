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

void Assets::SetPParent(const std::shared_ptr<Metadata>& in_parent) {
  auto old_p = p_parent;
  Metadata::SetPParent(in_parent);
  //在这里， 如果已经保存过或者已经是从磁盘中加载来时， 必然会持有工厂， 这个时候我们就要告诉工厂， 我们改变了父子关系
  if (p_metadata_flctory_ptr_)
    p_metadata_flctory_ptr_->modifyParent(this, old_p.lock().get());
}

void Assets::load(const MetadataFactoryPtr& in_factory) {
  in_factory->load(this);
  Metadata::load(in_factory);
}

void Assets::save(const MetadataFactoryPtr& in_factory) {
  Metadata::load(in_factory);
  in_factory->save(this);
}

}  // namespace doodle
