//
// Created by TD on 2021/7/30.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class tree_meta;
class tree_meta {
 public:
  using tree_meta_ptr      = std::shared_ptr<tree_meta>;
  using tree_meta_weak_ptr = std::weak_ptr<tree_meta>;

 private:
  tree_meta_weak_ptr parent;
  std::set<tree_meta_ptr> child_item;
  MetadataPtr data;

 public:
  tree_meta() = default;
  explicit tree_meta(tree_meta_ptr& in_meta, MetadataPtr in_metadata);
//  bool has_parent();
//  bool is_root();
};

}  // namespace doodle
