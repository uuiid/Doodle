//
// Created by teXiao on 2021/4/27.
//

#pragma once
#include <corelib/core_global.h>

namespace doodle {
class CORE_API Metadata {
 protected:
  std::weak_ptr<Metadata> p_parent;
  std::vector<MetadataPtr> p_child_items;

 public:
  Metadata();
  virtual ~Metadata();
  [[nodiscard]] bool HasParent() const;
  [[nodiscard]] std::shared_ptr<Metadata> GetPParent() const;
  void SetPParent(const std::shared_ptr<Metadata> &in_parent);

  [[nodiscard]] bool HasChild() const;
  [[nodiscard]] const std::vector<MetadataPtr> &GetPChildItems() const;
  void SetPChildItems(const std::vector<MetadataPtr> &in_child_items);
  void AddChildItem(const MetadataPtr &in_items);

  [[nodiscard]] virtual std::string str() const = 0;
  [[nodiscard]] virtual std::string ShowStr() const;
};
}  // namespace doodle
