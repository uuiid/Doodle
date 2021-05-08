//
// Created by teXiao on 2021/4/27.
//

#include <corelib/Metadata/Metadata.h>
#include <corelib/core/coreset.h>
#include <Exception/Exception.h>
namespace doodle {
std::shared_ptr<Metadata> Metadata::GetPParent() const {
  return p_parent.lock();
}
void Metadata::SetPParent(const std::shared_ptr<Metadata> &in_parent) {
  p_parent = in_parent;
  p_parent_uuid = in_parent->GetRoot();
}
const std::vector<MetadataPtr> &Metadata::GetPChildItems() const {
  return p_child_items;
}
void Metadata::SetPChildItems(const std::vector<MetadataPtr> &in_child_items) {
  p_child_items = in_child_items;
}
void Metadata::AddChildItem(const MetadataPtr &in_items) {
  p_child_items.emplace_back(in_items);
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
Metadata::Metadata()
    : p_parent(),
      p_child_items(),
      p_Root(coreSet::getSet().getUUIDStr()),
      p_Name(coreSet::getSet().getUUIDStr()){
}
Metadata::~Metadata() = default;
const std::string &Metadata::GetRoot() const {
  if(p_Root.empty())
    throw DoodleError{"p_Root is empty"};
  return p_Root;
}
const std::string &Metadata::GetRoot()  {
  if(p_Root.empty())
    p_Root = coreSet::getSet().getUUIDStr();
  return p_Root;
}
const std::string &Metadata::GetName() const {
  if(p_Name.empty())
    throw DoodleError{"p_Name is empty"};
  return p_Name;
}
const std::string &Metadata::GetName()  {
  if(p_Name.empty())
    p_Name = coreSet::getSet().getUUIDStr();
  return p_Name;
}

}  // namespace doodle
