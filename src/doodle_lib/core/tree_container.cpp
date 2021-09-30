//
// Created by TD on 2021/8/2.
//
#include "tree_container.h"

namespace doodle {
namespace details {
//void tree_node_destroy::operator()(tree_node* in_ptr) {
//  if (in_ptr->is_linked()) {
//    auto k_p = in_ptr->parent;
//    k_p->child_item.erase(*in_ptr);
//  }
//  std::default_delete<tree_node>::operator()(in_ptr);
//}
}  // namespace details

tree_node::tree_node()
    : parent(),
      data(),
      child_owner(),
      sig_class(),
      p_sig(new_object<signal_observe>()) {
}
tree_node::tree_node(tree_node* in_parent, metadata_ptr in_data)
    : parent(in_parent),
      data(std::move(in_data)),
      child_owner(),
      sig_class(),
      p_sig(new_object<signal_observe>()) {
  ;
}

tree_node::tree_node(const tree_node_ptr& in_parent, metadata_ptr in_data)
    : parent(in_parent.get()),
      data(std::move(in_data)),
      child_owner(),
      sig_class(),
      p_sig(new_object<signal_observe>()) {
}
tree_node::~tree_node() {
  child_owner.clear();
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

const tree_node::child_set_owner & tree_node::get_children() const {
  return child_owner;
}
tree_node::iterator tree_node::insert(const tree_node::tree_node_ptr& in_) {
  return insert(in_, false);
}
tree_node::iterator tree_node::insert_private(const tree_node::tree_node_ptr& in_) {
  in_->parent          = this;
    /// 所有权转移插入
  child_owner.push_back(in_);
  return  std::find(child_owner.begin(),  child_owner.end(),in_);
}

tree_node::iterator tree_node::insert(const tree_node_ptr& in_, bool emit_solt) {
  if (in_->parent == this)
    return {};

  if (in_->has_parent()) {
    if (emit_solt)
      in_->parent->erase_sig(in_);
    else
      in_->parent->erase(in_);
  }
  return insert_private(in_);
}
tree_node::operator metadata_ptr&() {
  return data;
}
metadata_ptr& tree_node::get() {
  return data;
}

void tree_node::clear() {
  for (auto& it : child_owner) {
    it->parent = nullptr;
  }
  child_owner.clear();
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
  return std::tie(parent, data, child_owner) == std::tie(in_rhs.parent, in_rhs.data, in_rhs.child_owner);
}
bool tree_node::operator!=(const tree_node& in_rhs) const {
  return !(in_rhs == *this);
}
bool tree_node::operator<(const tree_node& in_rhs) const {
  return std::tie(parent, data, child_owner) < std::tie(in_rhs.parent, in_rhs.data, in_rhs.child_owner);
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
tree_node::operator const metadata_ptr&() const {
  return data;
}
const metadata_ptr& tree_node::get() const {
  return data;
}

void tree_node::set(const metadata_ptr& in_) {
  value_fun_t::disconnect(data, shared_from_this());
  value_fun_t::set_node_ptr(data, tree_node_ptr{});
  data = in_;
  value_fun_t::connect(data, shared_from_this());
  value_fun_t::set_node_ptr(data, shared_from_this());
}

void tree_node::set(metadata_ptr&& in_) {
  value_fun_t::disconnect(data, shared_from_this());
  value_fun_t::set_node_ptr(data, tree_node_ptr{});
  data = std::move(in_);
  value_fun_t::connect(data, shared_from_this());
  value_fun_t::set_node_ptr(data, shared_from_this());
}

tree_node& tree_node::operator=(const metadata_ptr& in_) {
  set(in_);
  return *this;
}

tree_node& tree_node::operator=(metadata_ptr&& in_) {
  set(std::move(in_));
  return *this;
}
bool tree_node::empty() const {
  return child_owner.empty();
}
tree_node::iterator tree_node::erase(const metadata_ptr& in_ptr) {
  auto k_it = std::find_if(child_owner.begin(), child_owner.end(),
                           [in_ptr](const tree_node_ptr& in) {
                             return in->data == in_ptr;
                           });

  if (k_it != child_owner.end()) {
    return erase(*k_it);
  } else {
    return {};
  }
}

tree_node::iterator tree_node::erase(const tree_node_ptr& in_) {
  if (*in_ == *this)
    throw std::runtime_error{"无法移除自己"};

  auto it = std::find(child_owner.begin(),  child_owner.end(),in_);
  iterator k_iterator{};
  if (it != child_owner.end()) {
    in_->parent = nullptr;
    k_iterator  = child_owner.erase(it);
  } else {
    DOODLE_LOG_INFO("没有找到子元素");
  }
  return k_iterator;
}

tree_node::iterator tree_node::erase_sig(const tree_node_ptr& in_) {
  p_sig->sig_begin_erase(in_);
  auto k_i = erase(in_);
  p_sig->sig_erase(*k_i);
  return k_i;
}

tree_node::iterator tree_node::erase_sig(const metadata_ptr& in_ptr) {
  auto k_it = std::find_if(child_owner.begin(), child_owner.end(),
                           [in_ptr](const tree_node_ptr& in) {
                             return in->data == in_ptr;
                           });

  if (k_it != child_owner.end()) {
    p_sig->sig_begin_erase(*k_it);
    auto k_i = erase(*k_it);
    p_sig->sig_erase(*k_i);
    return k_i;
  } else {
    return {};
  }
}
tree_node::iterator tree_node::insert(const metadata_ptr& in_ptr) {
  auto k_ptr = new_object<tree_node>(this, in_ptr);
  return insert(k_ptr, false);
}

tree_node::iterator tree_node::insert_sig(const tree_node_ptr& in_) {
  p_sig->sig_begin_insert(in_);
  auto k_i = insert(in_, true);
  p_sig->sig_insert(*k_i);
  return k_i;
}

tree_node::iterator tree_node::insert_sig(const metadata_ptr& in_ptr) {
  auto k_ptr = new_object<tree_node>(this, in_ptr);
  p_sig->sig_begin_insert(k_ptr);
  auto k_i = insert(k_ptr, true);
  p_sig->sig_insert(*k_i);
  return k_i;
}

tree_node::iterator tree_node::begin() noexcept {
  return child_owner.begin();
}
tree_node::const_iterator tree_node::begin() const noexcept {
  return child_owner.begin();
}
tree_node::iterator tree_node::end() noexcept {
  return child_owner.end();
}
tree_node::const_iterator tree_node::end() const noexcept {
  return child_owner.end();
}
tree_node::reverse_iterator tree_node::rbegin() noexcept {
  return child_owner.rbegin();
}
tree_node::const_reverse_iterator tree_node::rbegin() const noexcept {
  return child_owner.rbegin();
}
tree_node::reverse_iterator tree_node::rend() noexcept {
  return child_owner.rend();
}
tree_node::const_reverse_iterator tree_node::rend() const noexcept {
  return child_owner.rend();
}
tree_node::const_reverse_iterator tree_node::crbegin() const noexcept {
  return child_owner.crbegin();
}
tree_node::const_reverse_iterator tree_node::crend() const noexcept {
  return child_owner.crend();
}
void tree_node::post_constructor() {
  if (this->has_parent())
    this->parent->insert_private(shared_from_this());

  value_fun_t::connect(this->data, shared_from_this());
  value_fun_t::set_node_ptr(this->data, shared_from_this());
}

}  // namespace doodle

// namespace doodle
