//
// Created by TD on 2021/10/14.
//

#include "reference_attr_setting.h"

#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/metadata/metadata.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/sim_cover_attr.h>
#include <maya_plug/main/maya_plug_fwd.h>
#include <maya_plug/node/files_info.h>

#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItSelectionList.h>
#include <maya/MNamespace.h>
#include <maya/MTime.h>
#include <maya/MUuid.h>

namespace doodle::maya_plug {

struct gui_tree {
  std::string name;
  MObject ref_node_;

  gui::gui_cache<bool> use_sim{"use sim", false};

  std::vector<std::string> collision_model_show_str;
  std::string wind_field_show_str;

  // 解算属性复盖
  gui::gui_cache<bool> sim_override{"解算属性复盖", false};
  gui::gui_cache<bool> simple_subsampling{"simple subsampling", true};
  gui::gui_cache<std::int32_t> frame_samples{"frame samples"s, 6};
  gui::gui_cache<std::float_t> time_scale{"time scale"s, 1.0f};
  gui::gui_cache<std::float_t> length_scale{"length scale"s, 1.0f};
  gui::gui_cache<std::int32_t> max_cg_iteration{"max CGIteration"s, 1000};
  gui::gui_cache<std::int32_t> cg_accuracy{"cg accuracy"s, 9};
  gui::gui_cache<std::array<std::float_t, 3>> gravity{"gravity"s, std::array<std::float_t, 3>{0.0f, -980.0f, 0.0f}};
};
class reference_attr_setting::impl {
 public:
  std::string title_name_;
  bool open{true};
  std::vector<gui_tree> p_ref_nodes;
  MObject p_current_node;
};

reference_attr_setting::reference_attr_setting() : p_i(std::make_unique<impl>()) {
  p_i->title_name_ = std::string{name};
}

bool reference_attr_setting::get_file_info() {
  MStatus l_status{};

  for (MItDependencyNodes l_it{MFn::kPluginDependNode, &l_status}; !l_it.isDone(); l_it.next()) {
    MFnDependencyNode l_fn_node{l_it.thisNode(), &l_status};
    maya_chick(l_status);
    if (l_fn_node.typeId() == doodle_file_info::doodle_id) {
      if (l_fn_node.findPlug(doodle_file_info::reference_file_path, true).asString().length() != 0) {
        p_i->p_ref_nodes.emplace_back(l_fn_node.name().asChar(), l_it.thisNode());
        maya_chick(l_status);
      }
    }
  }
  // 过滤有效
  p_i->p_ref_nodes |= ranges::actions::remove_if([&](const gui_tree& in) -> bool {
    reference_file l_ref{in.ref_node_};
    return !l_ref.export_group_attr();
  });

  // 过滤选中
  {
    MSelectionList l_selection_list{};
    l_status = MGlobal::getActiveSelectionList(l_selection_list);
    maya_chick(l_status);
    MObject l_obj{};
    p_i->p_ref_nodes |= ranges::actions::remove_if([&](gui_tree& in) -> bool {
      reference_file l_ref{in.ref_node_};
      return !l_ref.has_node(l_selection_list);
    });
  }

  // 调整名称
  ranges::for_each(p_i->p_ref_nodes, [](gui_tree& in) {
    reference_file l_ref{in.ref_node_};
    in.name = l_ref.get_namespace();
  });
  // 刷新属性
  ranges::for_each(p_i->p_ref_nodes, [](gui_tree& in) {
    in.use_sim            = get_attribute<bool>(in.ref_node_, "is_solve");
    in.sim_override       = get_attribute<bool>(in.ref_node_, "sim_override");
    in.simple_subsampling = get_attribute<bool>(in.ref_node_, "simple_subsampling");
    in.frame_samples      = get_attribute<std::int32_t>(in.ref_node_, "frame_samples");
    in.time_scale         = get_attribute<std::float_t>(in.ref_node_, "time_scale");
    in.length_scale       = get_attribute<std::float_t>(in.ref_node_, "length_scale");
    in.max_cg_iteration   = get_attribute<std::int32_t>(in.ref_node_, "max_cg_iteration");
    in.cg_accuracy        = get_attribute<std::int32_t>(in.ref_node_, "cg_accuracy");

    auto l_plug           = get_plug(in.ref_node_, "gravity");
    in.gravity.data       = {l_plug[0].asFloat(), l_plug[1].asFloat(), l_plug[2].asFloat()};

    MStatus l_status{};
    MFnDependencyNode l_fn_node{in.ref_node_};

    auto l_collision_objects_plug = get_plug(in.ref_node_, "collision_objects");

    for (auto i = 0; i < l_collision_objects_plug.numConnectedElements(); ++i) {
      auto l_connected_plug = l_collision_objects_plug.connectionByPhysicalIndex(i, &l_status);
      maya_chick(l_status);
      in.collision_model_show_str.emplace_back(get_node_full_name(l_connected_plug.source().node()));
    }

    auto l_wind_field_plug = get_plug(in.ref_node_, "wind_field");
    if (l_wind_field_plug.isConnected()) {
      in.wind_field_show_str = get_node_full_name(l_wind_field_plug.source().node());
    }
  });
  return true;
}

void reference_attr_setting::add_collision(const MObject& in_obj) {
  if (in_obj.isNull()) return;

  auto l_com = fmt::format("doodle_file_info_edit -add_collision -node {}", get_node_full_name(in_obj));
  MGlobal::executeCommand(conv::to_ms(l_com), true, true);

  MStatus l_status{};
  MSelectionList l_select{};
  maya_chick(MGlobal::getActiveSelectionList(l_select));
  MStringArray l_str{};
  l_select.getSelectionStrings(l_str);

  // 寻找gui_tree
  auto l_gui_tree =
      ranges::find_if(p_i->p_ref_nodes, [&](const gui_tree& in) -> bool { return in.ref_node_ == in_obj; });
  if (l_gui_tree == p_i->p_ref_nodes.end()) return;
  // 设置碰撞gui数据
  l_gui_tree->collision_model_show_str.clear();
  for (auto& i : l_str) {
    l_gui_tree->collision_model_show_str.emplace_back(conv::to_s(i));
  }
}
void reference_attr_setting::set_attr(
    const MObject& in_obj, const std::string& in_attr_name, const std::string& in_value
) {
  if (in_obj.isNull()) return;
  auto l_com =
      fmt::format("doodle_file_info_edit -node {} -{} {} ", get_node_full_name(in_obj), in_attr_name, in_value);
  MGlobal::executeCommand(conv::to_ms(l_com), true, true);
}

void reference_attr_setting::add_wind_field(const MObject& in_obj) {
  if (in_obj.isNull()) return;

  auto l_com = fmt::format("doodle_file_info_edit -add_wind_field -node {}", get_node_full_name(in_obj));
  MGlobal::executeCommand(conv::to_ms(l_com), true, true);

  MStatus l_status{};
  MSelectionList l_select{};
  maya_chick(MGlobal::getActiveSelectionList(l_select));
  MStringArray l_str{};
  l_select.getSelectionStrings(l_str);

  // 寻找gui_tree

  auto l_gui_tree =
      ranges::find_if(p_i->p_ref_nodes, [&](const gui_tree& in) -> bool { return in.ref_node_ == in_obj; });
  if (l_gui_tree == p_i->p_ref_nodes.end()) return;
  // 设置风场gui数据
  l_gui_tree->wind_field_show_str = conv::to_s(l_str[0]);
}

void reference_attr_setting::get_collision(const MObject& in_obj) {
  if (in_obj.isNull()) return;
  auto l_collision_objects_plug = get_plug(in_obj, "collision_objects");

  MSelectionList l_select{};
  MStatus l_status{};
  for (auto i = 0; i < l_collision_objects_plug.numConnectedElements(); ++i) {
    auto l_connected_plug = l_collision_objects_plug.connectionByPhysicalIndex(i, &l_status);
    maya_chick(l_status);
    l_select.add(l_connected_plug.node());
  }
  MGlobal::setActiveSelectionList(l_select);
}

bool reference_attr_setting::render() {
  const ImGuiViewport* viewport = ImGui::GetMainViewport();

  if (auto l_c = dear::Child{"ref_file", ImVec2{0, viewport->WorkSize.y / 2}}; l_c) {
    if (imgui::Button("解析选中引用")) {
      get_file_info();
    }
    MStatus k_s{};
    MSelectionList l_select{};

    for (auto&& l_ref : p_i->p_ref_nodes) {
      dear::TreeNode l_node{l_ref.name.c_str()};

      if (!l_node) continue;

      if (imgui::Checkbox(*l_ref.use_sim, &l_ref.use_sim)) {
        set_attr(l_ref.ref_node_, "is_solve", fmt::to_string(l_ref.use_sim.data));
      }
      if (!l_ref.use_sim.data) continue;

      if (imgui::Button("添加碰撞")) {
        add_collision(l_ref.ref_node_);
      }
      ImGui::SameLine();
      if (imgui::Button("选择已添加")) {
        get_collision(l_ref.ref_node_);
      }
      if (imgui::Button("设置布料风场")) {
        add_wind_field(l_ref.ref_node_);
      }

      dear::Text("解算碰撞: "s);
      for (const auto& k_f : l_ref.collision_model_show_str) dear::Text(k_f);

      dear::Text(fmt::format("链接风场: {}", l_ref.wind_field_show_str));
      if (imgui::Checkbox(*l_ref.sim_override, &l_ref.sim_override)) {
        set_attr(l_ref.ref_node_, "sim_override", fmt::to_string(l_ref.sim_override.data));
      }
      if (!l_ref.sim_override.data) continue;

      if (ImGui::Checkbox(*l_ref.simple_subsampling, &l_ref.simple_subsampling)) {
        set_attr(l_ref.ref_node_, "simple_subsampling", fmt::to_string(l_ref.simple_subsampling.data));
      }
      if (ImGui::InputInt(*l_ref.frame_samples, &l_ref.frame_samples)) {
        set_attr(l_ref.ref_node_, "frame_samples", fmt::to_string(l_ref.frame_samples.data));
      }
      if (ImGui::InputFloat(*l_ref.time_scale, &l_ref.time_scale)) {
        set_attr(l_ref.ref_node_, "time_scale", fmt::to_string(l_ref.time_scale.data));
      }
      if (ImGui::InputFloat(*l_ref.length_scale, &l_ref.length_scale)) {
        set_attr(l_ref.ref_node_, "length_scale", fmt::to_string(l_ref.length_scale.data));
      }
      if (ImGui::InputInt(*l_ref.max_cg_iteration, &l_ref.max_cg_iteration)) {
        set_attr(l_ref.ref_node_, "max_cg_iteration", fmt::to_string(l_ref.max_cg_iteration.data));
      }
      if (ImGui::InputInt(*l_ref.cg_accuracy, &l_ref.cg_accuracy)) {
        set_attr(l_ref.ref_node_, "cg_accuracy", fmt::to_string(l_ref.cg_accuracy.data));
      }
      if (ImGui::InputFloat3(*l_ref.gravity, l_ref.gravity.data.data())) {
        set_attr(
            l_ref.ref_node_, "gravity",
            fmt::format(R"("{} {} {}")", l_ref.gravity.data[0], l_ref.gravity.data[1], l_ref.gravity.data[2])
        );
      }
    }
  }

  return p_i->open;
}

reference_attr_setting::~reference_attr_setting() = default;
const std::string& reference_attr_setting::title() const { return p_i->title_name_; }

}  // namespace doodle::maya_plug
