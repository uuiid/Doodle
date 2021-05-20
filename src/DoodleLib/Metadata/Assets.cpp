//
// Created by teXiao on 2021/4/27.
//

#include <DoodleLib/Metadata/Assets.h>
#include <DoodleLib/PinYin/convert.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/Metadata/ContextMenu.h>

namespace doodle {
Assets::Assets()
    : Metadata(),
      p_name() {
}

Assets::Assets(std::weak_ptr<Metadata> in_metadata, std::string in_name)
    : Metadata(std::move(in_metadata)),
      p_name(std::move(in_name)) {

}

std::string Assets::str() const {
  return convert::Get().toEn(this->p_name);
}
std::string Assets::showStr() const {
  return p_name;
}

void Assets::load(const MetadataFactoryPtr& in_factory) {
  if(isLoaded())
    return;
  p_metadata_flctory_ptr_ = in_factory;
  in_factory->load(this);
}

void Assets::save(const MetadataFactoryPtr& in_factory) {
  if(isSaved())
    return;
  p_metadata_flctory_ptr_ = in_factory;
  in_factory->save(this);
}
bool Assets::operator<(const Assets& in_rhs) const {
  //  return std::tie(static_cast<const doodle::Metadata&>(*this), p_name) < std::tie(static_cast<const doodle::Metadata&>(in_rhs), in_rhs.p_name);
  return std::tie(p_name) < std::tie(in_rhs.p_name);
}
bool Assets::operator>(const Assets& in_rhs) const {
  return in_rhs < *this;
}
bool Assets::operator<=(const Assets& in_rhs) const {
  return !(in_rhs < *this);
}
bool Assets::operator>=(const Assets& in_rhs) const {
  return !(*this < in_rhs);
}

bool Assets::sort(const Metadata& in_rhs) const {
  if (typeid(in_rhs) == typeid(*this)) {
    return *this < (dynamic_cast<const Assets&>(in_rhs));
  } else {
    return str() < in_rhs.str();
  }
}
void Assets::modifyParent(const std::shared_ptr<Metadata>& in_old_parent) {
  ///在这里， 如果已经保存过或者已经是从磁盘中加载来时， 必然会持有工厂， 这个时候我们就要告诉工厂， 我们改变了父子关系
  if (p_metadata_flctory_ptr_)
    p_metadata_flctory_ptr_->modifyParent(this, in_old_parent.get());
}
void Assets::createMenu(ContextMenu* in_contextMenu) {
  in_contextMenu->createMenu(std::dynamic_pointer_cast<Assets>(shared_from_this()));
}
void Assets::deleteData(const MetadataFactoryPtr& in_factory) {
  in_factory->deleteData(this);
}
const std::string& Assets::getName1() const {
  return p_name;
}
void Assets::setName1(const std::string& in_name) {
  p_name = in_name;
  save();
}
const std::string& Assets::getNameEnus() const {
  return p_name_enus;
}
void Assets::setNameEnus(const std::string& in_nameEnus) {
  p_name_enus = in_nameEnus;
  save();
}
void Assets::save() const {
  if(p_metadata_flctory_ptr_)
    p_metadata_flctory_ptr_->save(this);
}

}  // namespace doodle
