//
// Created by TD on 2021/10/14.
//

#include "reference_attr_setting.h"

#include <doodle_lib/lib_warp/entt_warp.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata.h>

#include <maya/MTime.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnReference.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MUuid.h>
#include <maya/adskDataAssociations.h>
#include <maya/adskDataStream.h>
#include <maya/adskDebugPrint.h>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>

namespace doodle::maya_plug {

namespace reference_attr {

bool data::operator==(const data& in_rhs) const {
  return path == in_rhs.path;
}
bool data::operator!=(const data& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace reference_attr

reference_attr_setting::reference_attr_setting()
    : command_base(),
      p_handle() {
  p_name          = "引用编辑";
  p_show_str      = make_imgui_name(this,
                                    "解析引用",
                                    "引用",
                                    "设置场景",
                                    "保存");
  auto k_ref_view = g_reg()->view<reference_file>();
  std::transform(k_ref_view.begin(), k_ref_view.end(),
                 std::back_inserter(p_handle),
                 [](auto& in_e) {
                   return make_handle(in_e);
                 });
}

bool reference_attr_setting::get_file_info() {
  adsk::Debug::Print k_p{std::cout};
  MStatus k_status{};
  DOODLE_CHICK(k_status);
  for (auto& k_h : p_handle) {
    k_h.destroy();
  }
  p_handle.clear();

  MStatus k_s{};
  MFnReference k_ref_file{};
  for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
    k_s = k_ref_file.setObject(refIter.thisNode());
    DOODLE_CHICK(k_s);
    auto k_obj = refIter.thisNode(&k_s);
    DOODLE_CHICK(k_s);
    auto k_h = make_handle();
    try {
      k_h.emplace<reference_file>(g_reg()->ctx<root_ref>().root_handle(), k_obj);
      DOODLE_CHICK(k_s);
      p_handle.push_back(k_h);
    } catch (maya_error& err) {
      DOODLE_LOG_WARN("跳过无效的引用");
      k_h.destroy();
    }
  }

  auto k_j_str = maya_file_io::get_channel_date();
  if (k_j_str.empty())
    return true;
  auto k_j = nlohmann::json::parse(k_j_str);

  for (auto& l_i : p_handle) {
    auto& k_ref = l_i.get<reference_file>();
    auto l_p    = k_ref.path;
    if (k_j.contains(l_p))
      entt_tool::load_comm<reference_file>(l_i, k_j.at(l_p));
    l_i.get<reference_file>().init_show_name();
  }
  return true;
}

bool reference_attr_setting::render() {
  if (imgui::Button(p_show_str["解析引用"].c_str())) {
    get_file_info();
  }
  MStatus k_s{};
  MSelectionList l_select{};
  auto k_ref_view = g_reg()->view<reference_file>();

  for (auto k_e : k_ref_view) {
    auto& k_ref = k_ref_view.get<reference_file>(k_e);
    dear::TreeNode{k_ref.path.c_str()} && [&]() {
      imgui::Checkbox("解算", &(k_ref.use_sim));
      if (!k_ref.use_sim)
        return;

      imgui::Checkbox("高精度配置", &(k_ref.high_speed_sim));
      if (imgui::Button("替换")) {
        k_s = MGlobal::getActiveSelectionList(l_select);
        DOODLE_CHICK(k_s);
        k_ref.set_collision_model(l_select);
      }
      if (imgui::Button("获取")) {
        MGlobal::setActiveSelectionList(k_ref.get_collision_model());
      }
      dear::Text("解算碰撞");
      for (const auto& k_f : k_ref.collision_model_show_str)
        dear::Text(k_f);

      // if (imgui::Button("test")) {
      //   try {
      //     k_ref.replace_sim_assets_file();
      //   } catch (const maya_error& e) {
      //     std::cerr << e.what() << '\n';
      //   }
      // }
    };
  }

  if (imgui::Button(p_show_str["保存"].c_str())) {
    maya_file_io::chick_channel();
    nlohmann::json k_j{};
    for (auto& k : p_handle) {
      entt_tool::save_comm<reference_file>(k, k_j[k.get<reference_file>().path]);
    }
    maya_file_io::replace_channel_date(k_j.dump());
  }

  return true;
}

}  // namespace doodle::maya_plug
