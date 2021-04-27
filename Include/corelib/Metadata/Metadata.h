//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <corelib/core_global.h>

namespace doodle {
class CORE_API Metadata{
  std::weak_ptr<Metadata> p_parent;
  std::vector<MetadataPtr> p_child_items;
  public:

  [[nodiscard]] std::shared_ptr<Metadata> GetPParent() const;
  void SetPParent(const std::weak_ptr<Metadata> &p_parent);
  [[nodiscard]] const std::vector<MetadataPtr> &GetPChildItems() const;
  void SetPChildItems(const std::vector<MetadataPtr> &p_child_items);

  virtual std::string str() const = 0;

};
}