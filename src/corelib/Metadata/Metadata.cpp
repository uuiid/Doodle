//
// Created by teXiao on 2021/4/27.
//

#include <corelib/Metadata/Metadata.h>

namespace doodle {
std::shared_ptr<Metadata> Metadata::GetPParent() const {
  return p_parent;
}
void Metadata::SetPParent(const std::weak_ptr<Metadata> &p_parent) {
  p_parent = p_parent;
}
const std::vector<MetadataPtr> &Metadata::GetPChildItems() const {
  return p_child_items;
}
void Metadata::SetPChildItems(const std::vector<MetadataPtr> &p_child_items) {
  p_child_items = p_child_items;
}
}