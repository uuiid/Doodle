//
// Created by TD on 2021/8/2.
//
#include "tree_container.h"

namespace doodle {
namespace details {
void tree_node_destroy::operator()(tree_node* in_ptr) {
  if (in_ptr->is_linked()) {
    auto k_p = in_ptr->parent;
    k_p->child_item.erase(*in_ptr);
  }
  std::default_delete<tree_node>::operator()(in_ptr);
}
}  // namespace details

tree_node::tree_node()
    : parent(),
      data(),
      child_item() {
}
tree_node::tree_node(tree_node* in_parent, MetadataPtr in_data)
    : parent(in_parent),
      data(std::move(in_data)),
      child_item() {
  if (parent)
    parent->child_item.insert(*this);
}

tree_node::tree_node(const tree_node_ptr& in_parent, MetadataPtr in_data)
    : parent(in_parent.get()),
      data(std::move(in_data)),
      child_item() {
  if (parent)
    parent->child_item.insert(*this);
}
tree_node::~tree_node() {
  child_item.clear();
  data.reset();
}
bool tree_node::is_root() const {
  return parent == nullptr;
}
bool tree_node::has_parent() const {
  return !is_root();
}
tree_node::tree_node_ptr tree_node::get_parent() const {
  return parent->shared_from_this();
}

const tree_node::child_set& tree_node::get_children() const {
  return child_item;
}
void tree_node::insert(const tree_node::tree_node_ptr& in_) {
  insert(*in_);
}
void tree_node::insert(tree_node& in_) {
  if (in_.parent == this)
    return;

  if (in_.has_parent()) {
    in_.parent->remove(in_);
    assert(!in_.is_linked());
  }
  in_.parent = this;
  child_item.insert(in_);
}
tree_node::operator MetadataPtr&() {
  return data;
}
MetadataPtr& tree_node::get() {
  return data;
}
void tree_node::remove(const tree_node_ptr& in_) {
  remove(*in_);
}

void tree_node::remove(tree_node& in_) {
  if (in_ == *this)
    throw std::runtime_error{"无法移除自己"};

  auto it = child_item.find(in_);
  if (it != child_item.end()) {
    in_.parent = nullptr;
    child_item.erase(it);
  }
}
void tree_node::clear() {
  for (auto& it : child_item) {
    it.parent = nullptr;
  }
  child_item.clear();
}
bool tree_node::operator==(const tree_node& in_rhs) const {
  return std::tie(parent, data, child_item) == std::tie(in_rhs.parent, in_rhs.data, in_rhs.child_item);
}
bool tree_node::operator!=(const tree_node& in_rhs) const {
  return !(in_rhs == *this);
}
bool tree_node::operator<(const tree_node& in_rhs) const {
  return std::tie(parent, data, child_item) < std::tie(in_rhs.parent, in_rhs.data, in_rhs.child_item);
}
bool tree_node::operator>(const tree_node& in_rhs) const {
  return in_rhs < *this;
}
bool tree_node::operator<=(const tree_node& in_rhs) const {
  return !(in_rhs < *this);
}
bool tree_node::operator>=(const tree_node& in_rhs) const {
  return !(*this < in_rhs);
}
tree_node::operator const MetadataPtr&() const {
  return data;
}
const MetadataPtr& tree_node::get() const {
  return data;
}

}  // namespace doodle

// namespace doodle
