//
// Created by TD on 2021/8/2.
//
#include "tree_container.h"
namespace doodle {

tree_meta::tree_meta(tree_meta_ptr& in_meta, MetadataPtr in_metadata)
    : parent(in_meta),
      child_item(),
      data(std::move(in_metadata)) {
}

}  // namespace doodle
