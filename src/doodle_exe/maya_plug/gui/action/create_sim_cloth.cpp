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

create_sim_cloth::create_sim_cloth() {
  p_show_str  = make_imgui_name(this,
                                "获得低模",
                                "制作布料");
  auto k_view = g_reg()->view<qcloth_shape>();
  g_reg()->destroy(k_view.begin(), k_view.end());
}
bool create_sim_cloth::render() {
  if (imgui::Button(p_show_str["获得低模"].c_str())) {
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
          l_list.emplace_back(k_node);
        }
        l_h.emplace<qcloth_shape_n::shape_list>(l_list);
      }
      for (auto& k_hig_mesh : l_h.get<qcloth_shape_n::shape_list>()) {
        dear::Text(k_hig_mesh.p_name);
      }
    };
  }

  if (imgui::Button(p_show_str["制作布料"].c_str())) {
    for (auto& l_h : p_list) {
      l_h.get<qcloth_shape>().create_sim_cloth(l_h);
    }
  }

  if (imgui::Button("test")) {
    MSelectionList k_list{};
    auto k_s = MGlobal::getActiveSelectionList(k_list);
    DOODLE_CHICK(k_s);
    MObject k_node{};

    for (MItDependencyNodes i{MFn::Type::kPluginLocatorNode}; !i.isDone(); i.next()) {
      auto k_obj = i.thisNode(&k_s);
      DOODLE_LOG_INFO(k_obj.apiTypeStr());
      MFnDependencyNode k_dep{k_obj};
      DOODLE_LOG_INFO("{}", k_dep.typeId(&k_s).id());
      auto k_name = k_dep.typeName(&k_s);
      DOODLE_CHICK(k_s);
      DOODLE_LOG_INFO("{}", d_str{k_name}.str());
    }
  }
  return false;
}
}  // namespace doodle::maya_plug
