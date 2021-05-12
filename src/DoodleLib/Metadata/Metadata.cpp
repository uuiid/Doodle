//
// Created by teXiao on 2021/4/27.
//

#include <DoodleLib/Metadata/Metadata.h>
#include <core/coreset.h>
#include <Exception/Exception.h>
namespace doodle {
Metadata::Metadata()
    : p_parent(),
      p_child_items(),
      p_Root(coreSet::getSet().getUUIDStr()),
      p_Name(coreSet::getSet().getUUIDStr()),
      p_parent_uuid(),
      p_metadata_flctory_ptr_() {
}

Metadata::Metadata(std::weak_ptr<Metadata> in_metadata)
    : p_parent(std::move(in_metadata)),
      p_child_items(),
      p_Root(coreSet::getSet().getUUIDStr()),
      p_Name(coreSet::getSet().getUUIDStr()),
      p_parent_uuid(p_parent.lock()->p_Root),
      p_metadata_flctory_ptr_() {
  SetPParent(p_parent.lock());
}

Metadata::~Metadata() = default;

const std::string &Metadata::GetRoot() const {
  if (p_Root.empty())
    throw DoodleError{"p_Root is empty"};
  return p_Root;
}
std::shared_ptr<Metadata> Metadata::GetPParent() const {
  return p_parent.lock();
}
void Metadata::SetPParent(const std::shared_ptr<Metadata> &in_parent) {
  //先去除掉原先的子
  p_parent.lock()->RemoveChildItems(shared_from_this());
  //再添加
  p_parent      = in_parent;
  p_parent_uuid = in_parent->GetRoot();
  p_parent.lock()->p_child_items.emplace_back(shared_from_this());
}
const std::vector<MetadataPtr> &Metadata::GetPChildItems() const {
  return p_child_items;
}
void Metadata::SetPChildItems(const std::vector<MetadataPtr> &in_child_items) {
  for (auto child : in_child_items) {
    child->SetPParent(shared_from_this());
  }
}

bool Metadata::RemoveChildItems(const MetadataPtr &in_child) {
  auto it = std::find(p_child_items.begin(), p_child_items.end(), in_child);
  if (it != p_child_items.end()) {
    in_child->p_parent.reset();
    in_child->p_parent_uuid.clear();

    p_child_items.erase(it);
    return true;
  } else
    return false;
}

void Metadata::AddChildItem(const MetadataPtr &in_items) {
  in_items->SetPParent(shared_from_this());
}
bool Metadata::HasParent() const {
  return !p_parent.expired();
}
bool Metadata::HasChild() const {
  return !p_child_items.empty();
}
std::string Metadata::ShowStr() const {
  return str();
}
const std::string &Metadata::GetRoot() {
  if (p_Root.empty())
    p_Root = coreSet::getSet().getUUIDStr();
  return p_Root;
}
const std::string &Metadata::GetName() const {
  if (p_Name.empty())
    throw DoodleError{"p_Name is empty"};
  return p_Name;
}
const std::string &Metadata::GetName() {
  if (p_Name.empty())
    p_Name = coreSet::getSet().getUUIDStr();
  return p_Name;
}
bool Metadata::checkParent(const Metadata &in_metadata) const {
  return p_parent_uuid == in_metadata.p_Root;
}

void Metadata::load(const MetadataFactoryPtr &in_factory) {
  p_metadata_flctory_ptr_ = in_factory;
}

void Metadata::save(const MetadataFactoryPtr &in_factory) {
  p_metadata_flctory_ptr_ = in_factory;
}

}  // namespace doodle
