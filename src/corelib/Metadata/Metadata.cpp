//
// Created by teXiao on 2021/4/27.
//

#include <corelib/Metadata/Metadata.h>

namespace doodle {
std::shared_ptr<Metadata> Metadata::GetPParent() const {
  return p_parent.lock();
}
void Metadata::SetPParent(const std::shared_ptr<Metadata> &in_parent) {
  p_parent = in_parent;
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
}