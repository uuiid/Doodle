//
// Created by teXiao on 2021/4/27.
//

#include <DoodleLib/Logger/Logger.h>
#include <DoodleLib/Metadata/Metadata.h>
#include <DoodleLib/Metadata/MetadataFactory.h>
#include <Exception/Exception.h>
#include <core/CoreSet.h>

namespace doodle {
Metadata::Metadata()
    : std::enable_shared_from_this<Metadata>(),
      p_parent(),
      p_child_items(),
      p_id(0),
      p_parent_id(),
      p_uuid(std::move(CoreSet::getSet().getUUIDStr())),
      p_metadata_flctory_ptr_(),
      sig_thisChange(),
      sig_childClear(),
      sig_childAdd(),
      sig_childAddAll(),
      sig_childDelete(),
      p_need_save(true),
      p_need_load(true) {
}

Metadata::Metadata(std::weak_ptr<Metadata> in_metadata)
    : std::enable_shared_from_this<Metadata>(),
      p_parent(std::move(in_metadata)),
      p_child_items(),
      p_id(0),
      p_parent_id(p_parent.lock()->p_id),
      p_uuid(std::move(CoreSet::getSet().getUUIDStr())),
      p_metadata_flctory_ptr_(),
      sig_thisChange(),
      sig_childClear(),
      sig_childAdd(),
      sig_childAddAll(),
      sig_childDelete(),
      p_need_save(true),
      p_need_load(true) {
}

Metadata::~Metadata() = default;

std::shared_ptr<Metadata> Metadata::getParent() const {
  return p_parent.lock();
}
const std::vector<MetadataPtr> &Metadata::getChildItems() const {
  return p_child_items;
}
void Metadata::setChildItems(const std::vector<MetadataPtr> &in_child_items) {
  for (const auto &child : in_child_items) {
    addChildItemNotSig(child);
  }
  sig_childAddAll(in_child_items);
}

bool Metadata::removeChildItems(const MetadataPtr &in_child) {
  auto it = std::find(p_child_items.begin(), p_child_items.end(), in_child);
  if (it != p_child_items.end()) {
    in_child->p_parent.reset();
    in_child->p_parent_id.reset();

    p_child_items.erase(it);
    sig_childDelete(in_child);
    return true;
  } else
    return false;
}
void Metadata::addChildItemNotSig(const MetadataPtr &in_items) {
  MetadataPtr k_old{};
  ///先查看是否有父级关联
  if (in_items->hasParent()) {
    ///有关联就直接将父级的所有权清除
    k_old = in_items->p_parent.lock();
    auto it = std::find(k_old->p_child_items.begin(), k_old->p_child_items.end(), in_items);
    if (it != k_old->p_child_items.end()) {
      k_old->p_child_items.erase(it);
      k_old->sig_childDelete(shared_from_this());
    }
  }

  /// 这里将所有的子级要继承的父级属性给上
  in_items->p_parent = weak_from_this();
  in_items->p_parent_id = p_id;
  in_items->p_metadata_flctory_ptr_ = p_metadata_flctory_ptr_;

  p_child_items.emplace_back(in_items);

  if (k_old && (k_old.get() != this))
    in_items->modifyParent(k_old);
}
MetadataPtr Metadata::addChildItem(const MetadataPtr &in_items) {
  addChildItemNotSig(in_items);
  sig_childAdd(in_items);
  return in_items;
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
  if (p_child_items.empty()) {
    if (p_metadata_flctory_ptr_)
      k_is = p_metadata_flctory_ptr_->hasChild(this);
  } else
    k_is = true;
  return k_is;
}
std::string Metadata::showStr() const {
  return str();
}
const std::string &Metadata::getUUID() {
  return p_uuid;
}

const MetadataFactoryPtr &Metadata::getMetadataFactory() const {
  return p_metadata_flctory_ptr_;
}
bool Metadata::checkParent(const Metadata &in_metadata) const {
  return p_parent_id == in_metadata.p_id;
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
  while (!k_p->p_parent.expired()) {
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
FSys::path Metadata::getUrlUUID() {
  auto name = FSys::path{getRootParent()->str()};
  name /= p_uuid.substr(0, 3);
  name /= p_uuid;
  return name;
}

}  // namespace doodle
