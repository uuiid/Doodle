//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/Metadata/AssetsFile.h>
#include <PinYin/convert.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <utility>

namespace doodle {

AssetsFile::AssetsFile()
    : Metadata() {
}
AssetsFile::AssetsFile(std::weak_ptr<Metadata> in_metadata, std::string name, std::string showName)
    : Metadata(),
      p_name(std::move(name)),
      p_ShowName(std::move(showName)) {
  p_parent = std::move(in_metadata);
  if (p_ShowName.empty())
    p_ShowName = convert::Get().toEn(p_name);
}

std::string AssetsFile::str() const {
  return p_name;
}
std::string AssetsFile::ShowStr() const {
  return p_ShowName;
}

void AssetsFile::SetPParent(const std::shared_ptr<Metadata>& in_parent) {
  auto old_p = p_parent;
  Metadata::SetPParent(in_parent);
  //在这里， 如果已经保存过或者已经是从磁盘中加载来时， 必然会持有工厂， 这个时候我们就要告诉工厂， 我们改变了父子关系
  if (p_metadata_flctory_ptr_)
    p_metadata_flctory_ptr_->modifyParent(this, old_p.lock().get());
}

void AssetsFile::load(const MetadataFactoryPtr& in_factory) {
  Metadata::load(in_factory);
  in_factory->load(this);
}

void AssetsFile::save(const MetadataFactoryPtr& in_factory) {
  Metadata::load(in_factory);
  in_factory->save(this);
}
}  // namespace doodle
