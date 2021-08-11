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
      sig_class(),
      p_sig(std::make_shared<signal_observe>()) {
}
tree_node::tree_node(tree_node* in_parent, MetadataPtr in_data)
    : parent(in_parent),
      data(std::move(in_data)),
      child_item(),
      child_owner(),
      sig_class(),
      p_sig(std::make_shared<signal_observe>()) {
  ;
}

tree_node::tree_node(const tree_node_ptr& in_parent, MetadataPtr in_data)
    : parent(in_parent.get()),
      data(std::move(in_data)),
      child_item(),
      child_owner(),
      sig_class(),
      p_sig(std::make_shared<signal_observe>()) {
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
  return insert(in_, false);
}
tree_node::iterator tree_node::insert_private(const tree_node::tree_node_ptr& in_) {
  in_->parent          = this;
  auto [k_it, k_is_in] = child_item.insert(*in_);
  if (k_is_in) {
    /// 所有权转移插入
    child_owner.push_back(in_);
  } else {
    DOODLE_LOG_INFO("插入失败, 已经有这个子元素");
  }
  return k_it;
}

tree_node::iterator tree_node::insert(const tree_node_ptr& in_, bool emit_solt) {
  if (in_->parent == this)
    return {};

  if (in_->has_parent()) {
    if (emit_solt)
      in_->parent->erase_sig(in_);
    else
      in_->parent->erase(in_);
    assert(!in_->is_linked());
  }
  return insert_private(in_);
}
tree_node::operator MetadataPtr&() {
  return data;
}
MetadataPtr& tree_node::get() {
  return data;
}

void tree_node::clear() {
  for (auto& it : child_item) {
    it.parent = nullptr;
  }
  child_item.clear();
}

void tree_node::clear_sig() {
  p_sig->sig_begin_clear();
  clear();
  p_sig->sig_clear();
}

tree_node::signal_observe_ptr tree_node::get_signal_observe() const {
  return p_sig;
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

void tree_node::set(const MetadataPtr& in_) {
  value_fun_t::disconnect(data, shared_from_this());
  value_fun_t::set_node_ptr(data, tree_node_ptr{});
  data = in_;
  value_fun_t::connect(data, shared_from_this());
  value_fun_t::set_node_ptr(data, shared_from_this());
}

void tree_node::set(MetadataPtr&& in_) {
  value_fun_t::disconnect(data, shared_from_this());
  value_fun_t::set_node_ptr(data, tree_node_ptr{});
  data = std::move(in_);
  value_fun_t::connect(data, shared_from_this());
  value_fun_t::set_node_ptr(data, shared_from_this());
}

tree_node& tree_node::operator=(const MetadataPtr& in_) {
  set(in_);
}

tree_node& tree_node::operator=(MetadataPtr&& in_) {
  set(std::move(in_));
}
bool tree_node::empty() const {
  return child_item.empty();
}
tree_node::iterator tree_node::erase(const MetadataPtr& in_ptr) {
  auto k_it = std::find_if(child_item.begin(), child_item.end(),
                           [in_ptr](const tree_node& in) {
                             return in.data == in_ptr;
                           });

  if (k_it != child_item.end()) {
    return erase(*k_it);
  } else {
    return {};
  }
}

tree_node::iterator tree_node::erase(const tree_node_ptr& in_) {
  if (*in_ == *this)
    throw std::runtime_error{"无法移除自己"};

  auto it = child_item.find(*in_);
  iterator k_iterator{};
  if (it != child_item.end()) {
    in_->parent = nullptr;
    k_iterator  = child_item.erase(it);
    auto k_it   = std::find(child_owner.begin(), child_owner.end(), in_);
    child_owner.erase(k_it);
  } else {
    DOODLE_LOG_INFO("没有找到子元素");
  }
  return k_iterator;
}

tree_node::iterator tree_node::erase_sig(const tree_node_ptr& in_) {
  p_sig->sig_begin_erase(*in_);
  auto k_i = erase(in_);
  p_sig->sig_erase(*k_i);
  return k_i;
}

tree_node::iterator tree_node::erase_sig(const MetadataPtr& in_ptr) {
  auto k_it = std::find_if(child_item.begin(), child_item.end(),
                           [in_ptr](const tree_node& in) {
                             return in.data == in_ptr;
                           });

  if (k_it != child_item.end()) {
    p_sig->sig_begin_erase(*k_it);
    auto k_i = erase(*k_it);
    p_sig->sig_erase(*k_i);
    return k_i;
  } else {
    return {};
  }
}
tree_node::iterator tree_node::insert(const MetadataPtr& in_ptr) {
  auto k_ptr = make_this(this, in_ptr);
  return insert(k_ptr, false);
}

tree_node::iterator tree_node::insert_sig(const tree_node_ptr& in_) {
  p_sig->sig_begin_insert(*in_);
  auto k_i = insert(in_, true);
  p_sig->sig_insert(*k_i);
  return k_i;
}

tree_node::iterator tree_node::insert_sig(const MetadataPtr& in_ptr) {
  auto k_ptr = make_this(this, in_ptr);
  p_sig->sig_begin_insert(*k_ptr);
  auto k_i = insert(k_ptr, true);
  p_sig->sig_insert(*k_i);
  return k_i;
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
