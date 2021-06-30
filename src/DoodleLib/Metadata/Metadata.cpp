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
      p_need_load(true),
      p_updata_parent_id(false),
      p_has_child(0) {
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
      p_need_load(true),
      p_updata_parent_id(false),
      p_has_child(0) {
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
  p_has_child = p_child_items.size();
  saved(true);
}

bool Metadata::removeChildItems(const MetadataPtr &in_child) {
  auto it = std::find(p_child_items.begin(), p_child_items.end(), in_child);
  if (it != p_child_items.end()) {
    in_child->p_parent.reset();
    in_child->p_parent_id.reset();

    p_child_items.erase(it);
    sig_childDelete(in_child);

    /// 这里基本上是开始设置各个属性值
    p_has_child        = p_child_items.size();
    p_updata_parent_id = true;
    in_child->saved(true);
    saved(true);
    return true;
  } else
    return false;
}
void Metadata::addChildItemNotSig(const MetadataPtr &in_items) {
  MetadataPtr k_old{};
  ///先查看是否有父级关联
  if (in_items->hasParent()) {
    ///有关联就直接将父级的所有权清除
    k_old   = in_items->p_parent.lock();
    auto it = std::find(k_old->p_child_items.begin(), k_old->p_child_items.end(), in_items);
    if (it != k_old->p_child_items.end()) {
      k_old->p_child_items.erase(it);
      k_old->sig_childDelete(shared_from_this());
    }
    k_old->p_has_child = k_old->p_child_items.size();
    DOODLE_LOG_DEBUG(fmt::format("更改子数据的父对象 id: {} --> id: {}", k_old->p_id, p_id))
  }

  /// 这里将所有的子级要继承的父级属性给上
  in_items->p_parent                = weak_from_this();
  in_items->p_parent_id             = p_id;
  in_items->p_metadata_flctory_ptr_ = p_metadata_flctory_ptr_;

  p_child_items.emplace_back(in_items);

  /// 在这里如果父级和原先的父级不一样， 那么我们要同时记录子项要更新父id属性和要保存所有属性
  if (k_old && (k_old.get() != this)) {
    in_items->p_updata_parent_id = true;
    in_items->saved(true);
  }

  p_has_child = p_child_items.size();
  saved(true);
  DOODLE_LOG_INFO(fmt::format("插入子数据： {}", in_items->showStr()))
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
  return p_has_child > 0;
  // if (p_child_items.empty())
  // else
  //   return p_child_items.empty();
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
MetadataConstPtr Metadata::getRootParent() const {
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
  if (!in_need)
    p_updata_parent_id = false;
  p_need_save = in_need;
}
bool Metadata::isLoaded() const {
  return !p_need_load;
}
bool Metadata::isSaved() const {
  return !p_need_save;
}
FSys::path Metadata::getUrlUUID() const {
  auto name = FSys::path{getRootParent()->str()};
  name /= p_uuid.substr(0, 3);
  name /= p_uuid;
  return name;
}
uint64_t Metadata::getId() const {
  return p_id;
}
bool Metadata::operator==(const Metadata &in_rhs) const {
  return std::tie(p_id) == std::tie(in_rhs.p_id);
}
bool Metadata::operator!=(const Metadata &in_rhs) const {
  return !(in_rhs == *this);
}

void Metadata::select_indb(const MetadataFactoryPtr &in_factory) {
  if (in_factory)
    p_metadata_flctory_ptr_ = in_factory;
  if (isLoaded())
    return;

  _select_indb(p_metadata_flctory_ptr_);
  loaded();
}

void Metadata::updata_db(const MetadataFactoryPtr &in_factory) {
  if (in_factory)
    p_metadata_flctory_ptr_ = in_factory;

  if (isSaved())
    return;

  ///在这里测试使用具有父级， 并且如果有父级， 还要更新父id， 那么就可以断定也要更新父级的记录
  if (hasParent() && p_metadata_flctory_ptr_) {
    p_parent.lock()->updata_db(p_metadata_flctory_ptr_);
  }

  _updata_db(in_factory);
  saved();
}

void Metadata::deleteData(const MetadataFactoryPtr &in_factory) {
  if (in_factory)
    p_metadata_flctory_ptr_ = in_factory;

  _deleteData(p_metadata_flctory_ptr_);
}

void Metadata::insert_into(const MetadataFactoryPtr &in_factory) {
  if (in_factory)
    p_metadata_flctory_ptr_ = in_factory;

  _insert_into(p_metadata_flctory_ptr_);
  saved();
}

}  // namespace doodle
