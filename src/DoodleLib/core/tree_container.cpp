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
      child_item(),
      child_owner(),
      sig_class() {
}
tree_node::tree_node(tree_node* in_parent, MetadataPtr in_data)
    : parent(in_parent),
      data(std::move(in_data)),
      child_item(),
      child_owner(),
      sig_class() {
  //  if (parent)
  //    parent->child_item.insert(*this);
}

tree_node::tree_node(const tree_node_ptr& in_parent, MetadataPtr in_data)
    : parent(in_parent.get()),
      data(std::move(in_data)),
      child_item(),
      child_owner(),
      sig_class() {
  //  if (parent)
  //    parent->child_item.insert(*this);
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
tree_node::iterator tree_node::insert(const tree_node::tree_node_ptr& in_) {
  if (in_->parent == this)
    return {};

  if (in_->has_parent()) {
    in_->parent->remove(in_);
    assert(!in_->is_linked());
  }
  return insert_private(in_);
}
tree_node::iterator tree_node::insert_private(const tree_node::tree_node_ptr& in_) {
  in_->parent          = this;
  auto [k_it, k_is_in] = child_item.insert(*in_);
  if (k_is_in) {
    child_owner.insert(in_);
  } else {
    DOODLE_LOG_INFO("插入失败, 已经有这个子元素");
  }
  return k_it;
}
tree_node::operator MetadataPtr&() {
  return data;
}
MetadataPtr& tree_node::get() {
  return data;
}
tree_node::iterator tree_node::remove(const tree_node_ptr& in_) {
  if (*in_ == *this)
    throw std::runtime_error{"无法移除自己"};

  auto it = child_item.find(*in_);
  iterator k_iterator{};
  if (it != child_item.end()) {
    in_->parent = nullptr;
    k_iterator  = child_item.erase(it);
    child_owner.erase(in_);
  } else {
    DOODLE_LOG_INFO("没有找到子元素");
  }
  return k_iterator;
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
bool tree_node::empty() const {
  return child_item.empty();
}
tree_node::iterator tree_node::remove(const MetadataPtr& in_ptr) {
  auto k_it = std::find_if(child_item.begin(), child_item.end(),
                           [in_ptr](const tree_node& in) {
                             return in.data == in_ptr;
                           });

  if (k_it != child_item.end()) {
    return remove(*k_it);
  } else {
    return {};
  }
}
tree_node::iterator tree_node::insert(const MetadataPtr& in_ptr) {
  auto k_ptr = make_this(this, in_ptr);
  return insert(k_ptr);
}
tree_node::iterator tree_node::begin() noexcept {
  return child_item.begin();
}
tree_node::const_iterator tree_node::begin() const noexcept {
  return child_item.begin();
}
tree_node::iterator tree_node::end() noexcept {
  return child_item.end();
}
tree_node::const_iterator tree_node::end() const noexcept {
  return child_item.end();
}
tree_node::reverse_iterator tree_node::rbegin() noexcept {
  return child_item.rbegin();
}
tree_node::const_reverse_iterator tree_node::rbegin() const noexcept {
  return child_item.rbegin();
}
tree_node::reverse_iterator tree_node::rend() noexcept {
  return child_item.rend();
}
tree_node::const_reverse_iterator tree_node::rend() const noexcept {
  return child_item.rend();
}
tree_node::const_reverse_iterator tree_node::crbegin() const noexcept {
  return child_item.crbegin();
}
tree_node::const_reverse_iterator tree_node::crend() const noexcept {
  return child_item.crend();
}

}  // namespace doodle

// namespace doodle
