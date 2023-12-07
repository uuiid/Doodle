//
// Created by teXiao on 2021/4/27.
//

#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>

#include <boost/algorithm/string.hpp>

#include "core/core_help_impl.h"
#include "core/init_register.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include <treehh/tree.hh>

namespace doodle {

std::string assets::str() const { return p_path; }
void assets::set_path(const std::string& in_path) { p_path = in_path; }
const std::string& assets::get_path() const { return p_path; }

void assets::add_child(const entt::handle& in_child) {
  if (!in_child.any_of<assets>()) return;

  auto l_this_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_org_parent  = in_child.get<assets>().parent_;
  if (l_org_parent == l_this_handle) return;

  child_.insert(in_child);
  in_child.patch<assets>().parent_ = l_this_handle;
  if (l_org_parent && l_org_parent.any_of<assets>()) l_org_parent.patch<assets>().child_.erase(in_child);
}

bool assets::operator<(const assets& in_rhs) const { return std::tie(p_path) < std::tie(in_rhs.p_path); }
bool assets::operator!=(const assets& in_rhs) const { return in_rhs.p_path != p_path; }

namespace {

struct tree_node : boost::less_than_comparable<tree_node> {
 public:
  entt::handle handle{};
  std::string name{};
  tree_node() = default;

  explicit tree_node(std::string in_name, const entt::handle& in_handle)
      : name(std::move(in_name)), handle(in_handle) {}
  bool operator<(const tree_node& rhs) const { return handle < rhs.handle; };
};

using tree_type_t = tree<tree_node>;

void build_tree_(tree_type_t& in_tree_, const entt::handle& in_handle, const tree_type_t::iterator& in_iterator) {
  auto& l_ass = in_handle.get<assets>();
  auto l_it   = in_tree_.append_child(in_iterator, tree_node{l_ass.get_path(), in_handle});

  for (auto&& l_c : l_ass.get_child()) {
    build_tree_(in_tree_, l_c, l_it);
  }
}

tree_type_t build_tree(const registry_ptr& in_registry_ptr) {
  auto l_tree_    = tree_type_t{tree_node{"root", entt::handle{*in_registry_ptr, entt::null}}};
  auto l_ass_view = in_registry_ptr->view<assets>();
  for (auto&& [e, ass] : l_ass_view.each()) {
    if (!ass.get_parent()) {
      auto l_h = entt::handle{*in_registry_ptr, e};
      //      auto l_it = tree_.insert(tree_.begin(), assets_tree_node{ass.p_path, l_h});
      build_tree_(l_tree_, l_h, l_tree_.begin());
    }
  }
  return l_tree_;
}

// 先祖代合并, 再子代合并, 最后删除
void clear(const tree_type_t::iterator& in_node, const std::map<entt::handle, entt::handle>& in_ass_map_ass_file) {
  std::map<std::string, std::vector<entt::handle>> l_name{};
  for (auto it = tree_type_t::begin(in_node); it != tree_type_t::end(in_node); ++it) {
    auto l_key = std::string{it->name};
    if (l_name.contains(l_key)) {
      l_name[l_key].emplace_back(it->handle);
    } else {
      l_name.emplace(l_key, std::vector<entt::handle>{it->handle});
    }
  }
  std::vector<entt::handle> l_delete_handles{};
  bool has_dou{};
  for (auto&& i : l_name) {
    if (i.second.size() > 1) {
      has_dou      = true;
      auto l_merge = i.second[0];
      auto& l_ass  = l_merge.patch<assets>();
      for (auto l_it = ++std::begin(i.second); l_it != std::end(i.second); ++l_it) {
        for (auto&& l_c : l_it->get<assets>().get_child()) {
          l_ass.add_child(l_c);
        }
        if (in_ass_map_ass_file.contains(*l_it)) in_ass_map_ass_file.at(*l_it).patch<assets_file>().assets_attr(*l_it);
        l_delete_handles.emplace_back(*l_it);
      }
    }
  }

  for (auto it = tree_type_t::begin(in_node); it != tree_type_t::end(in_node); ++it) {
    clear(in_node, in_ass_map_ass_file);
  }
  for (auto l_h : l_delete_handles) {
    l_h.destroy();
  }
}
}  // namespace

void assets::merge_assets_tree(const registry_ptr& in_registry_ptr) {
  auto l_tree          = build_tree(in_registry_ptr);

  auto l_ass_file_view = in_registry_ptr->view<assets_file>().each();
  auto l_ass_map_ass_file =
      l_ass_file_view |
      ranges::views::transform(
          [&](const decltype(*l_ass_file_view.begin())& in_tup) -> std::pair<entt::handle, entt::handle> {
            return {
                std::get<assets_file&>(in_tup).assets_attr(),
                entt::handle{*in_registry_ptr, std::get<entt::entity>(in_tup)}
            };
          }
      ) |
      ranges::to<std::map<entt::handle, entt::handle>>;

  // 开始合并分类

  clear(l_tree.begin(), l_ass_map_ass_file);
}

DOODLE_REGISTER_BEGIN(assets) {
  entt::meta<assets>()
      .ctor<>()
      .ctor<std::string>()
      .data<&assets::p_path>("p_path"_hs)
      .func<&assets::set_path>("set_path"_hs)
      .func<&assets::get_path>("get_path"_hs);
}

}  // namespace doodle
