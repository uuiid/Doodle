//
// Created by teXiao on 2021/4/27.
//

#include <DoodleLib/Metadata/Metadata.h>
#include <core/coreset.h>
#include <Exception/Exception.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <DoodleLib/Logger/Logger.h>

namespace doodle {
Metadata::Metadata()
    : std::enable_shared_from_this<Metadata>(),
      p_parent(),
      p_child_items(),
      p_Root(coreSet::getSet().getUUIDStr()),
      p_Name(coreSet::getSet().getUUIDStr()),
      p_parent_uuid(),
      p_metadata_flctory_ptr_(),
      sig_thisChange(),
      sig_childClear(),
      sig_childAdd(),
      sig_childAddAll(),
      sig_childDelete(),
      p_need_save(true),
      p_need_load(false){
}

Metadata::Metadata(std::weak_ptr<Metadata> in_metadata)
    : std::enable_shared_from_this<Metadata>(),
      p_parent(std::move(in_metadata)),
      p_child_items(),
      p_Root(coreSet::getSet().getUUIDStr()),
      p_Name(coreSet::getSet().getUUIDStr()),
      p_parent_uuid(p_parent.lock()->p_Root),
      p_metadata_flctory_ptr_(),
      sig_thisChange(),
      sig_childClear(),
      sig_childAdd(),
      sig_childAddAll(),
      sig_childDelete(),
      p_need_save(true),
      p_need_load(false){

}

Metadata::~Metadata() = default;

const std::string &Metadata::getRoot() const {
  if (p_Root.empty())
    throw DoodleError{"p_Root is empty"};
  return p_Root;
}
std::shared_ptr<Metadata> Metadata::getParent() const {
  return p_parent.lock();
}
void Metadata::setParent(const std::shared_ptr<Metadata> &in_parent) {
  // if(*in_parent == *this)
  //先去除掉原先的子
  std::shared_ptr<Metadata> k_old{};
  if (hasParent()) {
    k_old = p_parent.lock();
    auto it = std::find(k_old->p_child_items.begin(), k_old->p_child_items.end(), shared_from_this());
    //    assert(it != p_p->p_child_items.end());
    if (it != k_old->p_child_items.end())
      p_parent.lock()->p_child_items.erase(it);
  }
  //再添加
  DOODLE_LOG_INFO( "begin set parent: "<<in_parent->str())
  p_parent      = in_parent;
  p_parent_uuid = in_parent->getRoot();
  p_metadata_flctory_ptr_ = in_parent->p_metadata_flctory_ptr_;
  in_parent->p_child_items.emplace_back(shared_from_this());
  if (k_old)
    modifyParent(k_old);
  in_parent->sig_childAdd(in_parent,shared_from_this());
  DOODLE_LOG_INFO(in_parent->str())
}
const std::vector<MetadataPtr> &Metadata::getChildItems() const {
  return p_child_items;
}
void Metadata::setChildItems(const std::vector<MetadataPtr> &in_child_items) {
  for (const auto& child : in_child_items) {
    child->setParent(shared_from_this());
  }
  sig_childAddAll(shared_from_this(),in_child_items);
}

bool Metadata::removeChildItems(const MetadataPtr &in_child) {
  auto it = std::find(p_child_items.begin(), p_child_items.end(), in_child);
  if (it != p_child_items.end()) {
    in_child->p_parent.reset();
    in_child->p_parent_uuid.clear();

    p_child_items.erase(it);
    sig_childDelete(shared_from_this(),in_child);
    return true;
  } else
    return false;
}

void Metadata::addChildItem(const MetadataPtr &in_items) {
  in_items->setParent(shared_from_this());
  sig_childAdd(shared_from_this(),in_items);
}

void Metadata::sortChildItems() {
  std::sort(p_child_items.begin(), p_child_items.end(),
            [](const MetadataPtr &r, const MetadataPtr &l) {
              return *r < *l;
            });
}

bool Metadata::hasParent() const {
  return !p_parent.expired();
}
bool Metadata::hasChild() const {
  auto k_is = false;
  if(p_child_items.empty()) {
    if (p_metadata_flctory_ptr_)
      k_is = p_metadata_flctory_ptr_->hasChild(this);
  } else
    k_is = true;
  return k_is;
}
std::string Metadata::showStr() const {
  return str();
}
const std::string &Metadata::getRoot() {
  if (p_Root.empty())
    p_Root = coreSet::getSet().getUUIDStr();
  return p_Root;
}
const std::string &Metadata::getName() const {
  if (p_Name.empty())
    throw DoodleError{"p_Name is empty"};
  return p_Name;
}
const std::string &Metadata::getName() {
  if (p_Name.empty())
    p_Name = coreSet::getSet().getUUIDStr();
  return p_Name;
}

const MetadataFactoryPtr &Metadata::getMetadataFactory() const {
  return p_metadata_flctory_ptr_;
}
bool Metadata::checkParent(const Metadata &in_metadata) const {
  return p_parent_uuid == in_metadata.p_Root;
}

bool Metadata::operator<(const Metadata &in_rhs) const {
  return this->sort(in_rhs);
}
bool Metadata::operator>(const Metadata &in_rhs) const {
  return in_rhs < *this;
}
bool Metadata::operator<=(const Metadata &in_rhs) const {
  return !(in_rhs < *this);
}
bool Metadata::operator>=(const Metadata &in_rhs) const {
  return !(*this < in_rhs);
}
void Metadata::clearChildItems() {
  p_child_items.clear();
}
MetadataPtr Metadata::getRootParent() {
  auto k_p = shared_from_this();
  while (!k_p->p_parent.expired()){
    k_p = k_p->p_parent.lock()->getRootParent();
  }
  return k_p;
//  if(p_parent.expired())
//    return shared_from_this();
//  else
//    return p_parent.lock()->getRootParent();
}
void Metadata::loaded(bool in_need) {
  p_need_load = in_need;
}
void Metadata::saved(bool in_need) {
  p_need_save = in_need;
}
bool Metadata::isLoaded() const {
  return !p_need_load;
}
bool Metadata::isSaved() const {
  return !p_need_save;
}

}  // namespace doodle
