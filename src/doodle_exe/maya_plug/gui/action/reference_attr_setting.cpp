//
// Created by TD on 2021/10/14.
//

#include "reference_attr_setting.h"

#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/thread_pool/process_pool.h>

#include <maya/MTime.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MUuid.h>
#include <maya/adskDataAssociations.h>
#include <maya/adskDataStream.h>
#include <maya/adskDebugPrint.h>
#include <maya/MNamespace.h>

#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>
#include <maya_plug/maya_plug_fwd.h>
#include <maya_plug/data/sim_overr_attr.h>

namespace doodle {
namespace maya_plug {

namespace reference_attr {

bool data::operator==(const data& in_rhs) const {
  return path == in_rhs.path;
}
bool data::operator!=(const data& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace reference_attr

reference_attr_setting::reference_attr_setting()
    : p_handle() {
  title_name_     = std::string{name};
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
  destroy_handle(p_handle);

  MStatus k_s{};
  auto k_names = MNamespace::getNamespaces(MNamespace::rootNamespace(), false, &k_status);

  for (int l_i = 0; l_i < k_names.length(); ++l_i) {
    auto&& k_name = k_names[l_i];
    reference_file k_ref{};

    if (k_ref.set_namespace(d_str{k_name})) {
      auto k_h = make_handle();
      k_h.emplace<reference_file>(k_ref);
      p_handle.push_back(k_h);
    } else {
      DOODLE_LOG_WARN("命名空间 {} 中无有效引用", k_name);
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

void reference_attr_setting::render() {
  if (imgui::Button("解析引用")) {
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

      if (imgui::Checkbox("高精度配置", &(k_ref.high_speed_sim))) {
        auto k_h = make_handle(k_e);
        if (!k_ref.high_speed_sim) {
          g_main_loop().attach([=](auto, auto, auto s, auto f) {
            k_h.erase<sim_overr_attr>();
            s();
          });
        }
      }
      if (imgui::Button("添加碰撞")) {
        k_s = MGlobal::getActiveSelectionList(l_select);
        DOODLE_CHICK(k_s);
        k_ref.set_collision_model(l_select);
      }
      ImGui::SameLine();
      if (imgui::Button("选择已添加")) {
        MGlobal::setActiveSelectionList(k_ref.get_collision_model());
      }
      dear::Text("解算碰撞: "s);
      for (const auto& k_f : k_ref.collision_model_show_str)
        dear::Text(k_f);

      auto& in_attr = make_handle(k_e).get_or_emplace<maya_plug::sim_overr_attr>();
      dear::Checkbox("精密解算", &in_attr.simple_subsampling);
      if (in_attr.simple_subsampling) {
        dear::SliderFloat("帧样本", &in_attr.frame_samples, 0.1f, 50.f);
        dear::SliderFloat("时间尺度", &in_attr.time_scale, 0.1f, 10.f);
        dear::SliderFloat("长度尺度", &in_attr.length_scale, 0.1f, 10.f);
        dear::Checkbox("尖锐碰撞", &in_attr.sharp_feature);
      }
    };
  }

  if (imgui::Button("保存")) {
    maya_file_io::chick_channel();
    nlohmann::json k_j{};
    for (auto& k : p_handle) {
      entt_tool::save_comm<reference_file>(k, k_j[k.get<reference_file>().path]);
    }
    maya_file_io::replace_channel_date(k_j.dump());
  }

  return;
}

void reference_attr_setting::clear() {
  destroy_handle(p_handle);
}
reference_attr_setting::~reference_attr_setting() {
  clear();
}

}  // namespace maya_plug

}  // namespace doodle
