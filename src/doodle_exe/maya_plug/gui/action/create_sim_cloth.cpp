//
// Created by TD on 2021/12/20.
//

#include "create_sim_cloth.h"
#include <maya_plug/maya_plug_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <maya_plug/data/qcloth_shape.h>
#include <maya_plug/data/maya_tool.h>

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnDependencyNode.h>

namespace doodle::maya_plug {

create_sim_cloth::create_sim_cloth()
    : p_coll(make_handle()) {
  auto k_view = g_reg()->view<qcloth_shape>();
  std::transform(k_view.begin(), k_view.end(),
                 std::back_inserter(p_list),
                 [](auto& in) -> entt::handle { return make_handle(in); });
}
bool create_sim_cloth::render() {
  if (imgui::Button("获得低模")) {
    MSelectionList k_list{};
    auto k_s = MGlobal::getActiveSelectionList(k_list);
    DOODLE_CHICK(k_s);
    MObject k_node{};
    for (MItSelectionList i{k_list}; !i.isDone(); i.next()) {
      auto k_h = make_handle();
      k_s      = i.getDependNode(k_node);
      DOODLE_CHICK(k_s);
      k_h.emplace<qcloth_shape_n::maya_obj>(k_node);
      k_h.emplace<qcloth_shape>();
      k_h.emplace<qcloth_shape_n::shape_list>();
      p_list.push_back(k_h);
    }
  }
  imgui::SameLine();
  if (imgui::Button("添加碰撞")) {
    qcloth_shape_n::shape_list l_list{};
    MSelectionList k_list{};
    auto k_s = MGlobal::getActiveSelectionList(k_list);
    DOODLE_CHICK(k_s);
    MObject k_node{};
    for (MItSelectionList i{k_list}; !i.isDone(); i.next()) {
      k_s = i.getDependNode(k_node);
      l_list.emplace_back(k_node);
    }
    p_coll.emplace_or_replace<qcloth_shape_n::shape_list>(l_list);
  }
  imgui::SameLine();
  if (imgui::Button("清理")) {
    for (auto& h : p_list) {
      h.destroy();
    }
    p_list.clear();
    p_coll.remove<qcloth_shape_n::shape_list>();
    g_reg()->unset<qcloth_shape::cloth_group>();
  }

  for (auto& l_h : p_list) {
    dear::TreeNode{l_h.get<qcloth_shape_n::maya_obj>().p_name.c_str()} && [&]() {
      if (imgui::Button("获得高模")) {
        MSelectionList k_list{};
        auto k_s = MGlobal::getActiveSelectionList(k_list);
        DOODLE_CHICK(k_s);
        MObject k_node{};
        qcloth_shape_n::shape_list l_list{};
        for (MItSelectionList i{k_list}; !i.isDone(); i.next()) {
          k_s = i.getDependNode(k_node);
          DOODLE_CHICK(k_s);

          chick_true<maya_error>(k_node != l_h.get<qcloth_shape_n::maya_obj>().obj, DOODLE_LOC, "低模和高模是相同的");

          l_list.emplace_back(k_node);
        }
        l_h.emplace<qcloth_shape_n::shape_list>(l_list);
      }
      for (auto& k_hig_mesh : l_h.get<qcloth_shape_n::shape_list>()) {
        dear::Text(k_hig_mesh.p_name);
      }
    };
  }

  dear::Text("碰撞物体"s);
  if (p_coll.any_of<qcloth_shape_n::shape_list>()) {
    for (auto&& l_obj : p_coll.get<qcloth_shape_n::shape_list>()) {
      dear::Text(l_obj.p_name);
    }
  }

  if (imgui::Button("制作布料")) {
    for (auto& l_h : p_list) {
      l_h.get<qcloth_shape>().create_sim_cloth(l_h);
    }
    if (p_coll.any_of<qcloth_shape_n::shape_list>() && !p_list.empty())
      p_list.front().get<qcloth_shape>().add_collider(p_coll);
  }
  return false;
}
create_sim_cloth::~create_sim_cloth() {
  p_coll.destroy();
  for (auto& h : p_list) {
    if (h)
      h.destroy();
  }
}
void create_sim_cloth::init() {
}
void create_sim_cloth::succeeded() {
}
void create_sim_cloth::failed() {
}
void create_sim_cloth::aborted() {
}
void create_sim_cloth::update(delta_type, void* data) {
  render();
}
}  // namespace doodle::maya_plug
